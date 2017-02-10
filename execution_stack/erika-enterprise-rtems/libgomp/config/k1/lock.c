/* Copyright (C) 2010, 2017 DEI - Universita' di Bologna
   Contributed by:
   Alessandro Capotondi <alessandro.capotondi@unibo.it>
   Daniele Cesarini <daniele.cesarini@unibo.it>
   Andrea Marongiu  <a.marongiu@unibo.it>
   Giuseppe Tagliavini <giuseppe.tagliavini@unibo.it>
*/

/* This file is part of the GNU OpenMP Library (libgomp).

   Libgomp is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   Libgomp is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */
          
/*	This file includes lock management functions for the Kalray MPPA 256 BSP */

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

