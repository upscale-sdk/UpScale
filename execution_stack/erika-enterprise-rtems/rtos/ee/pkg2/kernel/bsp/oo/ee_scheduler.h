/*
 * ee_scheduler.h
 *
 *  Created on: 06/dic/2014
 *      Author: AmministratoreErrico
 */

#ifndef EE_SCHEDULER_H_
#define EE_SCHEDULER_H_

#include "eecfg.h"
#include "ee_platform_types.h"
#include "ee_utils.h"
#include "ee_basic_data_structures.h"
#include "ee_api_types.h"
#include "ee_hal_internal_types.h"
#include "ee_kernel_types.h"

/* Scheduler Entry Points */
void EE_scheduler_task_not_terminated ( void );
void EE_scheduler_task_end ( void );

EE_BOOL  EE_scheduler_task_activated ( EE_CDB EE_CONST * p_CDB,
    EE_TDB EE_CONST * p_task_activated );
EE_BOOL  EE_scheduler_task_current_is_preempted ( EE_CDB  EE_CONST * p_CDB );
void     EE_scheduler_task_set_running ( EE_CDB EE_CONST * p_CDB,
   EE_TDB EE_CONST * p_task );
EE_TDB EE_CONST * EE_scheduler_task_terminated ( EE_CDB EE_CONST * p_CDB );
EE_TDB EE_CONST * EE_scheduler_task_dispatch ( EE_CDB EE_CONST * p_CDB );
EE_TDB EE_CONST * EE_scheduler_task_block_current ( EE_CDB EE_CONST * p_CDB,
  EE_array_index * p_block_queue_head );
EE_BOOL  EE_scheduler_task_unblocked ( EE_CDB EE_CONST * p_CDB,
  EE_array_index * p_block_queue_head );
EE_TDB EE_CONST * EE_scheduler_task_schedule_next ( EE_CDB EE_CONST * p_CDB,
   EE_TDB EE_CONST ** pp_term_task );

#endif /* ! EE_SCHEDULER_H_ */
