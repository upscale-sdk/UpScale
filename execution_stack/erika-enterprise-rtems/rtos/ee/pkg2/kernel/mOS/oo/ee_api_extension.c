/*
 * ee_api_extension.c
 *
 *  Created on: 08/dic/2014
 *      Author: AmministratoreErrico
 */

#include "ee_api_extension.h"
#include "ee_scheduler.h"
#include "ee_kernel_types.h"
#include "ee_get_kernel_and_core.h"

/* TODO: Add spinlock abstraction */

void InitSem ( SemRefType pSem, CountType count )
{
  if ( pSem != NULL ) {
    EE_k1_spin_init_lock(&pSem->lock);
    pSem->p_blocked_queue = NULL;
    pSem->count = count;
  }
}

StatusType WaitSem ( SemRefType Sem ) {
  /* Error Value */
  StatusType  status_type;

  if ( Sem == NULL ) {
    status_type = E_OS_PARAM_POINTER;
  } else {
    EE_KDB EE_CONST * const p_KDB   = EE_get_kernel();
    EE_CDB EE_CONST * const p_CDB   = EE_get_curr_core();
    EE_TDB EE_CONST * const p_curr  = p_CDB->p_CCB->p_curr;
    EE_FREG           const flags   = EE_hal_begin_nested_primitive();

    if ( p_curr->task_type != EE_TASK_TYPE_EXTENDED ) {
      status_type = E_OS_ACCESS;
    } else {
      EE_k1_spin_lock( &Sem->lock );
      if ( Sem->count != 0U ) {
        --Sem->count;
        EE_k1_spin_unlock( &Sem->lock );
      } else {
        /* Prepare to put the TASK inside the semaphore queue...
         * It will be really done in EE_scheduler_task_wrapper calling
         * EE_scheduler_task_saving_available */
        EE_TDB EE_CONST * const p_To = EE_scheduler_task_block_current(p_KDB,
          p_CDB, &Sem->p_blocked_queue);

#if (defined(EE_SCHEDULER_GLOBAL)) && (!defined(EE_SINGLECORE))
        p_CDB->p_CCB->p_lock_to_be_released = &Sem->lock;
#else
        EE_k1_spin_unlock( &Sem->lock );
#endif

        EE_hal_change_context(&p_To->HDB, p_To->task_func, &p_curr->HDB);
      }

      status_type = E_OK;
    }
    EE_hal_end_nested_primitive(flags);
  }
  return status_type;
}

StatusType PostSem (SemRefType Sem) {
  /* Error Value */
  StatusType  status_type;
  if ( Sem == NULL ) {
    status_type = E_OS_PARAM_POINTER;
  } else {
    EE_KDB EE_CONST * const p_KDB = EE_get_kernel();
    EE_CDB EE_CONST * const p_CDB = EE_get_curr_core();
    EE_FREG           const flags = EE_hal_begin_nested_primitive();
    /* check if the post on the semaphore wakes up someone */
    EE_k1_spin_lock( &Sem->lock );

    if ( Sem->p_blocked_queue != NULL ) {
      EE_BOOL  const is_preemption = EE_scheduler_task_unblocked(p_KDB, p_CDB,
        Sem->p_blocked_queue);

      /* Remove the head from the blocking queue */
      Sem->p_blocked_queue = Sem->p_blocked_queue->p_next;

      EE_k1_spin_unlock( &Sem->lock );

      if ( is_preemption ) {
        (void)EE_scheduler_task_preemption_point(p_KDB, p_CDB);
      }
    } else {
      ++Sem->count;
      EE_k1_spin_unlock( &Sem->lock );
    }
    EE_hal_end_nested_primitive(flags);
    status_type = E_OK;
  }

  return status_type;
}


