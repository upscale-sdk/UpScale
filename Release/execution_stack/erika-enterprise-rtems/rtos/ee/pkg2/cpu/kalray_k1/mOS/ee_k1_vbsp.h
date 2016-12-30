#ifndef EE_K1_VBSP_H_
#define EE_K1_VBSP_H_

#include <mOS_common_types_c.h>
#include <mOS_vcore_u.h>
#include <HAL/hal/hal.h>

/* Include the full Kalray HAL support for K1 */
#include "eecfg.h"
#include "ee_compiler_gcc.h"
#include "ee_platform_types.h"

#ifdef EE_TICKED_LOCKS
#define __K1_SPINLOCK_SET_TICKET
#else
#define __K1_SPINLOCK_SET_BACKOFF
#endif
#include <mppa_bsp.h>

#ifdef EE_TRACE_KERNEL
#define MPPA_TRACE_ENABLE
#define MPPA_TRACEPOINT_DEFINE
#include "ee_k1_mppa_trace.h"
#define MPPA_CREATE_TRACEPOINT
#include <mppa_trace.h>
#else
#include <mppa_trace.h>
#endif /* EE_TRACE_KERNEL */

/* #define EE_K1_FULL_PREEMPTION */

 /* __builtin_k1_wpurge(): purge the data cache write buffer (for cached stores)
  */
 /* __builtin_k1_fence(): wait until all on fly write are commit to the SMEM.
    It must be called if you want to make sure that your write buffer finished
    commit data in the SMEM.
    It must also be called if you want to make sure that all on fly uncached
    stores are commit  in the SMEM */

/* mOS_dinval(): invalidate the data cache */

/* Read Memory barrier */
EE_INLINE__ void EE_k1_rmb ( void ) {
  /* invalidate the data cache */
  mOS_dinval();
}

/* Write Memory Barrier */
EE_INLINE__ void EE_k1_wmb ( void ) {
  /* purge the data cache write buffer (for cached stores) */
  __builtin_k1_wpurge();
  /* wait until all on fly write are commit to the SMEM. */
  __builtin_k1_fence();
}

/* Full Memory Barrier */
EE_INLINE__ void EE_k1_mb ( void ) {
  EE_k1_rmb ();
  EE_k1_wmb ();
}

EE_INLINE__ void EE_k1_idle_enter ( void ) {
  mOS_idle1();
}

EE_INLINE__ void EE_k1_spin_init_lock ( EE_k1_spinlock * p_lock ) {
  __k1_fspinlock_init(p_lock);
}

EE_INLINE__ EE_BOOL EE_k1_spin_is_locked ( EE_k1_spinlock * p_lock ) {
  return __k1_fspinlock_is_locked(p_lock);
}

EE_INLINE__ void EE_k1_spin_lock ( EE_k1_spinlock * p_lock ) {
  EE_k1_rmb();
  __k1_fspinlock_lock(p_lock);
}

EE_INLINE__ EE_BOOL EE_k1_spin_trylock ( EE_k1_spinlock * p_lock ) {
  EE_BOOL accessed = __k1_fspinlock_trylock(p_lock);
  if ( accessed ) {
    EE_k1_rmb();
  }
  return accessed;
}

EE_INLINE__ void EE_k1_spin_unlock ( EE_k1_spinlock * p_lock ) {
  EE_k1_wmb();
  __k1_fspinlock_unlock(p_lock);
}

#endif /* !EE_K1_VBSP_H_ */
