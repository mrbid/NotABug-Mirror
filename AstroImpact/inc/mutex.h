#include <unistd.h>
#include <sys/syscall.h>
#include <stdint.h>
#include <linux/futex.h>

static inline void mutex_lock(uint32_t *futex) {
	uint32_t val;
	while (val = __sync_lock_test_and_set(futex, 1))
		syscall(SYS_futex, futex, FUTEX_WAIT, val, 0, 0, 0);
}

static inline void mutex_unlock(uint32_t *futex) {
	__sync_lock_release(futex);
	syscall(SYS_futex, futex, FUTEX_WAKE, 1, 0, 0, 0);
}
