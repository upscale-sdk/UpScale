#ifndef EE_KERNEL_K1_H_
#define EE_KERNEL_K1_H_

#include "ee_k1_bsp.h"
#include "ee_api.h"
#include "ee_api_k1.h"
#include "ee_kernel_types.h"

typedef struct EE_JOB_tag {
  EE_k1_spinlock  lock;
  EE_TID          task_attendees_id[EE_K1_PE_NUMBER];
  EE_job_func     job_func;
  EE_job_param    job_param;
  EE_job_prio     job_prio;
  EE_JOB_ID       previous;
  EE_UREG         attendee_mask;
  EE_UREG         terminated_task_mask;
} EE_JOB;

typedef struct EE_TCB_WJ_tag
{
  EE_TCB        tcb;
  EE_JOB  *     p_job;
} EE_TCB_WJ;

typedef struct EE_CCB_WL_tag {
  EE_CCB         ccb;
  EE_k1_spinlock lock;
} EE_CCB_WL;

typedef struct EE_KCB_WJ_tag {
  EE_k1_spinlock     lock;
  EE_KCB             kcb;
  EE_CCB_WL          core_ctrls[EE_K1_CORE_NUMBER];
  EE_JOB             jobs[EE_MAX_NUM_JOB];
  EE_array_index     job_index;
} __attribute__ ((aligned(_K1_DCACHE_LINE_SIZE))) EE_KCB_WJ;

typedef struct EE_KDB_WJ_tag {
  EE_KDB          kdb;
  EE_KCB_WJ *     p_KCB_WJ;
  EE_CDB          core_descriptors[EE_K1_CORE_NUMBER];
} EE_KDB_WJ;

extern EE_KCB_WJ KCB_WJ;
extern EE_KDB_WJ EE_CONST KDB_WJ;

extern EE_TCB_WJ ee_core0_tcb_array[EE_CORE0_TCB_ARRAY_SIZE];
extern EE_TCB_WJ ee_core1_tcb_array[EE_CORE1_TCB_ARRAY_SIZE];
extern EE_TCB_WJ ee_core2_tcb_array[EE_CORE2_TCB_ARRAY_SIZE];
extern EE_TCB_WJ ee_core3_tcb_array[EE_CORE3_TCB_ARRAY_SIZE];
extern EE_TCB_WJ ee_core4_tcb_array[EE_CORE4_TCB_ARRAY_SIZE];
extern EE_TCB_WJ ee_core5_tcb_array[EE_CORE5_TCB_ARRAY_SIZE];
extern EE_TCB_WJ ee_core6_tcb_array[EE_CORE6_TCB_ARRAY_SIZE];
extern EE_TCB_WJ ee_core7_tcb_array[EE_CORE7_TCB_ARRAY_SIZE];
extern EE_TCB_WJ ee_core8_tcb_array[EE_CORE8_TCB_ARRAY_SIZE];
extern EE_TCB_WJ ee_core9_tcb_array[EE_CORE9_TCB_ARRAY_SIZE];
extern EE_TCB_WJ ee_core10_tcb_array[EE_CORE10_TCB_ARRAY_SIZE];
extern EE_TCB_WJ ee_core11_tcb_array[EE_CORE11_TCB_ARRAY_SIZE];
extern EE_TCB_WJ ee_core12_tcb_array[EE_CORE12_TCB_ARRAY_SIZE];
extern EE_TCB_WJ ee_core13_tcb_array[EE_CORE13_TCB_ARRAY_SIZE];
extern EE_TCB_WJ ee_core14_tcb_array[EE_CORE14_TCB_ARRAY_SIZE];
extern EE_TCB_WJ ee_core15_tcb_array[EE_CORE15_TCB_ARRAY_SIZE];
extern EE_TCB_WJ ee_rm_tcb_array[EE_CORE_RM_TCB_ARRAY_SIZE];

extern EE_schedule_data ee_core0_sd_array[EE_CORE0_SD_ARRAY_SIZE];
extern EE_schedule_data ee_core1_sd_array[EE_CORE1_SD_ARRAY_SIZE];
extern EE_schedule_data ee_core2_sd_array[EE_CORE2_SD_ARRAY_SIZE];
extern EE_schedule_data ee_core3_sd_array[EE_CORE3_SD_ARRAY_SIZE];
extern EE_schedule_data ee_core4_sd_array[EE_CORE4_SD_ARRAY_SIZE];
extern EE_schedule_data ee_core5_sd_array[EE_CORE5_SD_ARRAY_SIZE];
extern EE_schedule_data ee_core6_sd_array[EE_CORE6_SD_ARRAY_SIZE];
extern EE_schedule_data ee_core7_sd_array[EE_CORE7_SD_ARRAY_SIZE];
extern EE_schedule_data ee_core8_sd_array[EE_CORE8_SD_ARRAY_SIZE];
extern EE_schedule_data ee_core9_sd_array[EE_CORE9_SD_ARRAY_SIZE];
extern EE_schedule_data ee_core10_sd_array[EE_CORE10_SD_ARRAY_SIZE];
extern EE_schedule_data ee_core11_sd_array[EE_CORE11_SD_ARRAY_SIZE];
extern EE_schedule_data ee_core12_sd_array[EE_CORE12_SD_ARRAY_SIZE];
extern EE_schedule_data ee_core13_sd_array[EE_CORE13_SD_ARRAY_SIZE];
extern EE_schedule_data ee_core14_sd_array[EE_CORE14_SD_ARRAY_SIZE];
extern EE_schedule_data ee_core15_sd_array[EE_CORE15_SD_ARRAY_SIZE];
extern EE_schedule_data ee_rm_sd_array[EE_CORE_RM_SD_ARRAY_SIZE];

typedef struct EE_ar_rpc_type_tag
{
  EE_CORE_ID      serving_core;
  EE_service_id   remote_procedure;
  uintptr_t       param1;
  uintptr_t       param2;
  uintptr_t       param3;
  EE_status_type  error;
} EE_ar_rpc_type;

#define INVALID_PARAM ((uintptr_t)-1)
extern EE_ar_rpc_type ee_ar_rpc_send[EE_K1_PE_NUMBER];
//extern EE_ar_rpc_type ee_ar_rpc_receive[EE_K1_PE_NUMBER];

void EE_job_wrapper ( void );

EE_status_type EE_ar_rpc (  EE_service_id service_id,
  uintptr_t param1, uintptr_t param2, uintptr_t param3 );
EE_status_type EE_k1_rm_ar_rpc ( EE_ar_rpc_type * const p_rpc_ref,
  EE_CORE_ID requiring_core );
EE_status_type EE_k1_rm_activate_job ( EE_JOB_ID job_id, EE_UREG core_num,
  EE_CORE_ID requiring_core );


#endif /* EE_KERNEL_K1_H_ */
