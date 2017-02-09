#ifndef EE_K1_BSP_H_
#define EE_K1_BSP_H_

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


/* Macro IDs numbers and boot */
#define EE_K1_PE_NUMBER   _K1_NB_PE
#define EE_K1_RM_NUMBER   _K1_NB_RM
#define EE_K1_CORE_NUMBER (EE_K1_PE_NUMBER + EE_K1_RM_NUMBER)
#define EE_K1_BOOT_CORE    EE_K1_PE_NUMBER
#define EE_K1_MAIN_CORE   (0U)

/* Read Memory barrier */
EE_INLINE__ void EE_k1_rmb ( void ) {
  __k1_rmb();
}

/* Write Memory Barrier */
EE_INLINE__ void EE_k1_wmb ( void ) {
  __k1_wmb();
}

/* Full Memory Barrier */
EE_INLINE__ void EE_k1_mb ( void ) {
  __k1_mb();
}

EE_INLINE__ void EE_k1_idle_enter ( void ) {
  __k1_idle_enter();
}

typedef __k1_spinlock_t EE_k1_spinlock;
typedef EE_k1_spinlock  EE_spin_lock;

#define EE_K1_SPIN_UNLOCKED _K1_SPIN_UNLOCKED

EE_INLINE__ void EE_k1_spin_init_lock ( EE_k1_spinlock * p_lock ) {
  __k1_spin_initlock(p_lock);
}

EE_INLINE__ EE_BOOL EE_k1_spin_is_locked ( EE_k1_spinlock * p_lock ) {
 return __k1_spin_is_locked(p_lock);
}

EE_INLINE__ void EE_k1_spin_lock ( EE_k1_spinlock * p_lock ) {
  EE_k1_rmb();
  __k1_spin_lock(p_lock);
}

EE_INLINE__ EE_BOOL EE_k1_spin_trylock ( EE_k1_spinlock * p_lock ) {
  EE_BOOL accessed = __k1_spin_trylock(p_lock);
  if ( accessed ) {
    EE_k1_rmb();
  }
  return accessed;
}

EE_INLINE__ void EE_k1_spin_unlock ( EE_k1_spinlock * p_lock ) {
  EE_k1_wmb();
  __k1_spin_unlock(p_lock);
}

#if 0
typedef struct EE_k1_reentrant_spinlock_tag {
  EE_k1_spinlock lock;
  EE_UREG        counter;
} EE_k1_reentrant_spinlock;

EE_INLINE__ void EE_k1_reentrant_spin_lock ( EE_k1_reentrant_spinlock * p_lock )
{
  EE_UREG local_counter = __k1_umem_read32(&p_lock->counter);
  if ( local_counter == 0U ) {
    __builtin_k1_wpurge();
    EE_k1_spin_lock(&p_lock->lock);
  }
  __k1_umem_write32(&p_lock->counter, ++local_counter);
}

EE_INLINE__ void EE_k1_reentrant_spin_unlock
  ( EE_k1_reentrant_spinlock * p_lock )
{
  EE_UREG local_counter = __k1_umem_read32(&p_lock->counter);
  if ( local_counter == 1U ) {
    __builtin_k1_wpurge();
    __k1_umem_write32(&p_lock->counter, 0);
    EE_k1_spin_lock(&p_lock->lock);
  } else if ( local_counter > 0U ) {
    __k1_umem_write32(&p_lock->counter, --local_counter);
  }
}
#endif /* 0 */
#endif /* !EE_K1_BOOT_H_ */
