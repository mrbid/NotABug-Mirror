#define _GNU_SOURCE
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <endian.h>
#include <time.h>
#include <signal.h>
#include <sys/mman.h>
#include <sched.h>
#include <sys/wait.h>

#include "main.h"
#include "types.h"
#include "utils.h"
#include "sleep.h"
#include "inc/protocol.h"

#define EXO_VERTICES_ONLY
#include "assets/models.h"

#include "config.h"

#include "inc/mutex.h"
#include "inc/munmap.h"

//#define PACKET_DUMP

#ifdef PACKET_DUMP
void SLEND(int sockfd, const char *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen)
{
	static int c=0;
	printf("!%u %02X %lu ", fd, (unsigned int)(buf[8]&0xFF), len);
	c++;
	if(c > 12){c=0;printf("\n");}
	//fflush(stdout);
	SENDTO(fd, buf, len, flags, addr, addrlen);
}
#else
#define SLEND SENDTO
#endif

int fd;

uint32_t main_lock = 0;
struct state_holder *states = 0;
uint64_t num_states = 0;

struct client_holder *all_clients = 0;
uint64_t num_clients = 0;

uint64_t verts_per_pod;

void skip_sleep_handler(int signal, siginfo_t *info, void *tmp) {
	struct ucontext_t *context = tmp;

	if (context->uc_mcontext.gregs[REG_RIP] >= (uint64_t)usleep2_start && context->uc_mcontext.gregs[REG_RIP] <= (uint64_t)usleep2_end)
		context->uc_mcontext.gregs[REG_RIP] = (uint64_t)usleep2_end;
}

int main_loop_world(void *tmp) {
#ifdef PACKET_DUMP
	printf("\nmain_loop_world\n");
#endif
	struct current_state *state = tmp; // annoying type checking
	void *my_stack = state->world_stack;

	{
		struct sigaction sleepthing;
		sleepthing.sa_restorer = 0;
		sleepthing.sa_sigaction = skip_sleep_handler;
		sleepthing.sa_flags = SA_SIGINFO;
		sigset_t set;
		sigemptyset(&set);
		sleepthing.sa_mask = set;

		sigaction(SIGUSR1, &sleepthing, NULL);
	}
#ifdef PACKET_DUMP
	printf("sigaction\nepoch:%lu\n", state->epoch);
#endif
	{
		uint64_t t = state->epoch - 1;
		for(uint64_t t1 = time(0); t1 < t; t1 = time(0)) // for loop abuse
		{
			const uint64_t tmp = t - t1;
#ifdef PACKET_DUMP
			printf("sleep((%lu-%lu)=%lu);\n", t, t1, tmp);
#endif
			sleep(tmp);
		}

		t = state->epoch * 1000000;
		for (uint64_t t1 = microtime(); t1 < t; t1 = microtime())
		{
			const uint64_t tmp = t - t1;
#ifdef PACKET_DUMP
			printf("\nusleep((%lu-%lu)=%lu);\n", t, t1, tmp);
#endif
			usleep(tmp);
		}
	}

	mutex_lock(&(state->lock));
#ifdef PACKET_DUMP
	printf("\nmutex_lock\n");
#endif
//	state->pod_thread = clone(&main_loop_pod, state->pod_stack+TOTAL_STACK_SIZE, CLONE_FLAGS, state);

	rand_all_asteroids(state, state->epoch * 1000000);
#ifdef PACKET_DUMP
	printf("rand_all_asteroids\n");
#endif

	uint8_t buffer[MAX_PACKET_SIZE];
	*((uint64_t*)buffer) = PROTOCOL_ID;

	buffer[8] = MSG_TYPE_ASTEROID_POS;
	*((uint64_t*)(buffer+9)) = HTOLE64(state->epoch * 1000000);
	uint16_t size = 17;

	//puts("FOR1");
	for (uint32_t asteroid_id = 0; asteroid_id <= (uint32_t)state->max_asteroid_id; asteroid_id++) {
		*((uint16_t*)(buffer+size)) = HTOLE16((uint16_t)asteroid_id);
		*((uint32_t*)(buffer+size+2)) = HTOLE32F(state->asteroids[asteroid_id].vel.x);
		*((uint32_t*)(buffer+size+6)) = HTOLE32F(state->asteroids[asteroid_id].vel.y);
		*((uint32_t*)(buffer+size+10)) = HTOLE32F(state->asteroids[asteroid_id].vel.z);
		*((uint32_t*)(buffer+size+14)) = HTOLE32F(state->asteroids[asteroid_id].start_pos.x);
		*((uint32_t*)(buffer+size+18)) = HTOLE32F(state->asteroids[asteroid_id].start_pos.y);
		*((uint32_t*)(buffer+size+22)) = HTOLE32F(state->asteroids[asteroid_id].start_pos.z);
		*((uint32_t*)(buffer+size+26)) = HTOLE32F(state->asteroids[asteroid_id].rot);
		*((uint32_t*)(buffer+size+30)) = HTOLE32F(state->asteroids[asteroid_id].scale);
		size += 34;
		if(size+34 > MAX_PACKET_SIZE || (uint16_t)asteroid_id == state->max_asteroid_id)
		{
			//puts("IF1");
			for (uint16_t pid = 0; pid <= (uint16_t)state->max_player_id; pid++)
			{
				//puts("FOR2");
				if (state->players[pid].valid)
				{
					//puts("IF2 - SEND");
					SLEND(fd, buffer, size, 0, (struct sockaddr*)&state->players[pid].address, sizeof(struct sockaddr));
				}
			}
			size = 17;
			usleep(10000); // 10ms delay between packets
		}
	}

	uint16_t hit_id = 0; // arbitrary number

	while (1) {
		uint16_t asteroid_id = 0;
		state->soonest_impact = state->asteroids[0].impact_time;
		for (uint32_t i = 1; i <= (uint32_t)state->max_asteroid_id; i++) {
			if (state->asteroids[i].impact_time < state->soonest_impact) {
				asteroid_id = i;
				state->soonest_impact = state->asteroids[i].impact_time;
			}
		}

		if (state->soonest_impact <= microtime()) {
			struct asteroid *asteroid = &(state->asteroids[asteroid_id]);
			uint64_t current_time = asteroid->impact_time;

			*((uint64_t*)(buffer+9)) = HTOLE64(current_time);

//			if (state->asteroids[asteroid_id].final_impact) {
				do_exo_impact(state, asteroid->impact_pos, (asteroid->scale+(vMod(asteroid->vel)*0.1f))*1.2f);

				buffer[8] = MSG_TYPE_EXO_HIT;

				*((uint16_t*)(buffer+17)) = HTOLE16(hit_id);
				*((uint16_t*)(buffer+19)) = HTOLE16(asteroid_id);
				*((uint32_t*)(buffer+21)) = HTOLE32F(asteroid->vel.x);
				*((uint32_t*)(buffer+25)) = HTOLE32F(asteroid->vel.y);
				*((uint32_t*)(buffer+29)) = HTOLE32F(asteroid->vel.z);
				*((uint32_t*)(buffer+33)) = HTOLE32F(asteroid->impact_pos.x);
				*((uint32_t*)(buffer+37)) = HTOLE32F(asteroid->impact_pos.y);
				*((uint32_t*)(buffer+41)) = HTOLE32F(asteroid->impact_pos.z);
				// @+45
				// 2 more vecs, +24
				*((uint32_t*)(buffer+69)) = HTOLE32F(asteroid->rot);
				*((uint32_t*)(buffer+73)) = HTOLE32F(asteroid->scale);

				rand_asteroid(state, asteroid, current_time);

				//+45 to +64
				*((uint32_t*)(buffer+45)) = HTOLE32F(asteroid->vel.x);
				*((uint32_t*)(buffer+49)) = HTOLE32F(asteroid->vel.y);
				*((uint32_t*)(buffer+53)) = HTOLE32F(asteroid->vel.z);
				*((uint32_t*)(buffer+57)) = HTOLE32F(asteroid->start_pos.x);
				*((uint32_t*)(buffer+61)) = HTOLE32F(asteroid->start_pos.y);
				*((uint32_t*)(buffer+65)) = HTOLE32F(asteroid->start_pos.z);
				// @+69
				// 2 f32s, +8
				*((uint32_t*)(buffer+77)) = HTOLE32F(asteroid->rot);
				*((uint32_t*)(buffer+81)) = HTOLE32F(asteroid->scale);
				if (state->damage >= exo_numvert/2)
					*((uint32_t*)(buffer+85)) = HTOLE32F(20.f);
				else
					*((uint32_t*)(buffer+85)) = HTOLE32F((f32)state->damage * 2 / exo_numvert);
				// total size: 89

				for (uint16_t pid = 0; pid <= (uint16_t)state->max_player_id; pid++)
					if (state->players[pid].valid)
						SLEND(fd, buffer, MSG_SIZE_EXO_HIT, 0, (struct sockaddr*)&state->players[pid].address, sizeof(struct sockaddr));

				memcpy(state->reliable_data_exo[hit_id], buffer, MSG_SIZE_EXO_HIT);
				for (uint16_t pid = 0; pid <= (uint16_t)state->max_player_id; pid++) {
					if (state->players[pid].valid) {
						state->reliable_timestamps_exo[hit_id][pid] = current_time + RESEND_TIME;
						state->reliable_attempts_exo[hit_id][pid] = 0;
						if (current_time + RESEND_TIME < state->soonest_resend) {
							state->soonest_resend = current_time + RESEND_TIME;
							kill(state->resend_thread, SIGUSR1);
						}
					}
				}

				hit_id++;
//			} else {
//				struct asteroid *asteroid = &(state->asteroids[asteroid_id]);
//				if (state->pods[asteroid->pod_id].state == POD_STATE_SPAWNED) {
//					buffer[8] = MSG_TYPE_POD_STATE_CHANGED;
//					buffer[17] = POD_STATE_DESTROYED;
//
//					state->damage += state->pods[asteroid->pod_id].saved;
//
//					*((uint16_t*)(buffer+18)) = HTOLE16(asteroid->pod_id);
//					*((uint16_t*)(buffer+20)) = HTOLE32F((f32)state->damage / (f32)exo_numvert);
//				}
//
//				// TODO: check for if it's going to run into another pod
//
//				asteroid->final_impact = 1;
//				asteroid->impact_time = asteroid->final_impact_time;
//			}

			if ((state->damage) >= exo_numvert/2) {
				mutex_unlock(&(state->lock));
				mutex_lock(&main_lock);
				mutex_lock(&(state->lock)); // ordering to avoid deadlocks

				kill(state->player_thread, SIGKILL);
//				kill(state->pod_thread, SIGKILL);
				kill(state->resend_thread, SIGKILL);
				// don't kill self :P

				waitpid(state->player_thread, NULL, 0); // just to ensure we don't segfault the other threads when unmapping
//				waitpid(state->pod_thread, NULL, 0);
				waitpid(state->resend_thread, NULL, 0);

				munmap(state->player_stack, TOTAL_STACK_SIZE);
//				munmap(state->pod_stack, TOTAL_STACK_SIZE);
				munmap(state->resend_stack, TOTAL_STACK_SIZE);

				for (uint16_t pid = 0; pid <= (uint16_t)state->max_player_id; pid++)
					if (state->players[pid].valid)
						all_clients[state->players[pid].client_index].valid = 0;

				states[state->state_index].epoch = 0;

				free(state->players);
				free(state->asteroids);
				free(state->damage_index);
				free(state);

				mutex_unlock(&main_lock);

				// signals between unmapping and exiting will segfault and kill the main thread, don't listen to them
				sigset_t sigset;
				sigfillset(&sigset);
				sigprocmask(SIG_SETMASK, &sigset, 0);

				munmap_and_exit(my_stack, TOTAL_STACK_SIZE);
			}
		} else {
			mutex_unlock(&(state->lock));

			usleep2(&(state->soonest_impact), microtime());

			mutex_lock(&(state->lock));
		}
	}

	return 0;
}

int main_loop_send_players(void *tmp) {
	struct current_state *state = tmp; // annoying type checking

	uint64_t current_time;
	uint64_t last_time = microtime();

	uint8_t buffer[MAX_PACKET_SIZE];
	*((uint64_t*)buffer) = PROTOCOL_ID;
	buffer[8] = MSG_TYPE_PLAYER_POS;
	while (1) {
		current_time = microtime();
		mutex_lock(&(state->lock));

		f32 scale = (f32)(current_time - last_time) * 0.000001f;

		*((uint64_t*)(buffer+9)) = HTOLE64(current_time);
		uint16_t size = 17;
		for (uint16_t player_id = 0; player_id <= (uint16_t)state->max_player_id; player_id++) {
			if (state->players[player_id].valid) {
				*((uint8_t*)(buffer+size)) = player_id;

				state->players[player_id].pos.x += state->players[player_id].vel.x * scale;
				state->players[player_id].pos.y += state->players[player_id].vel.y * scale;
				state->players[player_id].pos.z += state->players[player_id].vel.z * scale;

				*((uint32_t*)(buffer+size+1)) = HTOLE32F(state->players[player_id].pos.x);
				*((uint32_t*)(buffer+size+5)) = HTOLE32F(state->players[player_id].pos.y);
				*((uint32_t*)(buffer+size+9)) = HTOLE32F(state->players[player_id].pos.z);
				*((uint32_t*)(buffer+size+13)) = HTOLE32F(state->players[player_id].vel.x);
				*((uint32_t*)(buffer+size+17)) = HTOLE32F(state->players[player_id].vel.y);
				*((uint32_t*)(buffer+size+21)) = HTOLE32F(state->players[player_id].vel.z);

				*((uint32_t*)(buffer+size+25)) = HTOLE32F(state->players[player_id].front.x);
				*((uint32_t*)(buffer+size+29)) = HTOLE32F(state->players[player_id].front.y);
				*((uint32_t*)(buffer+size+33)) = HTOLE32F(state->players[player_id].front.z);
				*((uint32_t*)(buffer+size+37)) = HTOLE32F(state->players[player_id].up.x);
				*((uint32_t*)(buffer+size+41)) = HTOLE32F(state->players[player_id].up.y);
				*((uint32_t*)(buffer+size+45)) = HTOLE32F(state->players[player_id].up.z);

				size += 49;
			}

			if (size+49 > MAX_PACKET_SIZE || (uint8_t)player_id == state->max_player_id) {
				for (uint16_t pid = 0; pid <= (uint16_t)state->max_player_id; pid++)
					if (state->players[pid].valid)
						SLEND(fd, buffer, size, 0, (struct sockaddr*)&state->players[pid].address, sizeof(struct sockaddr));

				size = 17;
			}
		}

		state->last_player_update = current_time;

		mutex_unlock(&(state->lock));
		last_time = current_time;
		register uint64_t t = microtime();
		if (t < last_time + PLAYER_SEND_UTIME)
			usleep(last_time + PLAYER_SEND_UTIME - t);
		else
			fprintf(stderr, "WARNING: Player send took too long, skipping sleep! (%luus)\n", t-last_time);
	}

	return 0;
}

//int main_loop_pod(void *tmp) {
//	struct current_state *state = tmp;
//
//	uint8_t no_verts_left = 0;
//
//	uint64_t last_time = microtime();
//
//	uint8_t buffer[32];
//	*((uint64_t*)buffer) = PROTOCOL_ID;
//	buffer[8] = MSG_TYPE_POD_STATE_CHANGED;
//	buffer[19] = POD_STATE_SPAWNED;
//
//	while (1) {
//		{
//			uint64_t next = last_time + POD_SPAWN_UTIME;
//
//			uint64_t t = (next / 1000000) - 1;
//			for (uint64_t t1 = time(0); t1 < t; t1 = time(0))
//				sleep(t - t1);
//
//			for (uint64_t t1 = microtime(); t1 < next; t1 = microtime())
//				usleep(next - t1);
//		}
//
//		mutex_lock(&(state->lock));
//
//		if (state->pods_waiting > MAX_WAITING_PODS) {
//			mutex_unlock(&(state->lock));
//			last_time = microtime();
//			continue;
//		}
//
//		uint64_t current_time = microtime();
//
//		uint64_t saved = 0;
//		for (uint64_t i = 0; i < exo_numvert; i++) {
//			if (!state->damage_index[i]) {
//				saved++;
//				state->damage_index[i] = 1;
//				if (saved >= verts_per_pod)
//					break;
//			} else if (!saved && i == exo_numvert-1) {
//				no_verts_left = 1;
//			}
//		}
//
//		if (no_verts_left) { // nothing left for us to do here; stack will be unmapped by whatever thread terminates the world for now
//			state->pod_thread = -1;
//			mutex_unlock(&(state->lock));
//			return 0;
//		}
//
//		state->pods_waiting++;
//
//		uint16_t pod_id;
//		for (uint16_t i = 0; i < state->max_pod_id; i++) { // will always find one
//			if (state->pods[i].state & 1 == 0) { // destroyed: 0, delivered: 2
//				pod_id = i;
//				break;
//			}
//		}
//
//		state->pods[pod_id].state = POD_STATE_SPAWNED;
//
//		vec pos;
//		vRuv(&pos);
//		while (pos.x == 0.f && pos.y == 0.f && pos.z == 0.f)
//			vRuv(&pos);
//
//		vNorm(&pos);
//		pos.x *= POD_SPAWN_RADIUS;
//		pos.y *= POD_SPAWN_RADIUS;
//		pos.z *= POD_SPAWN_RADIUS;
//
//		state->pods[pod_id].pos = pos;
//
//		state->pods[pod_id].saved = saved;
//
//		*((uint64_t*)(buffer+9)) = HTOLE64(current_time);
//		*((uint16_t*)(buffer+17)) = HTOLE16(pod_id);
//		*((uint32_t*)(buffer+20)) = HTOLE32F(pos.x);
//		*((uint32_t*)(buffer+24)) = HTOLE32F(pos.y);
//		*((uint32_t*)(buffer+28)) = HTOLE32F(pos.z);
//
//		for (uint16_t pid = 0; pid <= (uint16_t)state->max_player_id; pid++)
//			if (state->players[pid].valid)
//				SLEND(fd, buffer, 32, 0, (struct sockaddr*)&state->players[pid].address, sizeof(struct sockaddr));
//
//		mutex_unlock(&(state->lock));
//
//		last_time = current_time;
//	}
//
//	return 0;
//}

// The state's lock is assumed to be held when calling this
static inline void timeout_player(struct current_state *state, uint8_t pid, uint64_t timestamp) {
	state->players[pid].valid = 0;

	*((uint64_t*)(state->reliable_data_player[pid])) = PROTOCOL_ID;
	state->reliable_data_player[pid][8] = MSG_TYPE_PLAYER_DISCONNECTED;
	state->reliable_data_player[pid][9] = pid;

	for (uint16_t oid = 0; oid < state->max_player_id; oid++) {
		if (!state->players[pid].valid)
			continue;
		state->reliable_attempts_player[pid][oid] = 0;
		state->reliable_timestamps_player[pid][oid] = timestamp + RESEND_TIME;

		SLEND(fd, state->reliable_data_player[pid], 10, 0, (struct sockaddr*)&state->players[pid].address, sizeof(struct sockaddr));
	}
}

int main_loop_resend(void *tmp) {
	struct current_state *state = tmp;

	void *my_stack = state->resend_stack;

	{
		struct sigaction sleepthing;
		sleepthing.sa_restorer = 0;
		sleepthing.sa_sigaction = skip_sleep_handler;
		sleepthing.sa_flags = SA_SIGINFO;
		sigset_t set;
		sigemptyset(&set);
		sleepthing.sa_mask = set;

		sigaction(SIGUSR1, &sleepthing, NULL);
	}

	mutex_lock(&(state->lock));

	while (1) {
		uint64_t current_time = microtime();

		state->soonest_resend = 0xFFFFFFFFFFFFFFFF;

		for (uint32_t aid = 0; aid <= state->max_asteroid_id; aid++) {
			for (uint16_t pid = 0; pid <= state->max_player_id; pid++) {
				if (!state->players[pid].valid)
					continue;

				if (!state->reliable_timestamps_asteroid[aid][pid])
					continue;

				if (state->reliable_attempts_asteroid[aid][pid] > MAX_RESEND_ATTEMPTS) {
					timeout_player(state, pid, current_time);
					mutex_lock(&main_lock);
					all_clients[state->players[pid].client_index].valid = 0;
					mutex_unlock(&main_lock);
					continue;
				} else if (state->reliable_timestamps_asteroid[aid][pid] <= current_time) {
					SLEND(fd, state->reliable_data_asteroid[aid], 51, 0, (struct sockaddr*)&state->players[pid].address, sizeof(struct sockaddr));
					state->reliable_attempts_asteroid[aid][pid]++;
					state->reliable_timestamps_asteroid[aid][pid] = current_time + RESEND_TIME;
				} else if (state->reliable_timestamps_asteroid[aid][pid] < state->soonest_resend) {
					state->soonest_resend = state->reliable_timestamps_asteroid[aid][pid];
				}
			}
		}

		for (uint32_t eid = 0; eid <= 65535; eid++) {
			for (uint16_t pid = 0; pid <= state->max_player_id; pid++) {
				if (!state->players[pid].valid)
					continue;

				if (!state->reliable_timestamps_exo[eid][pid])
					continue;

				if (state->reliable_attempts_exo[eid][pid] > MAX_RESEND_ATTEMPTS) {
					timeout_player(state, pid, current_time);
					mutex_lock(&main_lock);
					all_clients[state->players[pid].client_index].valid = 0;
					mutex_unlock(&main_lock);
					continue;
				} else if (state->reliable_timestamps_exo[eid][pid] <= current_time) {
					SLEND(fd, state->reliable_data_exo[eid], MSG_SIZE_EXO_HIT, 0, (struct sockaddr*)&state->players[pid].address, sizeof(struct sockaddr));
					state->reliable_attempts_exo[eid][pid]++;
					state->reliable_timestamps_exo[eid][pid] = current_time + RESEND_TIME;
				} else if (state->reliable_timestamps_exo[eid][pid] < state->soonest_resend) {
					state->soonest_resend = state->reliable_timestamps_exo[eid][pid];
				}
			}
		}

		for (uint32_t oid = 0; oid <= state->max_player_id; oid++) {
			for (uint16_t pid = 0; pid <= state->max_player_id; pid++) {
				if (!state->players[pid].valid)
					continue;

				if (!state->reliable_timestamps_player[oid][pid])
					continue;

				if (state->reliable_attempts_player[oid][pid] > MAX_RESEND_ATTEMPTS) {
					timeout_player(state, pid, current_time);
					mutex_lock(&main_lock);
					all_clients[state->players[pid].client_index].valid = 0;
					mutex_unlock(&main_lock);
					continue;
				} else if (state->reliable_timestamps_player[oid][pid] <= current_time) {
					SLEND(fd, state->reliable_data_player[oid], 10, 0, (struct sockaddr*)&state->players[pid].address, sizeof(struct sockaddr));
					state->reliable_attempts_player[oid][pid]++;
					state->reliable_timestamps_player[oid][pid] = current_time + RESEND_TIME;
				} else if (state->reliable_timestamps_player[oid][pid] < state->soonest_resend) {
					state->soonest_resend = state->reliable_timestamps_player[oid][pid];
				}
			}
		}

		if (microtime() < state->soonest_resend) {
			mutex_unlock(&(state->lock));

			usleep2(&(state->soonest_resend), microtime());

			mutex_lock(&(state->lock));
		}
	}

	return 0;
}

#define GFX_SCALE 0.01f
void scaleBuffer(GLfloat* b, GLsizeiptr s) {
	for(GLsizeiptr i = 0; i < s; i++)
		b[i] *= GFX_SCALE * 1.025;
}

int main(int argc, char **argv) {
	{
		struct sigaction ignore_sigchld = {
			.sa_handler = SIG_IGN,
		};
		if (sigaction(SIGCHLD, &ignore_sigchld, 0)) {
			fputs("ERROR: Unable to setup signal handler!\n(this should never actually happen)\n", stderr);
			return 1;
		}
	}

//	min_time_to_impact = min_asteroid_time();

	verts_per_pod = exo_numvert/PODS_PER_WORLD;
	if (verts_per_pod * PODS_PER_WORLD < exo_numvert) {
		verts_per_pod++; // round up
	}

	scaleBuffer(exo_vertices, exo_numvert*3);

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		fputs("ERROR: Unable to create socket!\n", stderr);
		return 1;
	}

	{ // no reason to keep `server` and `len` around
		struct sockaddr_in server;
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = INADDR_ANY;

		if (argc > 1) {
			uint32_t port;
			if (strlen(argv[1]) > 5 || sscanf(argv[1], "%u", &port) != 1 || port > 65535) {
				fprintf(stderr, "ERROR: Invalid syntax! Syntax: %s [port]\n", argv[0]);
				return 2;
			} else if (port == 0) {
				fputs("WARNING: Port 0 is automatically chosen by the OS; if you do not intend this, restart this program and select a different port.\n", stderr);
			}
			server.sin_port = HTOBE16((uint16_t)port);
		} else {
			server.sin_port = SERVER_PORT;
		}

		if (bind(fd, (struct sockaddr*)&server, sizeof(server)) != 0) {
			fputs("ERROR: Unable to bind socket!\n", stderr);
			return 1;
		}

		socklen_t len = sizeof(server);
		if (getsockname(fd, (struct sockaddr*)&server, &len) == 0)
			printf("Using port %u. Ensure you have ntp installed for the latest unix timestamp.\n", BE16TOH(server.sin_port));
	}

	puts( // source: toilet
		"┌────────────────────────────────────────────────────────────────┐\n"
		"│              [0;1;34;94m_[0m            [0;34m_____[0m                            [0;1;30;90m_[0m   │\n"
		"│    [0;1;34;94m/\\[0m       [0;34m|[0m [0;34m|[0m          [0;37m|_[0m   [0;37m_|[0m                          [0;1;34;94m|[0m [0;1;34;94m|[0m  │\n"
		"│   [0;34m/[0m  [0;34m\\[0m   [0;34m___|[0m [0;34m|[0;37m_[0m [0;37m_[0m [0;37m__[0m [0;37m___[0m  [0;37m|[0m [0;37m|[0m  [0;1;30;90m_[0m [0;1;30;90m__[0m [0;1;30;90m___[0m  [0;1;30;90m_[0m [0;1;30;90m__[0m   [0;1;34;94m__[0m [0;1;34;94m_[0m  [0;1;34;94m___|[0m [0;1;34;94m|_[0m │\n"
		"│  [0;34m/[0m [0;34m/\\[0m [0;34m\\[0m [0;37m/[0m [0;37m__|[0m [0;37m__|[0m [0;37m'__/[0m [0;1;30;90m_[0m [0;1;30;90m\\[0m [0;1;30;90m|[0m [0;1;30;90m|[0m [0;1;30;90m|[0m [0;1;30;90m'_[0m [0;1;30;90m`[0m [0;1;30;90m_[0m [0;1;34;94m\\|[0m [0;1;34;94m'_[0m [0;1;34;94m\\[0m [0;1;34;94m/[0m [0;1;34;94m_`[0m [0;1;34;94m|/[0m [0;34m__|[0m [0;34m__|[0m│\n"
		"│ [0;37m/[0m [0;37m____[0m [0;37m\\\\__[0m [0;37m\\[0m [0;37m|[0;1;30;90m_|[0m [0;1;30;90m|[0m [0;1;30;90m|[0m [0;1;30;90m(_)[0m [0;1;30;90m||[0m [0;1;30;90m|_[0;1;34;94m|[0m [0;1;34;94m|[0m [0;1;34;94m|[0m [0;1;34;94m|[0m [0;1;34;94m|[0m [0;1;34;94m|[0m [0;1;34;94m|_)[0m [0;34m|[0m [0;34m(_|[0m [0;34m|[0m [0;34m(__|[0m [0;34m|_[0m │\n"
		"│[0;37m/_/[0m    [0;37m\\[0;1;30;90m_\\___/\\__|_|[0m  [0;1;30;90m\\_[0;1;34;94m__/_____|_|[0m [0;1;34;94m|_|[0m [0;34m|_|[0m [0;34m.__/[0m [0;34m\\__,_|\\[0;37m___|\\__|[0m│\n"
		"│                                          [0;34m|[0m [0;34m|[0m                   │\n"
		"│                                          [0;37m|_|[0m                   │\n"
		"└────────────────────────────────────────────────────────────────┘"
	); // looks terrible like this yes

	uint8_t buffer[MAX_PACKET_SIZE];
	while (1) {
		// TODO: Resend stuff that should be reliable here

		struct sockaddr_in source;
		socklen_t source_len = sizeof(source);
		ssize_t msg_len = recvfrom(fd, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr*)&source, &source_len);
		if (msg_len < MIN_PACKET_SIZE) {
			fputs("Packet too small for any type!\n", stderr);
			continue;
		} else if (((uint64_t*)buffer)[0] != PROTOCOL_ID) {
			fputs("Invalid ID given!\n", stderr);
			continue;
		}

		uint64_t current_time = microtime();

		switch (buffer[8]) {
		case MSG_TYPE_PLAYER_POS:
			if (msg_len < 17+(4*12)) {
				fputs("Packet too small for its type! (player_pos)\n", stderr);
				break;
			}

			mutex_lock(&main_lock);

			uint64_t sent_time = LE64TOH(*((uint64_t*)(buffer+9)));

			for (uint64_t i = 0; i < num_clients; i++) {
				if (all_clients[i].address.sin_addr.s_addr == source.sin_addr.s_addr && all_clients[i].address.sin_port == source.sin_port && all_clients[i].valid) {
					mutex_lock(&(states[all_clients[i].state_index].state->lock));

					vec pos, vel;

					f32 scale = (f32)(int64_t)(states[all_clients[i].state_index].state->last_player_update - sent_time) * 0.000001f;

					pos.x = LE32FTOH(*((uint32_t*)(buffer+17)));
					pos.y = LE32FTOH(*((uint32_t*)(buffer+21)));
					pos.z = LE32FTOH(*((uint32_t*)(buffer+25)));

					vel.x = LE32FTOH(*((uint32_t*)(buffer+29)));
					vel.y = LE32FTOH(*((uint32_t*)(buffer+33)));
					vel.z = LE32FTOH(*((uint32_t*)(buffer+37)));

					pos.x += vel.x*scale;
					pos.y += vel.y*scale;
					pos.z += vel.z*scale;

					states[all_clients[i].state_index].state->players[all_clients[i].player_id].pos.x = pos.x;
					states[all_clients[i].state_index].state->players[all_clients[i].player_id].pos.y = pos.y;
					states[all_clients[i].state_index].state->players[all_clients[i].player_id].pos.z = pos.z;

					states[all_clients[i].state_index].state->players[all_clients[i].player_id].vel.x = vel.x;
					states[all_clients[i].state_index].state->players[all_clients[i].player_id].vel.y = vel.y;
					states[all_clients[i].state_index].state->players[all_clients[i].player_id].vel.z = vel.z;

					states[all_clients[i].state_index].state->players[all_clients[i].player_id].front.x = LE32FTOH(*((uint32_t*)(buffer+41)));
					states[all_clients[i].state_index].state->players[all_clients[i].player_id].front.y = LE32FTOH(*((uint32_t*)(buffer+45)));
					states[all_clients[i].state_index].state->players[all_clients[i].player_id].front.z = LE32FTOH(*((uint32_t*)(buffer+49)));

					states[all_clients[i].state_index].state->players[all_clients[i].player_id].up.x = LE32FTOH(*((uint32_t*)(buffer+53)));
					states[all_clients[i].state_index].state->players[all_clients[i].player_id].up.y = LE32FTOH(*((uint32_t*)(buffer+57)));
					states[all_clients[i].state_index].state->players[all_clients[i].player_id].up.z = LE32FTOH(*((uint32_t*)(buffer+61)));

					mutex_unlock(&(states[all_clients[i].state_index].state->lock));
					break;
				}
			}

			mutex_unlock(&main_lock);
			break;
		case MSG_TYPE_ASTEROID_DESTROYED:
			if (msg_len < 19) {
				fputs("Packet too small for its type! (asteroid_destroyed)\n", stderr);
				break;
			}

			mutex_lock(&main_lock);

			for (uint64_t i = 0; i < num_clients; i++) {
				if (all_clients[i].address.sin_addr.s_addr == source.sin_addr.s_addr && all_clients[i].address.sin_port == source.sin_port && all_clients[i].valid) {
					mutex_lock(&(states[all_clients[i].state_index].state->lock));

					uint64_t microtimestamp = LE64TOH(*((uint64_t*)(buffer+9)));
					uint16_t asteroid_id = LE16TOH(*((uint16_t*)(buffer+17)));
					if (asteroid_id > states[all_clients[i].state_index].state->max_asteroid_id) {
						fputs("Client attempted to destroy invalid asteroid!\n", stderr);
						goto skip_asteroid_destroyed;
					}

					struct asteroid *asteroid = &(states[all_clients[i].state_index].state->asteroids[asteroid_id]);

					uint8_t send_all;
					if (asteroid->gen_time < microtimestamp) {
						rand_asteroid(states[all_clients[i].state_index].state, asteroid, microtimestamp);
						if (asteroid->impact_time < states[all_clients[i].state_index].state->soonest_impact) {
							states[all_clients[i].state_index].state->soonest_impact = asteroid->impact_time;
							kill(states[all_clients[i].state_index].state->world_thread, SIGUSR1); // need to wake from sleep
						}
						send_all = 1;
					} else {
						send_all = 0;
					}

					buffer[8] = MSG_TYPE_ASTEROID_DESTROYED;
					*((uint64_t*)(buffer+9)) = HTOLE64(asteroid->gen_time);
					*((uint16_t*)(buffer+17)) = HTOLE16(asteroid_id);
					*((uint32_t*)(buffer+19)) = HTOLE32F(asteroid->vel.x);
					*((uint32_t*)(buffer+23)) = HTOLE32F(asteroid->vel.y);
					*((uint32_t*)(buffer+27)) = HTOLE32F(asteroid->vel.z);
					*((uint32_t*)(buffer+31)) = HTOLE32F(asteroid->start_pos.x);
					*((uint32_t*)(buffer+35)) = HTOLE32F(asteroid->start_pos.y);
					*((uint32_t*)(buffer+39)) = HTOLE32F(asteroid->start_pos.z);
					*((uint32_t*)(buffer+43)) = HTOLE32F(asteroid->rot);
					*((uint32_t*)(buffer+47)) = HTOLE32F(asteroid->scale);

					if (send_all) {
						for (uint16_t pid = 0; pid <= (uint16_t)states[all_clients[i].state_index].state->max_player_id; pid++) {
							if (states[all_clients[i].state_index].state->players[pid].valid)
								SLEND(fd, buffer, 51, 0, (struct sockaddr*)&states[all_clients[i].state_index].state->players[pid].address, sizeof(struct sockaddr_in));
						}

						memcpy(states[all_clients[i].state_index].state->reliable_data_asteroid[asteroid_id], buffer, 51);
						for (uint16_t pid = 0; pid <= (uint16_t)states[all_clients[i].state_index].state->max_player_id; pid++) {
							if (states[all_clients[i].state_index].state->players[pid].valid) {
								states[all_clients[i].state_index].state->reliable_timestamps_asteroid[asteroid_id][pid] = current_time;
								states[all_clients[i].state_index].state->reliable_attempts_asteroid[asteroid_id][pid] = 0;
								if (current_time + RESEND_TIME < states[all_clients[i].state_index].state->soonest_resend) {
									states[all_clients[i].state_index].state->soonest_resend = current_time + RESEND_TIME;
									kill(states[all_clients[i].state_index].state->resend_thread, SIGUSR1);
								}
							}
						}

						states[all_clients[i].state_index].state->reliable_timestamps_asteroid[asteroid_id][all_clients[i].player_id] = 0;
					} else {
						SLEND(fd, buffer, 51, 0, (struct sockaddr*)&source, source_len);
					}

					skip_asteroid_destroyed:
					mutex_unlock(&(states[all_clients[i].state_index].state->lock));
					break;
				}
			}

			mutex_unlock(&main_lock);
			break;
		case MSG_TYPE_PLAYER_DISCONNECTED:
			mutex_lock(&main_lock);

			buffer[8] = MSG_TYPE_PLAYER_DISCONNECTED;

			for (uint64_t i = 0; i < num_clients; i++) {
				if (all_clients[i].address.sin_addr.s_addr == source.sin_addr.s_addr && all_clients[i].address.sin_port == source.sin_port && all_clients[i].valid) {
					all_clients[i].valid = 0;
					buffer[9] = all_clients[i].player_id;

					mutex_lock(&(states[all_clients[i].state_index].state->lock));

					states[all_clients[i].state_index].state->players[all_clients[i].player_id].valid = 0;

					for (uint16_t pid = 0; pid <= (uint16_t)states[all_clients[i].state_index].state->max_player_id; pid++) {
						if (states[all_clients[i].state_index].state->players[pid].valid)
							SLEND(fd, buffer, 10, 0, (struct sockaddr*)&states[all_clients[i].state_index].state->players[pid].address, sizeof(struct sockaddr_in));
					}

					memcpy(states[all_clients[i].state_index].state->reliable_data_player[buffer[9]], buffer, 10);
					for (uint16_t pid = 0; pid <= (uint16_t)states[all_clients[i].state_index].state->max_player_id; pid++) {
						if (states[all_clients[i].state_index].state->players[pid].valid) {
							states[all_clients[i].state_index].state->reliable_timestamps_player[buffer[9]][pid] = current_time;
							states[all_clients[i].state_index].state->reliable_attempts_player[buffer[9]][pid] = 0;
							if (current_time + RESEND_TIME < states[all_clients[i].state_index].state->soonest_resend) {
								states[all_clients[i].state_index].state->soonest_resend = current_time + RESEND_TIME;
								kill(states[all_clients[i].state_index].state->resend_thread, SIGUSR1);
							}
						}
					}

					mutex_unlock(&(states[all_clients[i].state_index].state->lock));
					break;
				}
			}

			mutex_unlock(&main_lock);
			break;
		case MSG_TYPE_EXO_HIT_RECVD:
			if (msg_len < 11) {
				fputs("Packet too small for its type! (hit_recvd)\n", stderr);
				break;
			}

			mutex_lock(&main_lock);

			for (uint64_t i = 0; i < num_clients; i++) {
				if (all_clients[i].valid && all_clients[i].address.sin_addr.s_addr == source.sin_addr.s_addr && all_clients[i].address.sin_port == source.sin_port) {
					mutex_lock(&(states[all_clients[i].state_index].state->lock));

					uint16_t hit_id = LE16TOH(*((uint16_t*)(buffer+9)));
					states[all_clients[i].state_index].state->reliable_timestamps_exo[hit_id][all_clients[i].player_id] = 0;

					mutex_unlock(&(states[all_clients[i].state_index].state->lock));
					break;
				}
			}

			mutex_unlock(&main_lock);
			break;
		case MSG_TYPE_ASTEROID_DESTROYED_RECVD:
			if (msg_len < 11) {
				fputs("Packet too small for its type! (destroyed recvd)\n", stderr);
				break;
			}

			mutex_lock(&main_lock);

			for (uint64_t i = 0; i < num_clients; i++) {
				if (all_clients[i].valid && all_clients[i].address.sin_addr.s_addr == source.sin_addr.s_addr && all_clients[i].address.sin_port == source.sin_port) {
					mutex_lock(&(states[all_clients[i].state_index].state->lock));

					uint16_t asteroid_id = LE16TOH(*((uint16_t*)(buffer+9)));
					states[all_clients[i].state_index].state->reliable_timestamps_asteroid[asteroid_id][all_clients[i].player_id] = 0;

					mutex_unlock(&(states[all_clients[i].state_index].state->lock));
					break;
				}
			}

			mutex_unlock(&main_lock);
			break;
		case MSG_TYPE_PLAYER_DISCONNECTED_RECVD:
			if (msg_len < 10) {
				fputs("Packet too small for its type! (disconnected_recvd)\n", stderr);
				break;
			}

			mutex_lock(&main_lock);

			for (uint64_t i = 0; i < num_clients; i++) {
				if (all_clients[i].valid && all_clients[i].address.sin_addr.s_addr == source.sin_addr.s_addr && all_clients[i].address.sin_port == source.sin_port) {
					mutex_lock(&(states[all_clients[i].state_index].state->lock));

					uint8_t player_id = buffer[9];
					states[all_clients[i].state_index].state->reliable_timestamps_player[player_id][all_clients[i].player_id] = 0;

					mutex_unlock(&(states[all_clients[i].state_index].state->lock));
					break;
				}
			}

			mutex_unlock(&main_lock);
			break;
		case MSG_TYPE_REGISTER:
			if (msg_len < 19) {
				fputs("Packet too small for its type! (register)\n", stderr);
				break;
			}

			uint64_t requested_epoch = LE64TOH(*((uint64_t*) &(buffer[9])));
			uint32_t num_asteroids = LE16TOH(*((uint16_t*) &(buffer[17])));
			if (num_asteroids == 0)
				num_asteroids = 0x10000;
			else if (num_asteroids < 0x10)
				goto register_error;

			if (current_time >= requested_epoch * 1000000) // TODO: cap the upper limit, not wait 5 years for a game?
				goto register_error;

			uint8_t found = 0;
			uint64_t client_index;
			uint64_t state_index;
			uint64_t player_index;
			uint64_t available = 0;

			mutex_lock(&main_lock);

			for (uint64_t i = 0; i < num_clients; i++) {
				if (all_clients[i].address.sin_addr.s_addr == source.sin_addr.s_addr && all_clients[i].address.sin_port == source.sin_port && all_clients[i].valid) {
					if (all_clients[i].epoch != requested_epoch) { // client is already registered with a different epoch...
						goto register_error_unlock;
					} else {
						state_index = all_clients[i].state_index;
						player_index = all_clients[i].player_id;
						goto send_id;
					}
				} else if (!all_clients[i].valid && available) {
					available = 1;
					client_index = i;
				}
			}

			if (available) {
				available = 0;
			} else {
				void *tmp = realloc(all_clients, sizeof(*all_clients) * (num_clients + 1));
				if (tmp == 0)
					goto register_error_unlock;

				all_clients = tmp;
				client_index = num_clients;
				num_clients++;
			}

			for (uint64_t i = 0; i < num_states; i++) {
				if (states[i].epoch == requested_epoch) {
					found = 1;
					state_index = i;
					break;
				} else if (states[i].epoch == 0 && !available) {
					available = 1;
					state_index = i;
				}
			}

			struct current_state *state;
			if (found) {
#ifdef PACKET_DUMP
				printf("\n@thinks found\n");
#endif
				available = 0;
				state = states[state_index].state;
				mutex_lock(&(state->lock));
				for (uint16_t i = 0; i <= (uint16_t) state->max_player_id; i++) {
					if (!state->players[i].valid) {
						available = 1;
						player_index = i;
						break;
					}
				}

				if (!available) {
					if (state->max_player_id == 0xFF) {
						goto register_error_unlock;
					}

					void *tmp = realloc(state->players, sizeof(*(state->players)) * ((size_t)state->max_player_id + 2));
					if (tmp == 0) {
						goto register_error_unlock;
					}
					state->players = tmp;

					state->max_player_id++;
					player_index = state->max_player_id;
				}
			} else {
				player_index = 0;
				if (!available) {
					void *tmp = realloc(states, sizeof(*states) * (num_states + 1));
					if (tmp == 0)
						goto register_error_unlock;

					states = tmp;
					state_index = num_states;
					num_states++;
				}

				state = malloc(sizeof(*state));
				if (state == 0) // AAAAAAA OOM
					goto register_error_unlock;

				states[state_index].state = state;
				states[state_index].epoch = requested_epoch;

				memset(state, 0, sizeof(*state));

				state->players = malloc(sizeof(*(state->players)));
				if (state->players == 0) {
					goto register_error_clear;
				}
				state->max_player_id = 0;

				state->asteroids = malloc(sizeof(*(state->asteroids)) * num_asteroids);
				if (state->asteroids == 0) {
					free(state->players);

					goto register_error_clear;
				}

				state->max_asteroid_id = num_asteroids - 1;
				memset(state->asteroids, 0, sizeof(*(state->asteroids)) * num_asteroids);
				state->damage = 0;

				state->damage_index = malloc(sizeof(*state->damage_index) * exo_numvert);
				if (state->damage_index == 0) {
					free(state->players);
					free(state->asteroids);

					goto register_error_clear;
				}
				memset(state->damage_index, 0, sizeof(*state->damage_index) * exo_numvert);

//				state->pods = malloc(sizeof(*state->pods) * (0x100 * MAX_PODS_PER_PLAYER + MAX_WAITING_PODS));
//				if (state->pods == 0) {
//					free(state->damage_index);
//					free(state->players);
//					free(state->asteroids);
//
//					goto register_error_clear;
//				}
//				memset(state->pods, 0, sizeof(*state->pods) * (0x100 * MAX_PODS_PER_PLAYER + MAX_WAITING_PODS));
//				state->max_pod_id = ((0x100 * MAX_PODS_PER_PLAYER + MAX_WAITING_PODS) - 1);

				state->epoch = requested_epoch;
				state->state_index = state_index;
#ifdef PACKET_DUMP
				printf("\n@passing epoch\n");
#endif
				mutex_lock(&(state->lock));

				state->world_stack = mmap(0, TOTAL_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
				if (state->world_stack == 0) {
					goto register_error_clear_all;
				}
				mprotect(state->world_stack, BLOCKED_STACK_SIZE, PROT_NONE);

				state->player_stack = mmap(0, TOTAL_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
				if (state->player_stack == 0) {
					munmap(state->world_stack, TOTAL_STACK_SIZE);
					goto register_error_clear_all;
				}
				mprotect(state->player_stack, BLOCKED_STACK_SIZE, PROT_NONE);

//				state->pod_stack = mmap(0, TOTAL_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
//				if (state->pod_stack == 0) {
//					munmap(state->world_stack, TOTAL_STACK_SIZE);
//					munmap(state->player_stack, TOTAL_STACK_SIZE);
//					goto register_error_clear_all;
//				}
//				mprotect(state->pod_stack, BLOCKED_STACK_SIZE, PROT_NONE);

				state->resend_stack = mmap(0, TOTAL_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
				if (state->resend_stack == 0) {
					munmap(state->world_stack, TOTAL_STACK_SIZE);
					munmap(state->player_stack, TOTAL_STACK_SIZE);
//					munmap(state->pod_stack, TOTAL_STACK_SIZE);
					goto register_error_clear_all;
				}
				mprotect(state->resend_stack, BLOCKED_STACK_SIZE, PROT_NONE);

				state->player_thread = clone(&main_loop_send_players, state->player_stack+TOTAL_STACK_SIZE, CLONE_FLAGS, state);
				state->resend_thread = clone(&main_loop_resend, state->resend_stack+TOTAL_STACK_SIZE, CLONE_FLAGS, state);
				state->world_thread = clone(&main_loop_world, state->world_stack+TOTAL_STACK_SIZE, CLONE_FLAGS, state); // just in case the impossible happens (and the world thread finishes to kill the others before they get spawned), spawn this one last
			}

			state->players[player_index].address.sin_family = AF_INET;
			state->players[player_index].address.sin_addr.s_addr = source.sin_addr.s_addr;
			state->players[player_index].address.sin_port = source.sin_port;
			state->players[player_index].client_index = client_index;
			state->players[player_index].pos.x = 0.f;
			state->players[player_index].pos.y = 0.f;
			state->players[player_index].pos.z = 0.f;
			state->players[player_index].vel.x = 0.f;
			state->players[player_index].vel.y = 0.f;
			state->players[player_index].vel.z = 0.f;
			state->players[player_index].valid = 1;

			all_clients[client_index].state_index = state_index;
			all_clients[client_index].address.sin_addr.s_addr = source.sin_addr.s_addr;
			all_clients[client_index].address.sin_port = source.sin_port;
			all_clients[client_index].player_id = player_index;
			all_clients[client_index].epoch = requested_epoch;
			all_clients[client_index].valid = 1;

			state->sun_pos = 0.f;

			mutex_unlock(&(state->lock));

			send_id:
			*((uint16_t*)(buffer+10)) = HTOLE16(states[state_index].state->max_asteroid_id + 1);

			mutex_unlock(&main_lock);
			buffer[8] = MSG_TYPE_REGISTER_ACCEPTED;
			buffer[9] = (uint8_t)player_index;
			SLEND(fd, buffer, 12, 0, (struct sockaddr*)&source, source_len);

			break;

			register_error_clear_all:
			// not really all (stacks untouched), TODO: rename

//			free(states[state_index].state->pods);
			free(states[state_index].state->damage_index);
			free(states[state_index].state->players);
			free(states[state_index].state->asteroids);

			register_error_clear:
			free(states[state_index].state);
			states[state_index].epoch = 0;

			register_error_unlock:
			mutex_unlock(&main_lock);

			register_error:
			buffer[8] = MSG_TYPE_BAD_REGISTER_VALUE;
			SLEND(fd, buffer, 9, 0, (struct sockaddr*)&source, source_len);
			break;

		default:
			fputs("Invalid message type!\n", stderr);
			break;
		}
	}

	return 0;
}
