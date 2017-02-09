/*
 * ee_hal_internal.c
 *
 *  Created on: 07/dic/2014
 *      Author: AmministratoreErrico
 */

#include "eecfg.h"
#include "ee_k1_irq.h"
#include "ee_hal_internal.h"
#include "ee_kernel.h"
#include "ee_api_dynamic.h"
#include <assert.h>

#define EE_STATIC static

#define EE_K1_STACKS_SIZE EE_STACK_WORD_LEGHT(EE_STACKS_SECTION_SIZE)
#define EE_K1_STACKS_B  (EE_K1_STACKS_SIZE - 1U)

EE_STATIC EE_STACK_T EE_STACK_ATTRIB
  ee_task_stacks[EE_K1_STACKS_SIZE] =
    {[0 ... EE_K1_STACKS_B] = EE_FILL_PATTERN};

/* Stack Descriptor 0 is reserved to System Stack */
EE_HPB ee_k1_pool = {
  .pool_base        = &ee_task_stacks[EE_K1_STACKS_B],
  .residual_mem     = EE_STACKS_SECTION_SIZE,
};

/* This handle only the system stack initialization + iirq initialization */
void EE_hal_init ( void ) {
  EE_CORE_ID          const core_id = EE_get_curr_core_id();
  EE_KDB EE_CONST   * const p_KDB   = EE_get_kernel();
  EE_CDB EE_CONST   * const p_CDB   = EE_get_core(core_id);

  EE_TDB EE_CONST   * const p_TDB   =
    &p_KDB->tdb_array[EE_TASK_ARRAY_SIZE + core_id];

  EE_HDB EE_CONST   * const p_HDB   = &p_TDB->HDB;
  EE_CCB            * const p_CCB   = p_CDB->p_CCB;
  EE_TCB            * const p_TCB   = p_TDB->p_TCB;

  EE_SCB            * const p_SCB   =
    &ee_k1_pool.scb_array[EE_TASK_ARRAY_SIZE + core_id];

  EE_CTX            * const tos     = EE_get_SP();

  if ( p_CCB->os_started == EE_FALSE ) {
    p_HDB->p_SDB->bos         = tos;
    p_HDB->p_SDB->stack_size  = (EE_mem_size)(-1);
    p_SCB->tos                = tos;
    /* Tie Idle TASK with main Stack */
    p_HDB->p_SDB->p_SCB       = p_SCB;
    p_HDB->p_TA->stacked      = EE_TRUE;

    /* Fill TDB */
    p_TDB->p_TCB            = p_TCB;
    p_TDB->tid              = EE_TASK_ARRAY_SIZE + core_id;
    p_TDB->task_type        = EE_TASK_TYPE_IDLE;
    p_TDB->task_func        = NULL;
    p_TDB->dispatch_prio    = 0;
    p_TDB->ready_prio       = 0;
    p_TDB->orig_core_id     = core_id;

    /* Fill TCB */
    p_TCB->current_prio     = 0;
    p_TCB->current_core_id  = core_id;
    p_TCB->status           = EE_TASK_RUNNING;

    /* Fill CDB & CCB */
    p_CDB->p_idle_task  = p_TDB;
    p_CCB->p_curr       = p_TDB;
  }
}

static EE_ADDR EE_hal_allocate_stack ( EE_mem_size* p_stack_size )
{
  EE_ADDR p_stack;

  if ( ee_k1_pool.residual_mem >= (*p_stack_size) ) {
    (*p_stack_size) = ((*p_stack_size) + EE_STACK_GUARD_AREA +
    (EE_STACK_ALIGN_SIZE - 1U)) & EE_STACK_ALIGN;

    if ( ee_k1_pool.residual_mem >= (*p_stack_size) ) {
      p_stack = (EE_STACK_T *)((char *)ee_k1_pool.pool_base -
        (ptrdiff_t)(EE_STACK_GUARD_AREA));

      ee_k1_pool.pool_base     -= (ptrdiff_t)(*p_stack_size);
      ee_k1_pool.residual_mem  -= (*p_stack_size);
    } else {
      p_stack = NULL;
    }
  } else {
    p_stack = NULL;
  }
  return p_stack;
}

EE_BOOL EE_hal_hdb_init ( EE_CORE_ID core_id, EE_HDB * p_HDB,
  EE_mem_size stackSize )
{
  EE_BOOL  ret_val;
  EE_CTX * p_stack = EE_hal_allocate_stack(&stackSize);

  if ( stackSize == SYSTEM_STACK ) {
     /* Share the core global stack */
     p_HDB->p_SDB = EE_get_core(core_id)->p_idle_task->HDB.p_SDB;
     ret_val = EE_TRUE;
  } else {
    if ( p_stack != NULL ) {
      /* The following set the link between TASK and Stack */
      p_HDB->p_SDB->bos         = p_stack;
      p_HDB->p_SDB->p_SCB->tos  = p_stack;
      p_HDB->p_SDB->stack_size  = stackSize;

      ret_val = EE_TRUE;
    } else {
      ret_val = EE_FALSE;
    }
  }
  return ret_val;
}

void EE_os_init ( void ) {
  EE_UREG i;
  /* Initialize Kernel Descriptor block */
  KDB_WJ.p_KCB_WJ   = &KCB_WJ;
  KDB_WJ.kdb.p_KCB  = &KCB_WJ.kcb;
  EE_k1_spin_init_lock(&KCB_WJ.lock);

  /* Initialize Core Data Structures */
  for ( i = 0U; i < EE_K1_CORE_NUMBER; ++i ) {
    EE_k1_spin_init_lock(&KCB_WJ.core_ctrls[i].lock);
    KDB_WJ.core_descriptors[i].p_CCB = &KCB_WJ.core_ctrls[i].ccb;
  }

  /* Initialize the Task Description & Control Blocks (TDB & TCB) */
  for ( i = 0U; i < EE_ARRAY_ELEMENT_COUNT(KCB_WJ.kcb.tcb_array); ++i ) {
    ee_k1_pool.sdb_array[i].p_SCB         = &ee_k1_pool.scb_array[i];
    KDB_WJ.kdb.tdb_array[i].p_TCB         = &KCB_WJ.kcb.tcb_array[i];
    KDB_WJ.kdb.tdb_array[i].HDB.p_SDB     = &ee_k1_pool.sdb_array[i];
    KDB_WJ.kdb.tdb_array[i].HDB.p_TA      = &ee_k1_pool.ta_array[i];
  }

  /* Initialize the Scheduler Nodes (SN) Free Linked List */
  for ( i = 1U; i < EE_ARRAY_ELEMENT_COUNT(KCB_WJ.kcb.sn_array); ++i ) {
    KCB_WJ.kcb.sn_array[(i - 1U)].p_next = &KCB_WJ.kcb.sn_array[i];
  }
#if (defined(EE_SCHEDULER_GLOBAL))
  KCB_WJ.kcb.p_free_sn_first = &KCB_WJ.kcb.sn_array[0U];
#else
  {
    /* XXX: Arbitrary choice to try allocate allocate the same ammount of
            Scheduler data for Each core */
    ArrayIndex const core_sd_size = EE_ARRAY_ELEMENT_COUNT(KCB_WJ.kcb.sn_array);
    ArrayIndex const per_core_sd_size  = (core_sd_size / EE_K1_CORE_NUMBER);
    ArrayIndex const sd_offset_modulus = (core_sd_size % EE_K1_CORE_NUMBER);

    for ( i = (EE_K1_CORE_NUMBER - 1U); i > 0U; --i ) {
      KCB_WJ.core_ctrls[i].ccb.p_free_sn_first =
        &KCB_WJ.kcb.sn_array[(i * per_core_sd_size) + sd_offset_modulus];

      /* Initialize The Last element of the array associated to an array */
      KCB_WJ.core_ctrls[i].ccb.p_free_sn_first[per_core_sd_size - 1U].
        p_next = NULL;
    }
    /* I assign eventual offset to core 0 */
    KCB_WJ.core_ctrls[0].ccb.p_free_sn_first = &KCB_WJ.kcb.sn_array[0];
    /* Initialize The Last element of the array associated to an array */
    KCB_WJ.core_ctrls[0].ccb.
      p_free_sn_first[per_core_sd_size - 1U + sd_offset_modulus].p_next = NULL;
  }
#endif /* !EE_SCHEDULER_GLOBAL */
}
