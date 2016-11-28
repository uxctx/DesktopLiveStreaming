#include "atomic_lock.h"

int app_atomic_trylock(app_atomic_lock_t * lt, long value)
{
	return (lt->lock == 0 &&
		 compare_and_swap(&lt->lock, 0, value));
	return 0;
}

void app_atomic_lock(app_atomic_lock_t * lt, long value)
{
	u_int n, i;

	for (;; ) {
		
		if (lt->lock == 0 && compare_and_swap(&lt->lock, 0, value)) {
			return;
		}
		for (n = 1; n < spin_num; n <<= 1) {

			for (i = 0; i < n; i++) {
				cpu_pause();
			}

			if (lt->lock == 0 && compare_and_swap(&lt->lock, 0, value)) {
				return;
			}
		}

		thread_yield();
	}
}

void app_atomic_unlock(app_atomic_lock_t * lt, long value)
{
	return compare_and_swap(&lt->lock, value, 0);
}
