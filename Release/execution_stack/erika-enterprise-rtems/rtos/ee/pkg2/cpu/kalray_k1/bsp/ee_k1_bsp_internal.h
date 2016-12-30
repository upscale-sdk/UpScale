#ifndef EE_K1_BSP_INTERNAL_H_
#define EE_K1_BSP_INTERNAL_H_

#include "ee_k1_bsp.h"
#include "ee_k1_bsp_communication.h"

#ifndef EE_START_STACK_SIZE
#define EE_START_STACK_SIZE  4096U
#endif /* EE_START_STACK_SIZE */

#define EE_START_WORD_STACK_SIZE (EE_START_STACK_SIZE/sizeof(__k1_uint64_t))

/*******************************************************************************
                               Cluster Startup
 ******************************************************************************/

/* Cluster manager procedure (it's running on RM core) */
void EE_k1_rm_task_server_setup ( void );
void EE_k1_rm_task_server ( void );
void EE_k1_rm_iirq0_handler ( int r0 __attribute__((unused)));
void EE_k1_rm_iirq1_handler ( int r0 __attribute__((unused)));
void EE_k1_rm_iirq2_handler ( int r0 __attribute__((unused)));
void EE_k1_rm_iirq3_handler ( int r0 __attribute__((unused)));


/** RM and PE core BSP initialization */
void EE_k1_cluster_initialize ( void );
void EE_k1_cluster_secondary_cpu_initialize ( void );
void EE_k1_secondary_cpu_startup ( void );

#define EE_K1_CHECK_RIGHT() do {\
  if ( __k1_get_cpu_id() != EE_K1_BOOT_CPU ) {\
    printf("processor %d is not the boot processor and attempt to execute a "\
      "protected function. At %s: %s: %d\n",\
      __k1_get_cpu_id(),__FILE__,__func__,__LINE__);\
    exit(1); \
  }\
}while(0)

#endif /* EE_K1_BSP_INTERNAL_H_ */
