#pragma once
#ifndef _atomic_lock_h_include_
#define _atomic_lock_h_include_

#include <windows.h>

#define cpu_pause()  __asm {pause}
#define thread_yield()	     Yield()

#define spin_num (2048)

typedef struct {
	volatile long   lock;
} app_atomic_lock_t;

__forceinline int compare_and_swap(long volatile *des, long out, long set)
{
	InterlockedCompareExchange(des, set, out);
	return *des == set;
}

int app_atomic_trylock(app_atomic_lock_t* lt, long value);

void app_atomic_lock(app_atomic_lock_t* lt, long value);

void app_atomic_unlock(app_atomic_lock_t* lt, long value);
#endif

