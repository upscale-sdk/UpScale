/* Copyright 2014 DEI - Universita' di Bologna

author    DEI - Universita' di Bologna
          Alessandro Capotondi - alessandro.capotondi@unibo.it
          Giuseppe Tagliavini  - giuseppe.tagliavini@unibo.it
          Andrea Marongiu      - amarongiu@unibo.it
          
info      gomp management */

#include "omp-lock.h"
#include "appsupport.h"
#include "config.h"

/* gomp_hal_lock() - block until able to acquire lock "id" */
ALWAYS_INLINE void gomp_hal_lock(omp_lock_t *lock) {
  SpinLockObj((SpinlockObjType *) lock);
}

/* gomp_hal_lock() - release lock "id" */
ALWAYS_INLINE void gomp_hal_unlock(omp_lock_t *lock) {
  SpinUnlockObj((SpinlockObjType *) lock);
}

void gomp_hal_init_locks(int offset) {
}

/* gomp_hal_init_lock () - get a lock */
void gomp_hal_init_lock(omp_lock_t *lock) {
  EE_k1_spin_init_lock(lock);
}

/* gomp_hal_destroy_lock () - destroys a lock */
void gomp_hal_destroy_lock(omp_lock_t *lock) {
  //free(lock);
}

/*********************************** standard APIs ***********************************************/

void omp_set_lock(omp_lock_t *lock) {
	gomp_hal_lock(lock);
}

void omp_unset_lock(omp_lock_t *lock) {
	gomp_hal_unlock(lock);
}

void omp_init_lock(omp_lock_t *lock) {
	gomp_hal_init_lock(lock);
}

void omp_destroy_lock(omp_lock_t *lock) {
	gomp_hal_destroy_lock(lock);
}

