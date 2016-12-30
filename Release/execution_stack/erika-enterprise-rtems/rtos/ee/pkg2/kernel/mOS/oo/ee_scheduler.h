/*
 * ee_scheduler.h
 *
 *  Created on: 06/dic/2014
 *      Author: AmministratoreErrico
 */

#ifndef EE_SCHEDULER_H_
#define EE_SCHEDULER_H_

#include "eecfg.h"
#include "ee_compiler.h"
#include "ee_platform_types.h"
#include "ee_utils.h"
#include "ee_basic_data_structures.h"
#include "ee_api_types.h"
#include "ee_hal_internal_types.h"
#include "ee_kernel_types.h"

#if (defined(EE_SCHEDULER_GLOBAL))
EE_INLINE__ EE_SN * EE_scheduler_sn_alloc ( EE_KCB * p_KCB ) {
  EE_SN * p_sn_allocated = p_KCB->p_free_sn_first;
  p_KCB->p_free_sn_first = p_sn_allocated->p_next;
  p_sn_allocated->p_next = NULL;
  return p_sn_allocated;
}

EE_INLINE__ void EE_scheduler_sn_release ( EE_KCB * p_KCB, EE_SN * p_to_free )
{
  p_to_free->p_next = p_KCB->p_free_sn_first;
  p_KCB->p_free_sn_first = p_to_free;
}
#else
EE_INLINE__ EE_SN * EE_scheduler_sn_alloc ( EE_CCB * p_CCB ) {
  EE_SN * p_sn_allocated = p_CCB->p_free_sn_first;
  p_CCB->p_free_sn_first = p_sn_allocated->p_next;
  p_sn_allocated->p_next = NULL;
  return p_sn_allocated;
}

EE_INLINE__ void EE_scheduler_sn_release ( EE_CCB * p_CCB, EE_SN * p_to_free )
{
  p_to_free->p_next = p_CCB->p_free_sn_first;
  p_CCB->p_free_sn_first = p_to_free;
}
#endif /* EE_SCHEDULER_GLOBAL */

/* Scheduler Entry Points */
void EE_scheduler_task_not_terminated ( void );
void EE_scheduler_task_end ( void );
void EE_scheduler_task_wrapper ( EE_task_func task_func );

EE_status_type  EE_scheduler_task_activated ( EE_KDB EE_CONST * p_KDB,
  EE_CDB EE_CONST * p_CDB,  EE_TDB EE_CONST * p_task_activated,
  EE_BOOL preemption_point);

void EE_scheduler_task_preemption_point ( EE_KDB EE_CONST * p_KDB,
  EE_CDB EE_CONST * p_CDB );

EE_TDB EE_CONST * EE_scheduler_task_block_current ( EE_KDB EE_CONST * p_KDB,
  EE_CDB EE_CONST * p_CDB, EE_SN EE_CONST ** pp_block_queue_head );

EE_BOOL  EE_scheduler_task_unblocked ( EE_KDB EE_CONST * p_KDB,
  EE_CDB EE_CONST * p_CDB, EE_SN EE_CONST * p_block_queue_head );

EE_TDB EE_CONST * EE_scheduler_task_schedule_next (
   EE_KDB EE_CONST * p_KDB, EE_CDB EE_CONST * p_CDB );

void  EE_scheduler_task_set_running ( EE_KDB EE_CONST * p_KDB,
  EE_CDB EE_CONST * p_CDB, EE_TDB EE_CONST * p_task );

EE_TDB EE_CONST * EE_scheduler_task_terminated ( EE_KDB EE_CONST * p_KDB,
  EE_CDB EE_CONST * p_CDB );

#endif /* ! EE_SCHEDULER_H_ */
