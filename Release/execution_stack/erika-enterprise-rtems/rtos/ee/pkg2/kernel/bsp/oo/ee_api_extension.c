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

StatusType WaitSem ( SemRefType Sem ) {
  /* Error Value */
  StatusType  status_type;
  EE_KDB EE_CONST * p_KDB = EE_get_kernel();
  if ( Sem == NULL ) {
    status_type = E_OS_PARAM_POINTER;
  } else
  {
    EE_FREG     flags = EE_hal_begin_nested_primitive();
    EE_CDB * const p_CDB     = EE_lock_and_get_curr_core();
    EE_TDB * const p_current = &p_KDB->tdb_array[p_CDB->p_CCB->curr_tid];
    if ( p_current->task_type != EE_TASK_TYPE_EXTENDED ) {
      EE_unlock_curr_core();
      status_type = E_OS_ACCESS;
    } else
    {
      if ( Sem->count != 0U ) {
        --Sem->count;
        EE_unlock_curr_core();
      } else {
        /* Put the TASK inside the semaphore queue */
        EE_TDB * const p_To = EE_scheduler_task_block_current(p_CDB,
          &Sem->blocked_queue);

        EE_unlock_curr_core();
        EE_hal_change_context(p_To->p_HDB, p_To->task_func, p_current->p_HDB);
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
  } else
  {
    EE_KDB EE_CONST * const p_KDB = EE_get_kernel();
    EE_FREG           const flags = EE_hal_begin_nested_primitive();
    EE_CDB *          const p_CDB = EE_lock_and_get_curr_core();
    /* check if the post on the semaphore wakes up someone */
    if ( Sem->blocked_queue != INVALID_INDEX ) {
      EE_BOOL      const rqHeadChanged = EE_scheduler_task_unblocked(p_CDB,
        &Sem->blocked_queue);
      if ( rqHeadChanged ) {
        EE_TDB * const p_From = &p_KDB->tdb_array[p_CDB->p_CCB->curr_tid];
        EE_TDB * const p_To   = EE_scheduler_task_dispatch(p_CDB);

        EE_unlock_curr_core();
        EE_hal_change_context(p_To->p_HDB, p_To->task_func, p_From->p_HDB);
      } else {
        EE_unlock_curr_core();
      }
    } else {
      ++Sem->count;
      EE_unlock_curr_core();
    }
    EE_hal_end_nested_primitive(flags);
    status_type = E_OK;
  }

  return status_type;
}


