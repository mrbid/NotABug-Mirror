#pragma once

#include <endian.h>
#include <stdint.h>

#include "inc/gl.h"
#define f32 GLfloat

#if defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN
	#define SERVER_PORT 0x961F
	#define PROTOCOL_ID 0x75F6073677E10C44

	#define HTOLE16 // no htole* called when we're already little-endian
	#define HTOLE32
	#define HTOLE64

	#define LE16TOH // no letoh*
	#define LE32TOH
	#define LE64TOH

	#define HTOBE16 htobe16
	#define HTOBE32 htobe32
	#define HTOBE64 htobe64

	#define BE16TOH be16toh
	#define BE32TOH be32toh
	#define BE64TOH be64toh

	static inline uint32_t HTOLE32F(f32 x) {
		return *((uint32_t*)(&x));
	}

	static inline f32 LE32FTOH(uint32_t x) {
		return *((f32*)(&x));
	}
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN
	#define SERVER_PORT 0x1F96
	#define PROTOCOL_ID 0x440CE1773607F675

	#define HTOLE16 htole16
	#define HTOLE32 htole32
	#define HTOLE64 htole64

	#define LE16TOH le16toh
	#define LE32TOH le32toh
	#define LE64TOH le64toh

	#define HTOBE16 // no htobe*
	#define HTOBE32
	#define HTOBE64

	#define BE16TOH // no betoh*
	#define BE32TOH
	#define BE64TOH

	static inline uint32_t HTOLE32F(f32 x) {
		uint32_t y = *((uint32_t*)(&x));
		return HTOLE32(y);
	}

	static inline f32 LE32FTOH(uint32_t x) {
		x = LE32TOH(x);
		return *((f32*)(&x));
	}
#else
	#define SERVER_PORT htobe16(0x1F96)
	#define PROTOCOL_ID htole64(0x75F6073677E10C44)

	#define HTOLE16 htole16
	#define HTOLE32 htole32
	#define HTOLE64 htole64

	#define LE16TOH le16toh
	#define LE32TOH le32toh
	#define LE64TOH le64toh

	#define HTOBE16 htobe16
	#define HTOBE32 htobe32
	#define HTOBE64 htobe64

	#define BE16TOH be16toh
	#define BE32TOH be32toh
	#define BE64TOH be64toh

	static inline uint32_t HTOLE32F(f32 x) {
		uint32_t y = *((uint32_t*)(&x));
		return HTOLE32(y);
	}

	static inline f32 LE32FTOH(uint32_t x) {
		x = LE32TOH(x);
		return *((f32*)(&x));
	}
#endif

//uint32_t HTOLE32F(f32 f);
//f32 LE32FTOH(uint32_t i);
#define MSG_TYPE_ASTEROID_POS (uint8_t) 0x00

#define MSG_TYPE_PLAYER_POS (uint8_t) 0x01

#define MSG_TYPE_SUN_POS (uint8_t) 0x02
#define MSG_SIZE_SUN_POS (uint16_t) 21

#define MSG_TYPE_ASTEROID_DESTROYED (uint8_t) 0x03

#define MSG_TYPE_EXO_HIT (uint8_t) 0x04
#define MSG_SIZE_EXO_HIT (uint16_t) 89

#define MSG_TYPE_PLAYER_DISCONNECTED (uint8_t) 0x05

#define MSG_TYPE_POD_STATE_CHANGED (uint8_t) 0x06
#define POD_STATE_DESTROYED 0
#define POD_STATE_SPAWNED 1
#define POD_STATE_DELIVERED 2
#define POD_STATE_PICKEDUP 3


#define MSG_TYPE_EXO_HIT_RECVD (uint8_t) 0x80

#define MSG_TYPE_ASTEROID_DESTROYED_RECVD (uint8_t) 0x81

#define MSG_TYPE_PLAYER_DISCONNECTED_RECVD (uint8_t) 0x82

#define MSG_TYPE_POD_STATE_CHANGED_RECVD (uint8_t) 0x83


#define MSG_TYPE_REGISTER_ACCEPTED (uint8_t) 0xC0

#define MSG_TYPE_BAD_REGISTER_VALUE (uint8_t) 0xFE

#define MSG_TYPE_REGISTER (uint8_t) 0xFF



#define MAX_PODS_PER_PLAYER 6
