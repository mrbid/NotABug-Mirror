#include <math.h>
#include "protocol.h"

uint32_t HTOLE32F(f32 f) {
	int exp;
	f32 fm = frexpf(f, &exp);
	uint32_t m = (uint32_t)ldexpf(fm, 24);
	return 0;
}

f32 LE32FTOH(uint32_t i) {
	return 0.f;
}
