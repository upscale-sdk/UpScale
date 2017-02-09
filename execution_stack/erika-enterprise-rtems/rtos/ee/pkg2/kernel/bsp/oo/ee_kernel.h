/*
 * ee_kernel.h
 *
 *  Created on: 07/dic/2014
 *      Author: AmministratoreErrico
 */

#ifndef EE_KERNEL_H_
#define EE_KERNEL_H_

#include "ee_api.h"
#include "ee_hal_internal.h"
#include "ee_kernel_types.h"
#include "ee_scheduler.h"
#include "ee_get_kernel_and_core.h"

EE_INLINE__ EE_TDB * EE_get_curr_task ( void ) {
  EE_KDB EE_CONST * const p_KDB     = EE_get_kernel();
  EE_CDB EE_CONST * const p_CDB     = EE_get_curr_core();
  EE_CCB          * const p_CCB     = (p_CDB != NULL)? p_CDB->p_CCB: NULL;
  EE_TID            const curr_tid  = (p_CCB != NULL)? p_CCB->curr_tid:
    INVALID_TASK_ID;
  EE_TDB EE_CONST * p_TDB = NULL;
  if ( (p_KDB != NULL) && (p_KDB->tdb_array != NULL) &&
     (curr_tid != INVALID_TASK_ID) )
  {
    p_TDB =  &p_KDB->tdb_array[curr_tid];
  }
  return p_TDB ;
}

EE_INLINE__ void EE_task_end ( EE_TDB * const p_TDB ) {
  if ( p_TDB != NULL ) {
    EE_TCB * const p_TCB = p_TDB->p_TCB;
    if ( p_TCB != NULL ) {
      ++p_TCB->residual_activation;
      p_TCB->current_prio = p_TDB->ready_prio;

      if ( p_TCB->residual_activation == p_TDB->max_num_of_act )
      {
        p_TCB->status = EE_TASK_SUSPENDED;
      } else {
        p_TCB->status = EE_TASK_READY;
      }
    }
  }
}

EE_INLINE__ EE_status_type EE_activate_isr2( EE_TID isr2_id ) {
  EE_status_type ret_val  = E_OK;
  EE_KDB * const p_KDB    = EE_get_kernel();
  EE_CDB * const p_CDB    = EE_lock_and_get_curr_core();
  EE_CCB * const p_CCB    = (p_CDB != NULL)? p_CDB->p_CCB: NULL;
  EE_TDB * const p_ActTDB = (p_KDB != NULL)? &p_KDB->tdb_array[isr2_id]: NULL;
  EE_TCB * const p_ActTCB = p_ActTDB->p_TCB;

  if ( p_ActTCB->residual_activation > 0U ) {
    EE_TDB * const p_FromTDB = (p_KDB != NULL)?
      &p_KDB->tdb_array[p_CCB->curr_tid]: NULL;

    if ( p_ActTCB != NULL ) {
      --p_ActTCB->residual_activation;
    }
    EE_scheduler_task_set_running(p_CDB, p_ActTDB);
    EE_unlock_curr_core();
    EE_hal_change_context(p_ActTDB->p_HDB, p_ActTDB->task_func,
      p_FromTDB->p_HDB);
  } else {
    EE_unlock_curr_core();
    ret_val = E_OS_LIMIT;
  }
  return ret_val;
}

#ifdef EE_API_DYNAMIC
StatusType EE_create_task ( EE_CORE_ID core_id, EE_TID * taskIdRef,
  EE_task_type taskType, EE_task_func taskFunc, EE_task_prio readyPrio,
  EE_task_prio dispatchPrio, EE_task_nact maxNumOfAct, EE_mem_size stackSize );
#endif /* EE_API_DYNAMIC */

#endif /* EE_KERNEL_H_ */
