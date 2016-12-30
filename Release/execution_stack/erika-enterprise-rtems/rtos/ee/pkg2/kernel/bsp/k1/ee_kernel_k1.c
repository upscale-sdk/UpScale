/*
 * ee_kernel_k1.c
 *
 *  Created on: Jan 22, 2015
 *      Author: e.guidieri
 */
#include "ee_k1_bsp_communication.h"
#include "ee_hal_internal.h"
#include "ee_kernel.h"
#include "ee_kernel_k1.h"
#include <assert.h>

EE_ar_rpc_type ee_ar_rpc_send[EE_K1_PE_NUMBER];
//EE_ar_rpc_type ee_ar_rpc_receive[EE_K1_PE_NUMBER];

#if 0
ISR(EE_iirq_handler) {
  EE_CORE_ID       const core_id     = EE_get_curr_core_id();
  EE_ar_rpc_type * const p_rpc_rcv   = &ee_ar_rpc_receive[core_id];

  switch ( p_rpc_rcv->remote_procedure ) {
   case OSServiceId_ActivateJob:
     /* Nothing TODO: Everything is already done in RM core */
     break;
   case OSServiceId_SignalValue:
     /* Nothing TODO: Everything is already done in RM core */
     break;
   default:
     assert(false);
     /* Never Reached */
     break;
  }
}
#endif /* 0 */

void EE_job_wrapper ( void ) {
  EE_UREG           terminated_mask;
  /* C Single inheritance */
  EE_TCB_WJ * const p_TCB_wj = (EE_TCB_WJ *)(EE_get_curr_task()->p_TCB);
  EE_JOB    * const p_job    = p_TCB_wj->p_job;

  p_job->job_func(p_job->job_param);

  __k1_spin_lock(&p_job->lock);
  terminated_mask = p_job->terminated_task_mask | EE_BIT(EE_get_curr_core_id());
  __k1_umem_write32(&p_job->terminated_task_mask, terminated_mask );
  __k1_spin_unlock(&p_job->lock);

  TerminateTask();
}

EE_status_type EE_k1_rm_activate_job ( EE_JOB_ID job_id, CoreNumber core_num,
  EE_CORE_ID requiring_core )
{
  EE_status_type status_type;
  EE_UREG        i, core_counts;
  EE_TDB *       p_Act;

  struct {
    EE_TID  tid;
    EE_BOOL preemption;
  } local_task_attendee[EE_K1_PE_NUMBER] =
    { [0U ... (EE_K1_PE_NUMBER -1U)] = { INVALID_TASK_ID, EE_FALSE } };

  EE_JOB         * const p_curr_job  = &KCB_WJ.jobs[job_id];

  /* Lock All the Cores that attend to a Job and check preconditions */
  for ( i = core_counts = 0U; i < EE_K1_PE_NUMBER; ++i ) {
    EE_TID task_id  = p_curr_job->task_attendees_id[i];
    if ( task_id != INVALID_TASK_ID ) {

      __k1_spin_lock( &KCB_WJ.core_ctrls[i].lock );

      p_Act = &KDB_WJ.kdb.tdb_array[task_id];
      if ( p_Act->p_TCB->residual_activation > 0U ) {
        ++core_counts;
        local_task_attendee[i].tid = task_id;
      }
    }
  }

  /* If preconditions are full-filled Activate all the job TASks */
  if ( core_counts >= core_num ) {
    /* Get the Job lock */
    __k1_spin_lock(&p_curr_job->lock);
    p_curr_job->attendee_mask = 0U;
    for ( i = 0U; i < core_num; ++i ) {
      EE_TID task_id = local_task_attendee[i].tid;
      if ( task_id != INVALID_TASK_ID ) {
        EE_TCB_WJ * p_TCB_wj;
        EE_CDB EE_CONST * const p_CDB = EE_get_core(i);

        p_Act = &KDB_WJ.kdb.tdb_array[task_id];
        /* C Single Inheritance */
        p_TCB_wj = (EE_TCB_WJ *)p_Act->p_TCB;

        /* Prepare TASK */
        --p_TCB_wj->tcb.residual_activation;
        p_TCB_wj->p_job = p_curr_job;

        p_curr_job->attendee_mask |= EE_BIT(i);

        local_task_attendee[i].preemption =
          EE_scheduler_task_activated(p_CDB, p_Act);
      }
      /* Set the remote procedure ID for the receiving cores */
      //ee_ar_rpc_receive[i].remote_procedure = OSServiceId_ActivateJob;
    }
    /* Reset the Terminated Task Mask */
    p_curr_job->terminated_task_mask = 0U;

    /* Commit all the changes in memory */
    EE_k1_wmb();
    /* Release the Job lock */
    __k1_spin_unlock(&p_curr_job->lock);

    status_type = E_OK;
  } else {
    status_type = E_OS_LIMIT;
  }

  /* Free All the Cores that attend to a Job And Notify each attendee core */
  for ( i = 0U; (i < EE_K1_PE_NUMBER) && (core_counts > 0U) ; ++i ) {
    if ( local_task_attendee[i].tid != INVALID_TASK_ID ) {
      --core_counts;
      __k1_spin_unlock( &KCB_WJ.core_ctrls[i].lock );
      /* Raise an interrupt on the receiving core if:
       * * The job activation has been successful
       * * The interrupt receiving core is no the activation requiring core
       *   (Polling communication already active between requiring and RM cores)
       * * The job activation has preempted the actual execution TASK in
       *   receiving core (otherwise the interrupt is just waste of time).
       */
      if ( (status_type == E_OK) && (i != requiring_core) &&
         local_task_attendee[i].preemption )
      {
        EE_k1_event_notify(EE_K1_BOOT_CORE, i, EE_K1_AMP_EVENT);
      }
    }
  }

  return status_type;
}

static EE_status_type
  EE_k1_rm_signal_value (BlockableValueTypeRef BlockableValueRef)
{
  EE_status_type status_type;
  if ( BlockableValueRef == NULL ) {
   status_type = E_OS_PARAM_POINTER;
  } else
  {
    EE_CORE_ID core_id;
    core_id = BlockableValueRef->core_id;
    if ( core_id == INVALID_CORE_ID ) {
      /* THIS SHALL NEVER HAPPENS */
      status_type = E_OS_STATE;
      assert(false);
    } else
    {
      EE_CDB *       const p_CDB = EE_lock_and_get_core(core_id);
      /* check if the post on the semaphore wakes up someone */
      if ( BlockableValueRef->blocked_queue != INVALID_INDEX ) {
        EE_BOOL      const rqHeadChanged = EE_scheduler_task_unblocked(p_CDB,
          &BlockableValueRef->blocked_queue);
        BlockableValueRef->core_id = INVALID_CORE_ID;
        if ( rqHeadChanged ) {
          EE_unlock_core(core_id);
          EE_k1_event_notify(EE_K1_BOOT_CORE, core_id, EE_K1_AMP_EVENT);
        } else {
          EE_unlock_core(core_id);
        }
      } else {
        /* THIS SHALL NEVER HAPPENS */
        assert(false);
      }
    }
  }
  return status_type;
}

EE_status_type EE_k1_rm_ar_rpc ( EE_ar_rpc_type * const p_rpc_ref,
  EE_CORE_ID requiring_core )
{
  EE_status_type status_type;
  switch ( p_rpc_ref->remote_procedure ) {
    case OSServiceId_ActivateJob:
      status_type = EE_k1_rm_activate_job(p_rpc_ref->param1, p_rpc_ref->param2,
        requiring_core);
    break;
    case OSServiceId_SignalValue:
      status_type = EE_k1_rm_signal_value((BlockableValueTypeRef)p_rpc_ref->
        param1);
    break;
    default:
      status_type = E_OS_SERVICEID;
    break;
  }
  return status_type;
}

void EE_k1_rm_iirq0_handler ( int r0 ) {
  EE_UREG event_mask = __builtin_k1_get(_K1_SFR_EV0);
  while ( event_mask != 0U ) {
    EE_CORE_ID core_id;
    for ( core_id = 0U; core_id < EE_K1_PE_NUMBER; ++core_id ) {
      if ( (event_mask & EE_BIT(core_id)) != 0 ) {
        ee_ar_rpc_send[core_id].error =
        EE_k1_rm_ar_rpc(&ee_ar_rpc_send[core_id], core_id);
        EE_k1_event_clear ( EE_K1_BOOT_CORE, core_id, EE_K1_AMP_EVENT );
        EE_k1_event_notify ( EE_K1_BOOT_CORE, core_id, EE_K1_AMP_ACK_EVENT );
      }
    }
    event_mask = __builtin_k1_get(_K1_SFR_EV0);
  }
}

StatusType EE_ar_rpc ( EE_service_id service_id,
  uintptr_t param1, uintptr_t param2, uintptr_t param3 )
{
  EE_CORE_ID core_id = EE_get_curr_core_id();
  EE_ar_rpc_type * p_rpc = &ee_ar_rpc_send[core_id];

  p_rpc->remote_procedure = service_id;
  p_rpc->param1 = param1;
  p_rpc->param2 = param2;
  p_rpc->param3 = param3;
  EE_k1_wmb();
  EE_k1_event_notify(core_id, EE_K1_BOOT_CORE, EE_K1_AMP_EVENT);
  EE_k1_event_idle_and_clear(core_id, EE_K1_BOOT_CORE, EE_K1_AMP_ACK_EVENT);
  EE_k1_rmb();
  return p_rpc->error;
}

