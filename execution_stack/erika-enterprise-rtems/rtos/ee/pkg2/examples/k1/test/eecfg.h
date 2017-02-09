#ifndef EE_INCLUDE_EECFG_H
#define EE_INCLUDE_EECFG_H

#define EE_PERCORE_TASK   8U
#define EE_PERCORE_SD     EE_PERCORE_TASK

#if defined(LIBMPPA)
#define EE_CORE_RM_TCB_ARRAY_SIZE    1U
#define EE_CORE_RM_SD_ARRAY_SIZE     1U
#else
#define EE_CORE_RM_TCB_ARRAY_SIZE    0U
#define EE_CORE_RM_SD_ARRAY_SIZE     0U
#endif

/* Dimensione della regione degli stacks */
#define EE_STACKS_SECTION_SIZE  (4096U * 16U)

/* Size dell'array a comune fra i cores, contenete tutti i TDB */
#define EE_TASK_ARRAY_SIZE  ( (EE_PERCORE_TASK * 16U) +\
  EE_CORE_RM_TCB_ARRAY_SIZE )

/* Size dell'array a comune fra i cores, contenete gli schedule data (SD) */
#define EE_SD_ARRAY_SIZE ( (EE_PERCORE_SD * 16U) +\
  EE_CORE_RM_SD_ARRAY_SIZE )

#define EE_USE_SCHEDULER_BASIC
#define EE_API_DYNAMIC
#define EE_API_EXTENSION

/* P-Socrated, Kalray specific macros */
//#define EE_CONF_LIBGOMP
#ifndef EE_CONF_LIBGOMP
#define EE_JOB_TEST
#ifndef EE_JOB_TEST
#define EE_SINGLECORE
#endif /* !EE_JOB_TEST */
#endif /* !EE_CONF_LIBGOMP */

//#define EE_NO_CACHE
//#define EE_TRACE_KERNEL
//#define EE_TRACE_TASK

#ifdef EE_TRACE_KERNEL
#define EE_USER_TRACEPOINT "test_trace.h"
#endif /* EE_TRACE_KERNEL */

/* Ticked protocols for locks */
#define EE_TICKED_LOCKS

#define EE_MAX_NUM_JOB  4U

/* To Enable Offload Hook */
//#define EE_HAS_COMM_HOOK

#endif /* !EE_INCLUDE_EECFG_H */
