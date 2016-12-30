#ifndef EE_INCLUDE_EECFG_H
#define EE_INCLUDE_EECFG_H

#if (defined(K1_ERIKA_PLATFORM_MOS))
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
#define EE_STACKS_SECTION_SIZE  (4 * 1024U * 16 + 512)

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
#define EE_CONF_LIBGOMP
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
#define EE_HAS_COMM_HOOK
#endif /* K1_ERIKA_PLATFORM_MOS */

#if (defined(K1_ERIKA_PLATFORM_BSP))

/* Deve essere maggiore o uguale al numero di TASK che si vuol creare */
#define EE_CORE0_TCB_ARRAY_SIZE      8U
/* Deve essere maggiore o uguale alla somma di tutte le attivazioni di TASK */
#define EE_CORE0_SD_ARRAY_SIZE       8U

#define EE_CORE1_TCB_ARRAY_SIZE      6U
#define EE_CORE1_SD_ARRAY_SIZE       6U

#define EE_CORE2_TCB_ARRAY_SIZE      6U
#define EE_CORE2_SD_ARRAY_SIZE       6U

#define EE_CORE3_TCB_ARRAY_SIZE      6U
#define EE_CORE3_SD_ARRAY_SIZE       6U

#define EE_CORE4_TCB_ARRAY_SIZE      6U
#define EE_CORE4_SD_ARRAY_SIZE       6U

#define EE_CORE5_TCB_ARRAY_SIZE      6U
#define EE_CORE5_SD_ARRAY_SIZE       6U

#define EE_CORE6_TCB_ARRAY_SIZE      6U
#define EE_CORE6_SD_ARRAY_SIZE       6U

#define EE_CORE7_TCB_ARRAY_SIZE      6U
#define EE_CORE7_SD_ARRAY_SIZE       6U

#define EE_CORE8_TCB_ARRAY_SIZE      6U
#define EE_CORE8_SD_ARRAY_SIZE       6U

#define EE_CORE9_TCB_ARRAY_SIZE      6U
#define EE_CORE9_SD_ARRAY_SIZE       6U

#define EE_CORE10_TCB_ARRAY_SIZE     6U
#define EE_CORE10_SD_ARRAY_SIZE      6U

#define EE_CORE11_TCB_ARRAY_SIZE     6U
#define EE_CORE11_SD_ARRAY_SIZE      6U

#define EE_CORE12_TCB_ARRAY_SIZE     6U
#define EE_CORE12_SD_ARRAY_SIZE      6U

#define EE_CORE13_TCB_ARRAY_SIZE     6U
#define EE_CORE13_SD_ARRAY_SIZE      6U

#define EE_CORE14_TCB_ARRAY_SIZE     6U
#define EE_CORE14_SD_ARRAY_SIZE      6U

#define EE_CORE15_TCB_ARRAY_SIZE     6U
#define EE_CORE15_SD_ARRAY_SIZE      6U

#define EE_CORE_RM_TCB_ARRAY_SIZE    1U
#define EE_CORE_RM_SD_ARRAY_SIZE     1U

/* Dimensione della regione degli stacks */
#define EE_COREX_STACK_SECTION_SIZE  4096U

/* Size dell'array a comune fra i cores, contenete tutti i TDB */
#define EE_TDB_ARRAY_SIZE  (EE_CORE0_TCB_ARRAY_SIZE + EE_CORE1_TCB_ARRAY_SIZE +\
  EE_CORE2_TCB_ARRAY_SIZE + EE_CORE3_TCB_ARRAY_SIZE + EE_CORE4_TCB_ARRAY_SIZE +\
  EE_CORE5_TCB_ARRAY_SIZE + EE_CORE6_TCB_ARRAY_SIZE + EE_CORE7_TCB_ARRAY_SIZE +\
  EE_CORE8_TCB_ARRAY_SIZE + EE_CORE9_TCB_ARRAY_SIZE +\
  EE_CORE10_TCB_ARRAY_SIZE + EE_CORE11_TCB_ARRAY_SIZE +\
  EE_CORE12_TCB_ARRAY_SIZE + EE_CORE13_TCB_ARRAY_SIZE +\
  EE_CORE14_TCB_ARRAY_SIZE + EE_CORE15_TCB_ARRAY_SIZE +\
  EE_CORE_RM_TCB_ARRAY_SIZE)

/* Size dell'array a comune fra i cores, contenete gli schedule data (SD) */
#define EE_SD_ARRAY_SIZE  (EE_CORE0_SD_ARRAY_SIZE + EE_CORE1_SD_ARRAY_SIZE +\
  EE_CORE2_SD_ARRAY_SIZE + EE_CORE3_SD_ARRAY_SIZE + EE_CORE4_SD_ARRAY_SIZE +\
  EE_CORE5_SD_ARRAY_SIZE + EE_CORE6_SD_ARRAY_SIZE + EE_CORE7_SD_ARRAY_SIZE +\
  EE_CORE8_SD_ARRAY_SIZE + EE_CORE9_SD_ARRAY_SIZE +\
  EE_CORE10_SD_ARRAY_SIZE + EE_CORE11_SD_ARRAY_SIZE +\
  EE_CORE12_SD_ARRAY_SIZE + EE_CORE13_SD_ARRAY_SIZE +\
  EE_CORE14_SD_ARRAY_SIZE + EE_CORE15_SD_ARRAY_SIZE +\
  EE_CORE_RM_SD_ARRAY_SIZE)

#define EE_USE_SCHEDULER_BASIC
#define EE_API_DYNAMIC
#define EE_API_EXTENSION

/* P-Socrated, Kalray specific macros */
//#define EE_CONF_LIBGOMP
#ifndef EE_CONF_LIBGOMP
#define EE_JOB_TEST
#ifdef  EE_JOB_TEST
//#define EE_JOB_TEST_FROM_RM
//#define GOMP_TEST
#define NORMAL_TEST
#endif /* EE_JOB_TEST */
#endif /* !EE_CONF_LIBGOMP */

//#define EE_NO_CACHE
//#define EE_TRACE_KERNEL
//#define EE_TRACE_TASK

#ifdef EE_TRACE_KERNEL
#define EE_USER_TRACEPOINT "test_trace.h"
#endif /* EE_TRACE_KERNEL */

/* Ticked protocols for locks */
#define EE_TICKED_LOCKS

#define EE_MAX_NUM_JOB  2U

#endif /* K1_ERIKA_PLATFORM_BSP */

#endif /* !EE_INCLUDE_EECFG_H */
