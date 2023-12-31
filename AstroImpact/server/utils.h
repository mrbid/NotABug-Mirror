#pragma once

#include <stdint.h>

#include "types.h"
#include "inc/vec.h"

uint64_t microtime(void);
void do_exo_impact(struct current_state *state, vec pos, float f);
void rand_asteroid(struct current_state *state, struct asteroid *asteroid, uint64_t timestamp);
void rand_all_asteroids(struct current_state *state, uint64_t timestamp);

static inline f32 distance_to_impact(vec origin, vec direction, f32 radius_sq) {
	f32 a = vMag(direction);
	f32 b = 2.f*vDot(direction, origin);
	f32 c = vMag(origin) - radius_sq;

	f32 dis = b*b - 4.f*a*c;
	if (dis < 0.f) // this function assumes impact will occur, blame on float inaccuracies
		dis = 0.f;

	return (-b - sqrtps(dis))/(2.f*a); // negative root for near end
}

static inline uint64_t time_till_impact(vec origin, vec direction, f32 speed, f32 radius) {
	return (uint64_t)(distance_to_impact(origin, direction, radius) * 1000000.f / speed);
}

static inline uint8_t will_impact(vec origin, vec direction, f32 radius_sq) {
	// iirc there's more efficient ways of doing this, but this works for now
	// TODO: fix ^
	f32 a = vMag(direction);
	f32 b = 2.f*vDot(direction, origin);
	f32 c = vMag(origin) - radius_sq;

	f32 dis = b*b - 4.f*a*c;

	return dis >= 0.f;
}
