#include "rwmutex.h"

#ifdef _WIN32
#include <windows.h>
#include <synchapi.h>

// Transparent wrappper for InitializeSRWLock
int golibsass_rwmutex_init(golibsass_rwmutex *rwlock) {
    InitializeSRWLock(
        &(rwlock->lock)
    );
    return 0;
}

// noop in windows
int golibsass_rwmutex_destroy(golibsass_rwmutex *rwlock) {
    // destory is a noop so long as the lock is not currently
    // acquoired
    return 0;
}

// Transparent wrappper for AcquireSRWLockShared
int golibsass_rwmutex_rdlock(golibsass_rwmutex *rwlock) {
    AcquireSRWLockShared(
        &(rwlock->lock)
    );
    return 0;
}

// Transparent wrappper for AcquireSRWLockExclusive
int golibsass_rwmutex_wrlock(golibsass_rwmutex *rwlock) {
    AcquireSRWLockExclusive(
        &(rwlock->lock)
    );
    return 0;
}

// Transparent wrappper for ReleaseSRWLockShared
int golibsass_rwmutex_rdunlock(golibsass_rwmutex *rwlock) {
    ReleaseSRWLockShared(
        &(rwlock->lock)
    );
    return 0;
}

// Transparent wrappper for ReleaseSRWLockExclusive
int golibsass_rwmutex_wrunlock(golibsass_rwmutex *rwlock) {
    ReleaseSRWLockExclusive(
        &(rwlock->lock)
    );
    return 0;
}

#else
// Transparent wrappper for pthread_rwlock_init(lock, NULL)
int golibsass_rwmutex_init(golibsass_rwmutex *rwlock){
    return pthread_rwlock_init(
        &(rwlock->lock),
        NULL
    );
}

// Transparent wrappper for pthread_rwlock_destroy
int golibsass_rwmutex_destroy(golibsass_rwmutex *rwlock){
    return pthread_rwlock_destroy(
        &(rwlock->lock)
    );
}

// Transparent wrappper for pthread_rwlock_rdlock
int golibsass_rwmutex_rdlock(golibsass_rwmutex *rwlock){
    return pthread_rwlock_rdlock(
        &(rwlock->lock)
    );
}

// Transparent wrappper for pthread_rwlock_wrlock
int golibsass_rwmutex_wrlock(golibsass_rwmutex *rwlock){
    return pthread_rwlock_wrlock(
        &(rwlock->lock)
    );
}

// Transparent wrappper for pthread_rwlock_unlock
int golibsass_rwmutex_rdunlock(golibsass_rwmutex *rwlock){
    return pthread_rwlock_unlock(
        &(rwlock->lock)
    );
}

// Transparent wrappper for pthread_rwlock_unlock
int golibsass_rwmutex_wrunlock(golibsass_rwmutex *rwlock){
    return pthread_rwlock_unlock(
        &(rwlock->lock)
    );
}

#endif