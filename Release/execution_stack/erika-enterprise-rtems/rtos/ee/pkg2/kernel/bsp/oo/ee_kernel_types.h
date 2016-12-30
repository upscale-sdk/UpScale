#ifndef EE_KERNEL_TYPES_H_
#define EE_KERNEL_TYPES_H_

#include "eecfg.h"
#include "ee_api_types.h"
#include "ee_hal_internal_types.h"

typedef struct EE_TCB_tag {
  EE_task_status  status;
  EE_task_prio    current_prio;
  EE_task_nact    residual_activation;
  EE_CORE_ID      current_core;
} EE_TCB;

typedef struct EE_TDB_tag {
  EE_TCB *         p_TCB;
  EE_HDB *         p_HDB;
  EE_CORE_ID       orig_core;
  EE_TID           tid;
  EE_task_type     task_type;
  EE_task_prio     ready_prio;
  EE_task_prio     dispatch_prio;
  EE_task_func     task_func;
  EE_task_nact     max_num_of_act;
} EE_TDB;

typedef struct EE_CCB_tag {
  EE_TID           curr_tid;
  EE_array_index   rq_first;
  EE_array_index   stk_first;
  EE_BOOL          os_started;
  EE_array_index   free_SD_index;
} EE_CCB;

typedef struct EE_schedule_data_tag {
  EE_array_index next;
  EE_TID         tid;
} EE_schedule_data;

typedef struct EE_CDB_tag {
  EE_CCB *           p_CCB;
  EE_TCB *           tcb_array;
  EE_array_size      tcb_array_size;
  EE_schedule_data * sd_array;
  EE_array_index     sd_array_size;
#ifdef EE_API_DYNAMIC
  EE_array_index     free_TCB_index;
#endif /* EE_API_DYNAMIC */
} EE_CDB;

typedef struct EE_KCB_tag {
  EE_array_index free_TDB_index;
} EE_KCB;

typedef struct EE_KDB_tag {
  EE_KCB * p_KCB;
  EE_TDB   tdb_array[EE_TDB_ARRAY_SIZE];
} EE_KDB;

#endif /* !EE_KERNEL_TYPES_H_ */
