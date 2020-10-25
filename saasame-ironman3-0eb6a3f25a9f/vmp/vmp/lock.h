
#ifndef _LOCK_
#define _LOCK_

#include "ntddk.h"

typedef struct{
    KSPIN_LOCK data;
    KIRQL irql;
    LONG count;
    PETHREAD thread;
} spin_lock;

static void __inline lock_init(spin_lock *lock){
    KeInitializeSpinLock(&lock->data);
    lock->count = 0;
}

static void __inline lock_acquire(spin_lock *lock){
    KIRQL       kirql;
    PETHREAD    current_thread = NULL;
    if (lock){
        current_thread = PsGetCurrentThread();     // Thread handle
       
        if (current_thread == lock->thread){
            InterlockedIncrement(&lock->count);
        }
        else{
            KeAcquireSpinLock(&lock->data, &kirql);
            lock->irql = kirql;
            lock->thread = current_thread;
            InterlockedExchange(&lock->count, 1);
        }
    }
}

static void __inline lock_release(spin_lock *lock)
{
    KIRQL       kirql;
    PETHREAD    current_thread = NULL;

    if (lock){
        current_thread = PsGetCurrentThread();
        if (current_thread != lock->thread){
            // Do nothing when the lock thread different with current thread
        }
        else{
            if (InterlockedDecrement(&lock->count) == 0){              
                lock->thread = NULL;
                kirql = lock->irql;
                KeReleaseSpinLock(&lock->data, kirql);
            }
        }
    }
}
#endif