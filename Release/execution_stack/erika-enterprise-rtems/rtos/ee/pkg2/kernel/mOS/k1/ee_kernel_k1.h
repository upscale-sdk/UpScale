#ifndef EE_KERNEL_K1_H_
#define EE_KERNEL_K1_H_

#include "ee_k1_vbsp.h"
#include "ee_api.h"
#include "ee_api_k1.h"
#include "ee_kernel_types.h"

typedef struct EE_JOB_tag {
  EE_k1_spinlock  lock;
  EE_TID          task_attendees_id[EE_K1_CORE_NUMBER];
  EE_job_func     job_func;
  EE_job_param    job_param;
  EE_job_prio     job_prio;
  EE_JOB_ID       job_id;
  EE_UREG         attendee_mask;
  EE_UREG         terminated_task_mask;
} EE_JOB;

typedef struct EE_CCB_WL_tag {
  EE_CCB         ccb;
  EE_k1_spinlock lock;
} EE_CCB_WL;

typedef struct EE_KCB_WJ_tag {
  EE_KCB             kcb;
  EE_k1_spinlock     lock;
  EE_CCB_WL          core_ctrls[EE_K1_CORE_NUMBER];
  EE_JOB             jobs[EE_MAX_NUM_JOB];
  EE_JOB  *          tid_to_job[EE_TASK_ARRAY_SIZE];
  EE_array_index     job_index;
} __attribute__ ((aligned(_K1_DCACHE_LINE_SIZE))) EE_KCB_WJ;

typedef struct EE_KDB_WJ_tag {
  EE_KDB          kdb;
  EE_KCB_WJ *     p_KCB_WJ;
  EE_CDB          core_descriptors[EE_K1_CORE_NUMBER];
} EE_KDB_WJ;

extern EE_KCB_WJ KCB_WJ;
extern EE_KDB_WJ EE_CONST KDB_WJ;

#define INVALID_PARAM ((uintptr_t)-1)

void EE_job_wrapper ( void );

EE_status_type EE_k1_activate_job ( EE_JOB_ID job_id, mOS_vcore_set_t core_mask,
  EE_CORE_ID requiring_core
#if (defined(EE_K1_FULL_PREEMPTION))
  , EE_BOOL preemption_point
#endif /* EE_K1_FULL_PREEMPTION */
  );

#if (defined(EE_HAS_COMM_HOOK))
extern int isPendingOffload ( void );
extern void CommunicationHook ( void );
extern EE_k1_spinlock comm_lock;
#endif

void EE_k1_optimized_task_preemption_point ( void );

static EE_BOOL EE_k1_umem_data_priority_insert ( EE_SN ** pp_first,
  EE_SN * p_SN_new, EE_TDB * EE_CONST p_TDB_new )
{
  EE_BOOL     head_changed  = EE_FALSE;
  EE_SN *     p_prev        = NULL;
  EE_SN *     p_curr        = (EE_SN *)__k1_umem_read32(pp_first);
  EE_TDB *    p_curr_tdb    = (EE_TDB *)__k1_umem_read32(&p_curr->p_TDB);

  EE_task_prio  const  new_task_prio =
    (EE_task_prio)__k1_umem_read32(&p_TDB_new->p_TCB->current_prio);

  /* Save the TDB on the new SN */
  p_SN_new->p_TDB = p_TDB_new;

  /* Traverse the queue until needed */
  while ( (p_curr != NULL) &&
    (new_task_prio <= __k1_umem_read32(&p_curr_tdb->p_TCB->current_prio)) )
  {
    p_prev     = p_curr;
    p_curr     = (EE_SN *)__k1_umem_read32(&p_curr->p_next);
    p_curr_tdb = (EE_TDB *)__k1_umem_read32(&p_curr->p_TDB);
  };

  if ( p_prev != NULL ) {
    p_prev->p_next = p_SN_new;
  } else {
    (*pp_first)  = p_SN_new;
    head_changed = EE_TRUE;
  }

  p_SN_new->p_next = p_curr;

  /* Commit all the changes in memory */
  EE_k1_wmb();
  return head_changed;
}

#endif /* EE_KERNEL_K1_H_ */
