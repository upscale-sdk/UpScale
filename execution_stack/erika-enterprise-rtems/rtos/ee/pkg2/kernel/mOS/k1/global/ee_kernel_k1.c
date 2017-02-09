/*
 * ee_kernel_k1.c
 *
 *  Created on: Jan 22, 2015
 *      Author: e.guidieri
 */
#include "ee_k1_vbsp.h"
#include "ee_hal_internal.h"
#include "ee_kernel.h"
#include "ee_kernel_k1.h"
#include <assert.h>

/* Workaround to get the insertion function from scheduler implementation */
extern EE_BOOL EE_schedule_data_priority_insert ( EE_SN ** pp_first,
  EE_SN * p_SN_new, EE_TDB * EE_CONST p_TDB_new );

EE_status_type EE_k1_activate_job ( EE_JOB_ID job_id, mOS_vcore_set_t core_mask,
  EE_CORE_ID requiring_core
#if (defined(EE_K1_FULL_PREEMPTION))
  , EE_BOOL preemption_point
#endif /* EE_K1_FULL_PREEMPTION */
  )
{
  EE_status_type         status_type;
  EE_UREG                i, core_counts;
  EE_TID                 local_task_attendee[EE_K1_CORE_NUMBER];
  EE_JOB         * const p_curr_job    = &KCB_WJ.jobs[job_id];
  EE_UREG          const core_mask_bs  = __k1_cbs(core_mask);

  /* Lock The Core */
  EE_lock_kernel();

  /* check preconditions */
  for ( i = core_counts = 0U; (i < EE_K1_CORE_NUMBER) &&
    (core_counts < core_mask_bs); ++i )
  {
    EE_TID task_id  = p_curr_job->task_attendees_id[i];
    if ( task_id != INVALID_TASK_ID ) {
      EE_TDB EE_CONST * const p_Act = &KDB_WJ.kdb.tdb_array[task_id];
      if ( p_Act->p_TCB->residual_activation > 0U ) {
        ++core_counts;
        local_task_attendee[i] = task_id;
      } else {
        local_task_attendee[i] = INVALID_TASK_ID;
      }
    }
  }

  /* If preconditions are full-filled Activate all the job TASks */
  if ( core_counts >= core_mask_bs ) {
    /* Get the Job lock */
    __k1_fspinlock_lock(&p_curr_job->lock);
    p_curr_job->attendee_mask = 0U;
    for ( i = 0U; i < core_mask_bs; ++i ) {
      EE_TID task_id = local_task_attendee[i];
      if ( task_id != INVALID_TASK_ID ) {
        EE_TDB EE_CONST * const p_Act = &KDB_WJ.kdb.tdb_array[task_id];

        /* Register the TASK as belonging to the JOB */
        KCB_WJ.tid_to_job[task_id] = p_curr_job;
        p_curr_job->attendee_mask |= EE_BIT(i);

        /* Prepare TASK */
        if ( p_Act->p_TCB->status == EE_TASK_SUSPENDED ) {
          p_Act->p_TCB->status = EE_TASK_READY;
        }
        --p_Act->p_TCB->residual_activation;

        EE_schedule_data_priority_insert (&KCB_WJ.kcb.p_rq_first,
          EE_scheduler_sn_alloc(&KCB_WJ.kcb), p_Act);
      }
    }
    /* Reset the Terminated Task Mask */
    p_curr_job->terminated_task_mask = 0U;

    /* Commit all the changes in memory */
    EE_k1_wmb();
    /* Release the Job lock */
    __k1_fspinlock_unlock(&p_curr_job->lock);
    status_type = E_OK;
  } else {
    status_type = E_OS_LIMIT;
  }
  /* Unlock the Kernel */
  EE_unlock_kernel();

  /* Raise an interrupt on the receiving core if:
   * * The job activation has been successful
   */
  if ( status_type == E_OK ) {
    EE_hal_broadcast_signal();
  }

  return status_type;
}

StatusType SignalValue (BlockableValueTypeRef BlockableValueRef,
  ValueType Value)
{
  StatusType     status_type;
  if ( BlockableValueRef == NULL ) {
    status_type = E_OS_PARAM_POINTER;
  } else {
    __k1_uint32_t  p_bq_temp;
    EE_SN    *     p_blocked_queue;

    __k1_fspinlock_lock(&BlockableValueRef->lock);

    __k1_umem_write32(&BlockableValueRef->value, Value);

    p_bq_temp       = __k1_umem_read32(&BlockableValueRef->blocked_queue);
    p_blocked_queue = (EE_SN *)p_bq_temp;

    if ( p_blocked_queue != NULL ) {
      EE_BOOL cond_result;
      EE_wait_cond const wait_cond =
        __k1_umem_read32(&BlockableValueRef->wait_cond);
      EE_UREG      const right_value =
        __k1_umem_read32(&BlockableValueRef->right_value);

      status_type = CheckCondition(&cond_result, Value, wait_cond, right_value);

      if ( (status_type == E_OK) && cond_result ) {
        EE_KDB EE_CONST * const p_KDB         = EE_get_kernel();
        EE_CDB EE_CONST * const p_CDB         = EE_get_curr_core();
        EE_BOOL           const is_preemption = EE_scheduler_task_unblocked (
          p_KDB, p_CDB, p_blocked_queue);

        /* Pop from the blocked queue */
        __k1_umem_write32(&BlockableValueRef->blocked_queue,
          (__k1_uint32_t)BlockableValueRef->blocked_queue->p_next);

        __k1_fspinlock_unlock(&BlockableValueRef->lock);

        if ( is_preemption ) {
          EE_k1_optimized_task_preemption_point();
        }
      } else {
        __k1_fspinlock_unlock(&BlockableValueRef->lock);
      }
    } else {
      __k1_fspinlock_unlock(&BlockableValueRef->lock);
      status_type = E_OK;
    }
  }
  return status_type;
}

#if 1
void EE_k1_optimized_task_preemption_point ( void ) {
    EE_SN           *        p_rq_first;
    EE_KDB_WJ       * const  p_KDB  = (EE_KDB_WJ *)EE_get_kernel();
    EE_KCB_WJ       * const  p_KCB  = p_KDB->p_KCB_WJ;
    EE_CDB          * const  p_CDB  = EE_get_curr_core();
    EE_CCB          * const  p_CCB  = p_CDB->p_CCB;

    /* Lock the Scheduler */
    __k1_fspinlock_lock(&p_KCB->lock);

    p_rq_first      = (EE_SN *)__k1_umem_read32(&p_KCB->kcb.p_rq_first);

    if ( p_rq_first != NULL ) {
      EE_TDB EE_CONST * const  p_rq_first_tdb  =
        (EE_TDB EE_CONST *)__k1_umem_read32(&p_rq_first->p_TDB);

      EE_TCB          * const  p_rq_first_tcb  = p_rq_first_tdb->p_TCB;

      EE_TDB EE_CONST * const  p_curr          =
        (EE_TDB EE_CONST *)__k1_umem_read32(&p_CCB->p_curr);

      EE_task_prio      const  rq_current_prio =
        __k1_umem_read32(&p_rq_first_tcb->current_prio);

      EE_BOOL           const  is_preemption   =
       rq_current_prio > __k1_umem_read32(&p_curr->p_TCB->current_prio);

      /* If there'is preemption: schedule */
      if ( is_preemption ) {
        EE_SN * const  p_curr_sn =
          (EE_SN *)__k1_umem_read32(&p_CCB->p_curr_sn);

        p_curr->p_TCB->status              = EE_TASK_READY;

        /* Extract from ready queue */
        p_KCB->kcb.p_rq_first  = (EE_SN *)__k1_umem_read32(&p_rq_first->p_next);

        /* Set as current */
        p_rq_first->p_next                 = NULL;
        p_CCB->p_curr_sn                   = p_rq_first;
        p_CCB->p_curr                      = p_rq_first_tdb;

        p_rq_first_tcb->current_core_id    = EE_get_curr_core_id();
        p_rq_first_tcb->status             = EE_TASK_RUNNING;

        /* Adjust actual priority with dispatch priority: if needed */
        if ( rq_current_prio < p_rq_first_tdb->dispatch_prio ) {
          p_rq_first_tcb->current_prio = p_rq_first_tdb->dispatch_prio;
        }

        if ( p_curr != p_CDB->p_idle_task ) {
          (void)EE_k1_umem_data_priority_insert ( &p_KCB->kcb.p_rq_first,
            p_curr_sn, p_curr );
          EE_hal_broadcast_signal ();
        }

        /* Full memory barrier: commit data in memory + cache invalidation */
        EE_k1_mb();

        /* YOU HAD NOT TO RELESE THE KERNEL HERE IS A GLOBAL QUEUE NEED TO BE
         * RELEASED AFTER ACTUAL CONTEXT CHANGE */

        EE_hal_change_context(&p_rq_first_tdb->HDB,
          p_rq_first_tdb->task_func, &p_curr->HDB);
      } else {
        __k1_fspinlock_unlock(&p_KCB->lock);
      }
    } else {
      __k1_fspinlock_unlock(&p_KCB->lock);
    }
}
#else
void EE_k1_optimized_task_preemption_point ( void ) {
  /* TODO: Not yet implemented */
  EE_scheduler_task_preemption_point(EE_get_kernel(), EE_get_curr_core());
}
#endif

