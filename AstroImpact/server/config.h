#include "inc/protocol.h"

#define EXO_IMPACT_RADIUS 1.14f
#define EXO_TARGET_RADIUS 1.f // if you don't want asteroids scraping the edge, set this to somewhere less than EXO_IMPACT_RADIUS

#define MIN_PACKET_SIZE 9
#define MAX_PACKET_SIZE 512

#define PLAYER_SEND_UTIME 50000 // 20hz
#define ASTEROID_SEND_UTIME 1000000 // 1hz
#define RESEND_TIME 1000000 // 1hz
#define MAX_RESEND_ATTEMPTS (30 * 1000000 / RESEND_TIME) // 30s

#define USABLE_STACK_SIZE 128*4096	// 512K, there's not really much stack in usage with these threads
#define BLOCKED_STACK_SIZE 16*4096	// 64K, this is the area below the stack to map as PROT_NONE -
					// segfault if we run out rather than overflowing into whatever may have happened to be there originally

#define TOTAL_STACK_SIZE BLOCKED_STACK_SIZE + USABLE_STACK_SIZE

#define CLONE_FLAGS CLONE_FILES | CLONE_FS | CLONE_IO | CLONE_VM | SIGCHLD

#define PODS_PER_WORLD 480
#define POD_SPAWN_UTIME 60000000 // 1 per 60s
#define MAX_WAITING_PODS 20

#define POD_SPAWN_RADIUS 1.2f

#define POD_IMPACT_RADIUS 0.12f

#if MAX_WAITING_PODS + (0x100 * MAX_PODS_PER_PLAYER) > 0x10000
	#error Not enough valid pod ids for that value of MAX_WAITING_PODS
#endif

#define EXO_IMPACT_RADIUS_SQ (EXO_IMPACT_RADIUS * EXO_IMPACT_RADIUS)
#define POD_IMPACT_RADIUS_SQ (POD_IMPACT_RADIUS * POD_IMPACT_RADIUS)
