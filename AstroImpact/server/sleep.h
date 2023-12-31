#include <stdint.h>

void usleep2(uint64_t *microtimestamp, uint64_t current_time); // pointer so it can be chosen as to when to read
							       // current_time bc I don't want to bother doing that in assembly

void usleep2_start(void);
void usleep2_end(void);
