/*
 * ee_api_types.h
 *
 *  Created on: 07/dic/2014
 *      Author: AmministratoreErrico
 */

#ifndef EE_API_TYPES_H_
#define EE_API_TYPES_H_

#include "ee_platform_types.h"
#include "ee_basic_data_structures.h"

/**
 * ISR2 un tipo di TASK e usa lo stesso schedulatore degli altri TASK -->
 * Risorse a comune ISR2/TASK nativamente (E' possibile che IPL, vada lo stesso gestisto)
 */
typedef enum EE_task_type_tag {
  EE_TASK_TYPE_IDLE = 0U,
  EE_TASK_TYPE_BASIC,
  EE_TASK_TYPE_EXTENDED,
  EE_TASK_TYPE_ISR2
} EE_task_type;

typedef enum EE_task_status_tag {
  EE_TASK_SUSPENDED = 0U,
  EE_TASK_READY,
  EE_TASK_WAITING,
  EE_TASK_RUNNING,
} EE_task_status;

/* OSEK Style typedefs */
typedef EE_task_id        TaskType;
typedef EE_task_prio      TaskPrio;
typedef EE_task_nact      TaskActivation;
typedef EE_task_func      TaskFunc;
typedef EE_mem_size       MemSize;
typedef EE_isr2_source_id IRS2SourceId;
typedef EE_isr2_prio      ISR2Prio;
typedef EE_service_id     OSServiceIdType;
typedef EE_array_index    ArrayIndex;
typedef EE_array_size     ArraySize;
typedef EE_BOOL           BoolType;
typedef EE_BOOL *         BoolTypeRef;
typedef EE_CORE_ID        CoreIdType;

typedef TaskType * TaskTypeRef;

typedef EE_status_type    StatusType;

#define E_OK                        ((StatusType)0U)
#define E_OS_ACCESS                 ((StatusType)1U)
#define E_OS_CALLEVEL               ((StatusType)2U)
#define E_OS_ID                     ((StatusType)3U)
#define E_OS_LIMIT                  ((StatusType)4U)
#define E_OS_NOFUNC                 ((StatusType)5U)
#define E_OS_RESOURCE               ((StatusType)6U)
#define E_OS_STATE                  ((StatusType)7U)
#define E_OS_VALUE                  ((StatusType)8U)
#define E_OS_SERVICEID              ((StatusType)9U)
#define E_OS_ILLEGAL_ADDRESS        ((StatusType)10U)
#define E_OS_MISSINGEND             ((StatusType)11U)
#define E_OS_DISABLEDINT            ((StatusType)12U)
#define E_OS_STACKFAULT             ((StatusType)13U)
#define E_OS_PARAM_POINTER          ((StatusType)14U)
#define E_OS_PROTECTION_MEMORY      ((StatusType)15U)
#define E_OS_PROTECTION_TIME        ((StatusType)16U)
#define E_OS_PROTECTION_ARRIVAL     ((StatusType)17U)
#define E_OS_PROTECTION_LOCKED      ((StatusType)18U)
#define E_OS_PROTECTION_EXCEPTION   ((StatusType)19U)
/* Spinlocks errors */
#define E_OS_SPINLOCK               ((StatusType)20U)
#define E_OS_INTERFERENCE_DEADLOCK  ((StatusType)21U)
#define E_OS_NESTING_DEADLOCK       ((StatusType)22U)
/* RPC errors */
#define E_OS_CORE                   ((StatusType)23U)

/* Implementation specific errors: they must start with E_OS_SYS_ */
/* Error during StartOS */
#define E_OS_SYS_INIT               ((StatusType)24U)

#define INVALID_TASK_ID ((EE_task_id)-1)
#define INVALID_INDEX   ((EE_array_index)-1)
#define SYSTEM_STACK    ((EE_mem_size) -1)

/* Non corretto, ma ok per ora */
typedef EE_task_type TaskExecutionType;

#define TASK_FUNC(Task) Task##Func
#ifndef EE_TRACE_KERNEL
#define TASK(Task) void TASK_FUNC(Task) ( void )
#else
#define TASK(Task)                                  \
static void EE_S_J(TASK_FUNC(Task),Body) ( void );  \
void TASK_FUNC(Task) ( void ) {                     \
  mppa_tracepoint(erika,CONTEXT_SWITCH_EXIT);       \
  EE_S_J(TASK_FUNC(Task),Body)();                   \
}                                                   \
static void EE_S_J(TASK_FUNC(Task),Body) ( void )
#endif /* !EE_TRACE_KERNEL */

/* Semaphore forward declaration */
struct EE_type_sem_tag;
typedef struct EE_type_sem_tag EE_type_sem;
/* User semaphore types. */
typedef EE_type_sem  SemType;
typedef SemType *    SemRefType;

typedef EE_UREG                   CountType;
typedef EE_list_base              List;
typedef EE_linear_priority_queue  LinearPriorityQueue;

typedef struct EE_type_sem_tag {
  CountType     count;
  ArrayIndex    blocked_queue;
} EE_type_sem;

#endif /* !EE_API_TYPES_H_ */
