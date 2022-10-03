#ifndef GOLIBSASS_MUTEX_H
#define GOLIBSASS_MUTEX_H

/**
 * Platform-independent-ish implementation of a read/write mutex
 * 
 * Used to lock the resolution cache
 */

#ifdef _WIN32
#include <windows.h>
#include <synchapi.h>
typedef struct {
    SRWLOCK lock;
} golibsass_rwmutex;
#else
# include <pthread.h>
typedef struct {
    pthread_rwlock_t lock;
} golibsass_rwmutex;
#endif

int golibsass_rwmutex_init(golibsass_rwmutex *rwlock);
int golibsass_rwmutex_destroy(golibsass_rwmutex *rwlock);
int golibsass_rwmutex_rdlock(golibsass_rwmutex *rwlock);
int golibsass_rwmutex_wrlock(golibsass_rwmutex *rwlock);
int golibsass_rwmutex_rdunlock(golibsass_rwmutex *rwlock);
int golibsass_rwmutex_wrunlock(golibsass_rwmutex *rwlock);

#endif