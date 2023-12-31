#pragma once

#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "inc/gl.h"
#include "inc/vec.h"
#include "inc/protocol.h"

#include "config.h"

#define f32 GLfloat

struct asteroid {
	vec vel, start_pos, impact_pos;
	f32 rot, scale;
	f32 speed;
	uint64_t gen_time;
	uint64_t impact_time;
//	uint64_t final_impact_time;
//	uint8_t final_impact;
//	uint16_t pod_id; // only active if final_impact == 0
};

//struct pod {
//	uint8_t state;
//	uint8_t player_id; // only valid if state == POD_STATE_PICKEDUP
//	uint64_t saved; // state->saved += pod[id].saved
//	vec pos;
//};

struct player {
	uint8_t valid;
	vec vel, pos;
	vec front, up;
	struct sockaddr_in address;
	uint64_t client_index;
//	struct pod pods[MAX_PODS_PER_PLAYER];
};

struct current_state {
	struct asteroid *asteroids;
	struct player *players;
	uint16_t max_asteroid_id;
	uint8_t max_player_id;
	f32 sun_pos;
//	struct pod *pods;
//	uint16_t max_pod_id;

//	uint64_t saved;
//	uint64_t pods_waiting;
//	uint16_t current_pod_index;
	uint64_t damage;
	uint8_t *damage_index;
	uint64_t epoch;
	uint64_t state_index;
	uint32_t lock;

	pid_t world_thread;
	pid_t player_thread;
//	pid_t pod_thread;
	pid_t resend_thread;

	uint8_t *world_stack;
	uint8_t *player_stack;
//	uint8_t *pod_stack;
	uint8_t *resend_stack;

	uint64_t last_player_update;

	uint64_t soonest_impact;

	uint8_t reliable_data_asteroid[65536][51];
	uint64_t reliable_timestamps_asteroid[65536][256];
	uint8_t reliable_attempts_asteroid[65536][256];

	uint8_t reliable_data_exo[65536][MSG_SIZE_EXO_HIT];
	uint64_t reliable_timestamps_exo[65536][256];
	uint8_t reliable_attempts_exo[65536][256];

	uint8_t reliable_data_player[256][10];
	uint64_t reliable_timestamps_player[256][256];
	uint8_t reliable_attempts_player[256][256];

	uint64_t soonest_resend;
};

struct state_holder {
	uint64_t epoch;
	struct current_state *state;
};

struct client_holder {
	struct sockaddr_in address;
	uint64_t epoch;
	uint8_t valid;
	uint64_t state_index;
	uint8_t player_id;
};
