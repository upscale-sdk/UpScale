#ifndef EE_KERNEL_TYPES_H_
#define EE_KERNEL_TYPES_H_

#include "eecfg.h"
#include "ee_api_types.h"
#include "ee_hal_internal_types.h"

struct EE_TDB_tag;

typedef struct EE_TCB_tag {
  EE_task_status                  status;
  EE_task_prio                    current_prio;
  EE_task_nact                    residual_activation;
  EE_CORE_ID                      current_core_id;
} EE_TCB;

typedef struct EE_TDB_tag {
  EE_HDB           HDB;
  EE_TCB *         p_TCB;
  EE_CORE_ID       orig_core_id;
  EE_TID           tid;
  EE_task_type     task_type;
  EE_task_prio     ready_prio;
  EE_task_prio     dispatch_prio;
  EE_task_func     task_func;
  EE_task_nact     max_num_of_act;
} EE_TDB;

typedef struct EE_SN_tag {
  struct EE_SN_tag  *  p_next;
  EE_TDB  EE_CONST  *  p_TDB;
} EE_SN;

typedef struct EE_CCB_tag {
  EE_TDB EE_CONST  *  p_curr;
#if (!defined(EE_SINGLECORE))
  EE_spin_lock     *  p_lock_to_be_released;
#endif /* EE_SINGLECORE */
  EE_BOOL        os_started;
#if (!defined(EE_SCHEDULER_GLOBAL))
  EE_SN *   p_rq_first;
  EE_SN *   p_free_sn_first;
#endif /* EE_SCHEDULER_GLOBAL */
  EE_SN *         p_curr_sn;
/* Error Handling variables */
  EE_status_type  last_error;
#if (defined(EE_HAS_ERROR_HOOK))
  /* TODO */
#endif /* EE_HAS_ERROR_HOOK */
} EE_CCB;

typedef struct EE_CDB_tag {
  EE_CCB *    p_CCB;
  EE_TDB *    p_idle_task;
} EE_CDB;

typedef struct EE_KCB_tag {
  EE_TCB          tcb_array[EE_TASK_ARRAY_SIZE + EE_K1_CORE_NUMBER];
  EE_SN           sn_array[EE_SD_ARRAY_SIZE];
#if (defined(EE_SCHEDULER_GLOBAL))
  EE_SN *         p_rq_first;
  EE_SN *         p_free_sn_first;
#endif
#ifdef EE_API_DYNAMIC
  EE_array_index  free_task_index;
#endif /* EE_API_DYNAMIC */
} EE_KCB;

typedef struct EE_KDB_tag {
  EE_KCB *        p_KCB;
  EE_TDB          tdb_array[EE_TASK_ARRAY_SIZE + EE_K1_CORE_NUMBER];
} EE_KDB;

#endif /* !EE_KERNEL_TYPES_H_ */
