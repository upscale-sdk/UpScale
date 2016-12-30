/*
 * ee_scheduler_entry_points.c
 *
 *  Created on: 07/dic/2014
 *      Author: AmministratoreErrico
 */

#include "eecfg.h"
#include "ee_hal_internal.h"
#include "ee_scheduler.h"
#include "ee_kernel_types.h"
#include "ee_get_kernel_and_core.h"

void EE_scheduler_task_not_terminated ( void ) {
  EE_CDB * p_CDB;
  EE_TDB * p_terminated;

  EE_hal_disableIRQ();

  p_CDB = EE_get_curr_core();
  p_terminated = EE_scheduler_task_terminated(EE_get_kernel(), p_CDB);
  mppa_tracepoint(erika,TERMINATE_TASK_ENTER, p_terminated->tid);
  EE_hal_terminate_activation(&p_terminated->HDB, &EE_scheduler_task_end);
}

void EE_scheduler_task_end ( void ) {
  EE_TDB EE_CONST * p_To;
  EE_CDB EE_CONST * const p_CDB  = EE_get_curr_core();
  EE_TDB EE_CONST * const p_From = p_CDB->p_CCB->p_curr;

  EE_hal_activation_terminated(&p_From->HDB);

  p_To = EE_scheduler_task_schedule_next(EE_get_kernel(), p_CDB);

  mppa_tracepoint(erika,TERMINATE_TASK_EXIT, p_From->tid);

  if ( p_From->task_type != EE_TASK_TYPE_ISR2 ) {
    EE_hal_change_context(&p_To->HDB, p_To->task_func, &p_From->HDB);
  } else {
    EE_hal_change_context_from_isr2(&p_To->HDB, p_To->task_func,
      &p_From->HDB);
  }
}

void EE_scheduler_task_wrapper ( EE_task_func task_func ) {
#if (defined(EE_SCHEDULER_GLOBAL))
  EE_CDB EE_CONST * const p_CDB                 = EE_get_curr_core();
  EE_CCB          * const p_CCB                 = p_CDB->p_CCB;
  EE_k1_spinlock  * const p_lock_to_be_released = p_CCB->p_lock_to_be_released;

  EE_unlock_kernel();

  if ( p_lock_to_be_released != NULL ) {
    EE_k1_spin_unlock(p_lock_to_be_released);
    p_CCB->p_lock_to_be_released = NULL;
  }
#endif /* EE_SCHEDULER_GLOBAL */
  EE_hal_enableIRQ();
  if ( task_func != NULL ) {
    task_func ();
  }
}
