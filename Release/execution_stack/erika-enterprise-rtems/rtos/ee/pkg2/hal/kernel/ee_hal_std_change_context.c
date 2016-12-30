/*
 * ee_hal_change_context.c
 *
 *  Created on: 07/dic/2014
 *      Author: AmministratoreErrico
 */

#include "ee_hal_internal.h"
#include "ee_scheduler.h"

void EE_hal_change_context ( EE_HDB * p_To, EE_task_func task_func,
  EE_HDB * p_From )
{
  EE_BOOL from_a_stacked, to_a_stacked;

  mppa_tracepoint(erika,CONTEXT_SWITCH_ENTER);
  from_a_stacked  = p_From->p_TA->stacked;
  to_a_stacked    = p_To->p_TA->stacked;

  if ( to_a_stacked ) {
    if ( from_a_stacked ) {
      EE_hal_save_ctx_and_restore_ctx(p_To->p_SDB, p_From->p_SDB);
    } else {
      EE_hal_restore_ctx(p_To->p_SDB);
    }
  } else {
    p_To->p_TA->stacked = EE_TRUE;
    if ( from_a_stacked ) {
      EE_hal_save_ctx_and_ready2stacked(p_To->p_SDB, task_func, p_From->p_SDB);
    } else {
      EE_hal_ready2stacked(p_To->p_SDB, task_func);
    }
  }

  mppa_tracepoint(erika,CONTEXT_SWITCH_EXIT);
}

void EE_hal_terminate_idle_task ( EE_HDB * p_Idle )
{
  EE_CTX *        p_CTX;
  EE_SDB *  const p_SDB = p_Idle->p_SDB;
  EE_SCB *  const p_SCB = p_SDB->p_SCB;
  EE_CTX *  const bos   = p_SDB->bos;
  EE_CTX *        tos   = p_SCB->tos;

  do {
    p_CTX = tos;
    tos   = tos->p_CTX;
  } while ( tos != bos );

  /* Unwind the stack until the last context*/
  p_SCB->tos = p_CTX;

  EE_hal_restore_ctx(p_SDB);
}
