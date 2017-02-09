#ifndef EE_STD_CHANGE_CONTEXT_H_
#define EE_STD_CHANGE_CONTEXT_H_

#include "ee_platform_types.h"
#include "ee_hal.h"
#include "ee_hal_internal_types.h"

/*******************************************************************************
                        Standard HAL For Initialization
 ******************************************************************************/

void    EE_hal_init ( void );
#if (defined(K1_ERIKA_PLATFORM_BSP))
EE_BOOL EE_hal_hdb_init ( EE_CORE_ID core_id, EE_HDB ** pp_HDB,
  EE_mem_size stackSize );
#else
EE_BOOL EE_hal_hdb_init ( EE_CORE_ID core_id, EE_HDB * p_HDB,
  EE_mem_size stackSize );
#endif

/*******************************************************************************
                           Standard Context Change
 ******************************************************************************/
void    EE_hal_change_context ( EE_HDB * p_To, EE_task_func task_func, EE_HDB *
  p_From );

#if (defined(K1_ERIKA_PLATFORM_BSP))
#define EE_HAL_FROM_ISR2_ALLOC EE_INLINE__
#else
#define EE_HAL_FROM_ISR2_ALLOC
#endif

/* No portable context handling HAL */
EE_HAL_FROM_ISR2_ALLOC void EE_hal_change_context_from_isr2 ( EE_HDB * p_To,
  EE_task_func task_func, EE_HDB * p_From );

void EE_hal_save_ctx_and_restore_ctx ( EE_SDB * pToSDB, EE_SDB * pFromSDB );
void EE_hal_restore_ctx ( EE_SDB * pToSDB );
void EE_hal_save_ctx_and_ready2stacked ( EE_SDB * pToSDB, EE_task_func func,
  EE_SDB * pFromSDB);
void EE_hal_ready2stacked ( EE_SDB * pToSDB, EE_task_func func );
void EE_hal_terminate_ctx ( EE_SDB * pTermSDB, EE_task_func func );

EE_status_type EE_hal_set_isr2_source (EE_task_id task_id,
  EE_isr2_source_id source_id, EE_isr2_prio isr2_prio);

EE_INLINE__ void EE_hal_activation_started ( EE_HDB * p_Act ) {
  p_Act->p_TA->stacked   = EE_TRUE;
}

EE_INLINE__ void EE_hal_activation_terminated ( EE_HDB * p_Term ) {
  p_Term->p_TA->stacked  = EE_FALSE;
}

EE_INLINE__ void EE_hal_terminate_activation ( EE_HDB * p_Term,
  EE_kernel_callback kern_callback)
{
  EE_hal_terminate_ctx ( p_Term->p_SDB, kern_callback );
}

EE_INLINE__ void EE_hal_start_idle_task ( EE_HDB * p_Idle,
  EE_task_func idle_task_func )
{
  p_Idle->p_TA->stacked = EE_TRUE;
#if (defined(EE_API_DYNAMIC))
  p_Idle->p_SDB->bos = EE_get_SP();
  p_Idle->p_SDB->p_SCB->tos = p_Idle->p_SDB->bos;
#endif /* EE_API_DYNAMIC */
  EE_hal_save_ctx_and_ready2stacked(p_Idle->p_SDB, idle_task_func,
    p_Idle->p_SDB);
}

void EE_hal_terminate_idle_task ( EE_HDB * p_Idle );

#endif /* EE_STD_CHANGE_CONTEXT_H_ */
