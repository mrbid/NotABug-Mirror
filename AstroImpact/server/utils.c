#include <sys/time.h>
#include <stdint.h>

#include <stdio.h>

#include "main.h"
#include "utils.h"
#include "types.h"
#include "assets/models.h"
#include "inc/vec.h"
#include "config.h"

uint64_t microtime(void) {
	struct timeval tv;
	struct timezone tz = {0};

	gettimeofday(&tv, &tz);
	return (tv.tv_sec * 1000000) + tv.tv_usec;
}

void do_exo_impact(struct current_state *state, vec pos, float f) {
	for (GLsizeiptr i = 0; i < exo_numvert; i++) {
		if (!state->damage_index[i]) {
			vec v = {exo_vertices[i*3], exo_vertices[(i*3)+1], exo_vertices[(i*3)+2]};
			if (vDistSq(v, pos) < f*f) {
				state->damage++;
				state->damage_index[i] = 1;
			}
		}
	}

	return;
}

//void rand_asteroid(struct asteroid *asteroid) {
//	vRuvBT(&(asteroid->pos));
//	vMulS(&(asteroid->pos), asteroid->pos, 10.f);
//
//	vec dp;
//	vRuvTA(&dp);
//	vSub(&(asteroid->vel), asteroid->pos, dp);
//	vNorm(&(asteroid->vel));
//	vInv(&(asteroid->vel));
//
//	vec of = asteroid->vel;
//	vInv(&of);
//	vMulS(&of, of, randf() * 6.f);
//	vAdd(&(asteroid->pos), asteroid->pos, of);
//
//	asteroid->rot = randf() * 300.f;
//	asteroid->scale = 0.01f + (randf() * 0.07f);
//	f32 speed = 0.16f + (randf() * 0.08f);
//	asteroid->vel.x = asteroid->vel.x * speed;
//	asteroid->vel.y = asteroid->vel.y * speed;
//	asteroid->vel.z = asteroid->vel.z * speed;
//}

void rand_asteroid(struct current_state *state, struct asteroid *asteroid, uint64_t timestamp) {
	asteroid->gen_time = timestamp;

	asteroid->rot = randf() * 300.f;
	asteroid->scale = 0.01f + (randf() * 0.07f);
	f32 speed = 0.16f + (randf() * 0.08f);

	vRuv(&asteroid->start_pos);
	while (vMag(asteroid->start_pos) == 0.f) // can't use null vectors
		vRuv(&asteroid->start_pos);

	vNorm(&asteroid->start_pos);

	f32 dist = randfc()+10.f;
	asteroid->start_pos.x *= dist;
	asteroid->start_pos.y *= dist;
	asteroid->start_pos.z *= dist;

	vec impactpos;
	vRuv(&impactpos);
	while (vMag(impactpos) == 0.f)
		vRuv(&impactpos);

	vNorm(&impactpos);
	impactpos.x *= EXO_TARGET_RADIUS;
	impactpos.y *= EXO_TARGET_RADIUS;
	impactpos.z *= EXO_TARGET_RADIUS;

	asteroid->vel = (vec){impactpos.x - asteroid->start_pos.x, impactpos.y - asteroid->start_pos.y, impactpos.z - asteroid->start_pos.z};

	vNorm(&asteroid->vel);

	f32 dist_to_impact = distance_to_impact(asteroid->start_pos, asteroid->vel, EXO_IMPACT_RADIUS_SQ);
	asteroid->impact_pos = (vec){
		asteroid->start_pos.x + asteroid->vel.x*dist_to_impact,
		asteroid->start_pos.y + asteroid->vel.y*dist_to_impact,
		asteroid->start_pos.z + asteroid->vel.z*dist_to_impact
	};

	asteroid->vel.x *= speed;
	asteroid->vel.y *= speed;
	asteroid->vel.z *= speed;

	speed = vMod(asteroid->vel);
	asteroid->impact_time = (uint64_t)(dist_to_impact * 1000000.f / speed);
//	asteroid->final_impact_time = asteroid->impact_time;

	asteroid->speed = speed;

//	asteroid->final_impact = 1;
//	for (uint32_t i = 0; i <= (uint32_t)state->max_pod_id; i++) {
//		struct pod *pod = &(state->pods[i]);
//		if (pod->state == POD_STATE_SPAWNED) {
//			vec offset;
//			vSub(&offset, asteroid->start_pos, pod->pos);
//			if (will_impact(offset, asteroid->vel, POD_IMPACT_RADIUS_SQ)) {
//				uint64_t t = time_till_impact(offset, asteroid->vel, speed, POD_IMPACT_RADIUS_SQ);
//				if (t < asteroid->impact_time) {
//					asteroid->impact_time = t;
//					asteroid->final_impact = 0;
//				}
//			}
//		}
//	}

	asteroid->impact_time += timestamp;
}

void rand_all_asteroids(struct current_state *state, uint64_t timestamp) {
	for (uint32_t i = 0; i <= (uint32_t)state->max_asteroid_id; i++)
		rand_asteroid(state, &(state->asteroids[i]), timestamp);
}
