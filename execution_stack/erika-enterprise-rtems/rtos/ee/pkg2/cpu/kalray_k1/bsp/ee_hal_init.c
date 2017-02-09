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
#include <assert.h>

#define EE_K1_CORE_LAST  (EE_K1_PE_NUMBER - 1U)
#define EE_K1_STACKS_SIZE EE_STACK_WORD_LEGHT(EE_COREX_STACK_SECTION_SIZE)
#define EE_K1_STACKS_B  (EE_K1_STACKS_SIZE - 1U)

#define EE_STATIC static

EE_STATIC EE_STACK_T EE_STACK_ATTRIB
  ee_task_stacks_matrix[EE_K1_PE_NUMBER][EE_K1_STACKS_SIZE] =
    {[0 ... EE_K1_CORE_LAST] = {[0 ... EE_K1_STACKS_B] = EE_FILL_PATTERN}};

/* It would be really wordy initialize these... */
EE_STATIC EE_HDB  ee_hdb_core0_array[EE_CORE0_TCB_ARRAY_SIZE];
EE_STATIC EE_TA   ee_ta_core0_array[EE_CORE0_TCB_ARRAY_SIZE];
EE_STATIC EE_SDB  ee_sdb_core0_array[EE_CORE0_TCB_ARRAY_SIZE];
EE_STATIC EE_SCB  ee_scb_core0_array[EE_CORE0_TCB_ARRAY_SIZE];

EE_STATIC EE_HDB  ee_hdb_core1_array[EE_CORE1_TCB_ARRAY_SIZE];
EE_STATIC EE_TA   ee_ta_core1_array[EE_CORE1_TCB_ARRAY_SIZE];
EE_STATIC EE_SDB  ee_sdb_core1_array[EE_CORE1_TCB_ARRAY_SIZE];
EE_STATIC EE_SCB  ee_scb_core1_array[EE_CORE1_TCB_ARRAY_SIZE];

EE_STATIC EE_HDB  ee_hdb_core2_array[EE_CORE2_TCB_ARRAY_SIZE];
EE_STATIC EE_TA   ee_ta_core2_array[EE_CORE2_TCB_ARRAY_SIZE];
EE_STATIC EE_SDB  ee_sdb_core2_array[EE_CORE2_TCB_ARRAY_SIZE];
EE_STATIC EE_SCB  ee_scb_core2_array[EE_CORE2_TCB_ARRAY_SIZE];

EE_STATIC EE_HDB  ee_hdb_core3_array[EE_CORE3_TCB_ARRAY_SIZE];
EE_STATIC EE_TA   ee_ta_core3_array[EE_CORE3_TCB_ARRAY_SIZE];
EE_STATIC EE_SDB  ee_sdb_core3_array[EE_CORE3_TCB_ARRAY_SIZE];
EE_STATIC EE_SCB  ee_scb_core3_array[EE_CORE3_TCB_ARRAY_SIZE];

EE_STATIC EE_HDB  ee_hdb_core4_array[EE_CORE4_TCB_ARRAY_SIZE];
EE_STATIC EE_TA   ee_ta_core4_array[EE_CORE4_TCB_ARRAY_SIZE];
EE_STATIC EE_SDB  ee_sdb_core4_array[EE_CORE4_TCB_ARRAY_SIZE];
EE_STATIC EE_SCB  ee_scb_core4_array[EE_CORE4_TCB_ARRAY_SIZE];

EE_STATIC EE_HDB  ee_hdb_core5_array[EE_CORE5_TCB_ARRAY_SIZE];
EE_STATIC EE_TA   ee_ta_core5_array[EE_CORE5_TCB_ARRAY_SIZE];
EE_STATIC EE_SDB  ee_sdb_core5_array[EE_CORE5_TCB_ARRAY_SIZE];
EE_STATIC EE_SCB  ee_scb_core5_array[EE_CORE5_TCB_ARRAY_SIZE];

EE_STATIC EE_HDB  ee_hdb_core6_array[EE_CORE6_TCB_ARRAY_SIZE];
EE_STATIC EE_TA   ee_ta_core6_array[EE_CORE6_TCB_ARRAY_SIZE];
EE_STATIC EE_SDB  ee_sdb_core6_array[EE_CORE6_TCB_ARRAY_SIZE];
EE_STATIC EE_SCB  ee_scb_core6_array[EE_CORE6_TCB_ARRAY_SIZE];

EE_STATIC EE_HDB  ee_hdb_core7_array[EE_CORE7_TCB_ARRAY_SIZE];
EE_STATIC EE_TA   ee_ta_core7_array[EE_CORE7_TCB_ARRAY_SIZE];
EE_STATIC EE_SDB  ee_sdb_core7_array[EE_CORE7_TCB_ARRAY_SIZE];
EE_STATIC EE_SCB  ee_scb_core7_array[EE_CORE7_TCB_ARRAY_SIZE];

EE_STATIC EE_HDB  ee_hdb_core8_array[EE_CORE8_TCB_ARRAY_SIZE];
EE_STATIC EE_TA   ee_ta_core8_array[EE_CORE8_TCB_ARRAY_SIZE];
EE_STATIC EE_SDB  ee_sdb_core8_array[EE_CORE8_TCB_ARRAY_SIZE];
EE_STATIC EE_SCB  ee_scb_core8_array[EE_CORE8_TCB_ARRAY_SIZE];

EE_STATIC EE_HDB  ee_hdb_core9_array[EE_CORE9_TCB_ARRAY_SIZE];
EE_STATIC EE_TA   ee_ta_core9_array[EE_CORE9_TCB_ARRAY_SIZE];
EE_STATIC EE_SDB  ee_sdb_core9_array[EE_CORE9_TCB_ARRAY_SIZE];
EE_STATIC EE_SCB  ee_scb_core9_array[EE_CORE9_TCB_ARRAY_SIZE];

EE_STATIC EE_HDB  ee_hdb_core10_array[EE_CORE10_TCB_ARRAY_SIZE];
EE_STATIC EE_TA   ee_ta_core10_array[EE_CORE10_TCB_ARRAY_SIZE];
EE_STATIC EE_SDB  ee_sdb_core10_array[EE_CORE10_TCB_ARRAY_SIZE];
EE_STATIC EE_SCB  ee_scb_core10_array[EE_CORE10_TCB_ARRAY_SIZE];

EE_STATIC EE_HDB  ee_hdb_core11_array[EE_CORE11_TCB_ARRAY_SIZE];
EE_STATIC EE_TA   ee_ta_core11_array[EE_CORE11_TCB_ARRAY_SIZE];
EE_STATIC EE_SDB  ee_sdb_core11_array[EE_CORE11_TCB_ARRAY_SIZE];
EE_STATIC EE_SCB  ee_scb_core11_array[EE_CORE11_TCB_ARRAY_SIZE];

EE_STATIC EE_HDB  ee_hdb_core12_array[EE_CORE12_TCB_ARRAY_SIZE];
EE_STATIC EE_TA   ee_ta_core12_array[EE_CORE12_TCB_ARRAY_SIZE];
EE_STATIC EE_SDB  ee_sdb_core12_array[EE_CORE12_TCB_ARRAY_SIZE];
EE_STATIC EE_SCB  ee_scb_core12_array[EE_CORE12_TCB_ARRAY_SIZE];

EE_STATIC EE_HDB  ee_hdb_core13_array[EE_CORE13_TCB_ARRAY_SIZE];
EE_STATIC EE_TA   ee_ta_core13_array[EE_CORE13_TCB_ARRAY_SIZE];
EE_STATIC EE_SDB  ee_sdb_core13_array[EE_CORE13_TCB_ARRAY_SIZE];
EE_STATIC EE_SCB  ee_scb_core13_array[EE_CORE13_TCB_ARRAY_SIZE];

EE_STATIC EE_HDB  ee_hdb_core14_array[EE_CORE14_TCB_ARRAY_SIZE];
EE_STATIC EE_TA   ee_ta_core14_array[EE_CORE14_TCB_ARRAY_SIZE];
EE_STATIC EE_SDB  ee_sdb_core14_array[EE_CORE14_TCB_ARRAY_SIZE];
EE_STATIC EE_SCB  ee_scb_core14_array[EE_CORE14_TCB_ARRAY_SIZE];

EE_STATIC EE_HDB  ee_hdb_core15_array[EE_CORE15_TCB_ARRAY_SIZE];
EE_STATIC EE_TA   ee_ta_core15_array[EE_CORE15_TCB_ARRAY_SIZE];
EE_STATIC EE_SDB  ee_sdb_core15_array[EE_CORE15_TCB_ARRAY_SIZE];
EE_STATIC EE_SCB  ee_scb_core15_array[EE_CORE15_TCB_ARRAY_SIZE];

EE_STATIC EE_HDB  ee_hdb_rm_array[EE_CORE_RM_TCB_ARRAY_SIZE];
EE_STATIC EE_TA   ee_ta_rm_array[EE_CORE_RM_TCB_ARRAY_SIZE];
EE_STATIC EE_SDB  ee_sdb_rm_array[EE_CORE_RM_TCB_ARRAY_SIZE];
EE_STATIC EE_SCB  ee_scb_rm_array[EE_CORE_RM_TCB_ARRAY_SIZE];

/* Stack Descriptor 0 is reserved to System Stack */
EE_HPB ee_k1_pools[EE_K1_CORE_NUMBER] = {
  {
    .pool_base                   = &ee_task_stacks_matrix[0U][EE_K1_STACKS_B],
    .residual_mem                = EE_COREX_STACK_SECTION_SIZE,
    .p_sdb_array                 = &ee_sdb_core0_array[0U],
    .p_scb_array                 = &ee_scb_core0_array[0U],
    .stack_descriptor_size       = EE_CORE0_TCB_ARRAY_SIZE,
    .free_stack_descriptor_index = 1U,
    .p_hdb_array                 = &ee_hdb_core0_array[0U],
    .p_ta_array                  = &ee_ta_core0_array[0U],
    .hal_descriptor_size         = EE_CORE0_TCB_ARRAY_SIZE,
    .free_hal_descriptor_index   = 0U
  },
  {
    .pool_base                   = &ee_task_stacks_matrix[1U][EE_K1_STACKS_B],
    .residual_mem                = EE_COREX_STACK_SECTION_SIZE,
    .p_sdb_array                 = &ee_sdb_core1_array[0U],
    .p_scb_array                 = &ee_scb_core1_array[0U],
    .stack_descriptor_size       = EE_CORE1_TCB_ARRAY_SIZE,
    .free_stack_descriptor_index = 1U,
    .p_hdb_array                 = &ee_hdb_core1_array[0U],
    .p_ta_array                  = &ee_ta_core1_array[0U],
    .hal_descriptor_size         = EE_CORE1_TCB_ARRAY_SIZE,
    .free_hal_descriptor_index   = 0U
  },
  {
    .pool_base                   = &ee_task_stacks_matrix[2U][EE_K1_STACKS_B],
    .residual_mem                = EE_COREX_STACK_SECTION_SIZE,
    .p_sdb_array                 = &ee_sdb_core2_array[0U],
    .p_scb_array                 = &ee_scb_core2_array[0U],
    .stack_descriptor_size       = EE_CORE2_TCB_ARRAY_SIZE,
    .free_stack_descriptor_index = 1U,
    .p_hdb_array                 = &ee_hdb_core2_array[0U],
    .p_ta_array                  = &ee_ta_core2_array[0U],
    .hal_descriptor_size         = EE_CORE2_TCB_ARRAY_SIZE,
    .free_hal_descriptor_index   = 0U
  },
  {
    .pool_base                   = &ee_task_stacks_matrix[3U][EE_K1_STACKS_B],
    .residual_mem                = EE_COREX_STACK_SECTION_SIZE,
    .p_sdb_array                 = &ee_sdb_core3_array[0U],
    .p_scb_array                 = &ee_scb_core3_array[0U],
    .stack_descriptor_size       = EE_CORE3_TCB_ARRAY_SIZE,
    .free_stack_descriptor_index = 1U,
    .p_hdb_array                 = &ee_hdb_core3_array[0U],
    .p_ta_array                  = &ee_ta_core3_array[0U],
    .hal_descriptor_size         = EE_CORE3_TCB_ARRAY_SIZE,
    .free_hal_descriptor_index   = 0U
  },
  {
    .pool_base                   = &ee_task_stacks_matrix[4U][EE_K1_STACKS_B],
    .residual_mem                = EE_COREX_STACK_SECTION_SIZE,
    .p_sdb_array                 = &ee_sdb_core4_array[0U],
    .p_scb_array                 = &ee_scb_core4_array[0U],
    .stack_descriptor_size       = EE_CORE4_TCB_ARRAY_SIZE,
    .free_stack_descriptor_index = 1U,
    .p_hdb_array                 = &ee_hdb_core4_array[0U],
    .p_ta_array                  = &ee_ta_core4_array[0U],
    .hal_descriptor_size         = EE_CORE4_TCB_ARRAY_SIZE,
    .free_hal_descriptor_index   = 0U
  },
  {
    .pool_base                   = &ee_task_stacks_matrix[5U][EE_K1_STACKS_B],
    .residual_mem                = EE_COREX_STACK_SECTION_SIZE,
    .p_sdb_array                 = &ee_sdb_core5_array[0U],
    .p_scb_array                 = &ee_scb_core5_array[0U],
    .stack_descriptor_size       = EE_CORE5_TCB_ARRAY_SIZE,
    .free_stack_descriptor_index = 1U,
    .p_hdb_array                 = &ee_hdb_core5_array[0U],
    .p_ta_array                  = &ee_ta_core5_array[0U],
    .hal_descriptor_size         = EE_CORE5_TCB_ARRAY_SIZE,
    .free_hal_descriptor_index   = 0U
  },
  {
    .pool_base                   = &ee_task_stacks_matrix[6U][EE_K1_STACKS_B],
    .residual_mem                = EE_COREX_STACK_SECTION_SIZE,
    .p_sdb_array                 = &ee_sdb_core6_array[0U],
    .p_scb_array                 = &ee_scb_core6_array[0U],
    .stack_descriptor_size       = EE_CORE6_TCB_ARRAY_SIZE,
    .free_stack_descriptor_index = 1U,
    .p_hdb_array                 = &ee_hdb_core6_array[0U],
    .p_ta_array                  = &ee_ta_core6_array[0U],
    .hal_descriptor_size         = EE_CORE6_TCB_ARRAY_SIZE,
    .free_hal_descriptor_index   = 0U
  },
  {
    .pool_base                   = &ee_task_stacks_matrix[7U][EE_K1_STACKS_B],
    .residual_mem                = EE_COREX_STACK_SECTION_SIZE,
    .p_sdb_array                 = &ee_sdb_core7_array[0U],
    .p_scb_array                 = &ee_scb_core7_array[0U],
    .stack_descriptor_size       = EE_CORE7_TCB_ARRAY_SIZE,
    .free_stack_descriptor_index = 1U,
    .p_hdb_array                 = &ee_hdb_core7_array[0U],
    .p_ta_array                  = &ee_ta_core7_array[0U],
    .hal_descriptor_size         = EE_CORE7_TCB_ARRAY_SIZE,
    .free_hal_descriptor_index   = 0U
  },
  {
    .pool_base                   = &ee_task_stacks_matrix[8U][EE_K1_STACKS_B],
    .residual_mem                = EE_COREX_STACK_SECTION_SIZE,
    .p_sdb_array                 = &ee_sdb_core8_array[0U],
    .p_scb_array                 = &ee_scb_core8_array[0U],
    .stack_descriptor_size       = EE_CORE8_TCB_ARRAY_SIZE,
    .free_stack_descriptor_index = 1U,
    .p_hdb_array                 = &ee_hdb_core8_array[0U],
    .p_ta_array                  = &ee_ta_core8_array[0U],
    .hal_descriptor_size         = EE_CORE8_TCB_ARRAY_SIZE,
    .free_hal_descriptor_index   = 0U
  },
  {
    .pool_base                   = &ee_task_stacks_matrix[9U][EE_K1_STACKS_B],
    .residual_mem                = EE_COREX_STACK_SECTION_SIZE,
    .p_sdb_array                 = &ee_sdb_core9_array[0U],
    .p_scb_array                 = &ee_scb_core9_array[0U],
    .stack_descriptor_size       = EE_CORE9_TCB_ARRAY_SIZE,
    .free_stack_descriptor_index = 1U,
    .p_hdb_array                 = &ee_hdb_core9_array[0U],
    .p_ta_array                  = &ee_ta_core9_array[0U],
    .hal_descriptor_size         = EE_CORE9_TCB_ARRAY_SIZE,
    .free_hal_descriptor_index   = 0U
  },
  {
    .pool_base                   = &ee_task_stacks_matrix[10U][EE_K1_STACKS_B],
    .residual_mem                = EE_COREX_STACK_SECTION_SIZE,
    .p_sdb_array                 = &ee_sdb_core10_array[0U],
    .p_scb_array                 = &ee_scb_core10_array[0U],
    .stack_descriptor_size       = EE_CORE10_TCB_ARRAY_SIZE,
    .free_stack_descriptor_index = 1U,
    .p_hdb_array                 = &ee_hdb_core10_array[0U],
    .p_ta_array                  = &ee_ta_core10_array[0U],
    .hal_descriptor_size         = EE_CORE10_TCB_ARRAY_SIZE,
    .free_hal_descriptor_index   = 0U
  },
  {
    .pool_base                   = &ee_task_stacks_matrix[11U][EE_K1_STACKS_B],
    .residual_mem                = EE_COREX_STACK_SECTION_SIZE,
    .p_sdb_array                 = &ee_sdb_core11_array[0U],
    .p_scb_array                 = &ee_scb_core11_array[0U],
    .stack_descriptor_size       = EE_CORE11_TCB_ARRAY_SIZE,
    .free_stack_descriptor_index = 1U,
    .p_hdb_array                 = &ee_hdb_core11_array[0U],
    .p_ta_array                  = &ee_ta_core11_array[0U],
    .hal_descriptor_size         = EE_CORE11_TCB_ARRAY_SIZE,
    .free_hal_descriptor_index   = 0U
  },
  {
    .pool_base                   = &ee_task_stacks_matrix[12U][EE_K1_STACKS_B],
    .residual_mem                = EE_COREX_STACK_SECTION_SIZE,
    .p_sdb_array                 = &ee_sdb_core12_array[0U],
    .p_scb_array                 = &ee_scb_core12_array[0U],
    .stack_descriptor_size       = EE_CORE12_TCB_ARRAY_SIZE,
    .free_stack_descriptor_index = 1U,
    .p_hdb_array                 = &ee_hdb_core12_array[0U],
    .p_ta_array                  = &ee_ta_core12_array[0U],
    .hal_descriptor_size         = EE_CORE12_TCB_ARRAY_SIZE,
    .free_hal_descriptor_index   = 0U
  },
  {
    .pool_base                   = &ee_task_stacks_matrix[13U][EE_K1_STACKS_B],
    .residual_mem                = EE_COREX_STACK_SECTION_SIZE,
    .p_sdb_array                 = &ee_sdb_core13_array[0U],
    .p_scb_array                 = &ee_scb_core13_array[0U],
    .stack_descriptor_size       = EE_CORE13_TCB_ARRAY_SIZE,
    .free_stack_descriptor_index = 1U,
    .p_hdb_array                 = &ee_hdb_core13_array[0U],
    .p_ta_array                  = &ee_ta_core13_array[0U],
    .hal_descriptor_size         = EE_CORE13_TCB_ARRAY_SIZE,
    .free_hal_descriptor_index   = 0U
  },
  {
    .pool_base                   = &ee_task_stacks_matrix[14U][EE_K1_STACKS_B],
    .residual_mem                = EE_COREX_STACK_SECTION_SIZE,
    .p_sdb_array                 = &ee_sdb_core14_array[0U],
    .p_scb_array                 = &ee_scb_core14_array[0U],
    .stack_descriptor_size       = EE_CORE14_TCB_ARRAY_SIZE,
    .free_stack_descriptor_index = 1U,
    .p_hdb_array                 = &ee_hdb_core14_array[0U],
    .p_ta_array                  = &ee_ta_core14_array[0U],
    .hal_descriptor_size         = EE_CORE14_TCB_ARRAY_SIZE,
    .free_hal_descriptor_index   = 0U
  },
  {
    .pool_base                   = &ee_task_stacks_matrix[15U][EE_K1_STACKS_B],
    .residual_mem                = EE_COREX_STACK_SECTION_SIZE,
    .p_sdb_array                 = &ee_sdb_core15_array[0U],
    .p_scb_array                 = &ee_scb_core15_array[0U],
    .stack_descriptor_size       = EE_CORE15_TCB_ARRAY_SIZE,
    .free_stack_descriptor_index = 1U,
    .p_hdb_array                 = &ee_hdb_core15_array[0U],
    .p_ta_array                  = &ee_ta_core15_array[0U],
    .hal_descriptor_size         = EE_CORE15_TCB_ARRAY_SIZE,
    .free_hal_descriptor_index   = 0U
  },
  {
    .pool_base                   = NULL,
    .residual_mem                = 0U,
    .p_sdb_array                 = &ee_sdb_rm_array[0U],
    .p_scb_array                 = &ee_scb_rm_array[0U],
    .stack_descriptor_size       = EE_CORE_RM_TCB_ARRAY_SIZE,
    .free_stack_descriptor_index = 1U,
    .p_hdb_array                 = &ee_hdb_rm_array[0U],
    .p_ta_array                  = &ee_ta_rm_array[0U],
    .hal_descriptor_size         = EE_CORE_RM_TCB_ARRAY_SIZE,
    .free_hal_descriptor_index   = 0U
  },
};

/* This handle only the system stack initialization + iirq initialization */
void EE_hal_init ( void ) {
  EE_CORE_ID const core_id = EE_get_curr_core_id();
  EE_HPB * const p_hpb = &ee_k1_pools[core_id];
  EE_stack_context * const sp = EE_get_SP();

  /* Il descrittore in posizione 0 descrive lo Stack di sistema (PE) */
  p_hpb->p_sdb_array[0].bos = sp;
  p_hpb->p_sdb_array[0].stack_size = (EE_mem_size)(-1);
  p_hpb->p_scb_array[0].tos = sp;
  /* Tie Idle TASK with main Stack */
  p_hpb->p_hdb_array[0].p_SDB = &p_hpb->p_sdb_array[0];

  if ( core_id != EE_K1_BOOT_CORE ) {
#if 0
    EE_TID task_id;
    EE_status_type status_type = EE_create_task(core_id, &task_id,
      EE_TASK_TYPE_ISR2, EE_iirq_handler, EE_ISR_PRI_MAX, EE_ISR_PRI_MAX, 1,
      SYSTEM_STACK );
    if ( status_type == E_OK ) {
      EE_hal_set_isr2_source (task_id, EE_ISR_ID_IIRQ0, EE_ISR_PRI_MAX);
    } else {
      assert(false);
    }
#endif /* 0 */
    EE_k1_clear_irq_source(EE_ISR_ID_IIRQ0);
    EE_k1_activate_irq_source(EE_ISR_ID_IIRQ0, EE_ISR_PRI_MAX);
  }
}

static EE_ADDR EE_hal_allocate_stack ( EE_CORE_ID core_id,
  EE_mem_size* p_stack_size )
{
  EE_ADDR p_stack;
  EE_HPB * const p_pool = &ee_k1_pools[core_id];
  (*p_stack_size) = ((*p_stack_size) + EE_STACK_GUARD_AREA +
    (EE_STACK_ALIGN_SIZE - 1U)) & EE_STACK_ALIGN;
  if ( p_pool->residual_mem >= (*p_stack_size) ) {
    p_stack = (EE_STACK_T *)((char *)p_pool->pool_base -
      (ptrdiff_t)(EE_STACK_GUARD_AREA));
    p_pool->pool_base    -= (ptrdiff_t)(*p_stack_size);
    p_pool->residual_mem -= (*p_stack_size);
  } else {
    p_stack = NULL;
  }
  return p_stack;
}

EE_BOOL EE_hal_hdb_init ( EE_CORE_ID core_id, EE_HDB **  pp_HDB,
  EE_mem_size stackSize )
{
  EE_BOOL ret_val;
  EE_HPB  * const p_pool = &ee_k1_pools[core_id];
  if ( p_pool->free_hal_descriptor_index < p_pool->hal_descriptor_size )
  {
    if ( stackSize == SYSTEM_STACK ) {
      (*pp_HDB) = &p_pool->p_hdb_array[p_pool->free_hal_descriptor_index++];
      (*pp_HDB)->p_SDB = &p_pool->p_sdb_array[0];

      ret_val = EE_TRUE;
    } else {
      if ( p_pool->free_stack_descriptor_index < p_pool->stack_descriptor_size )
      {
        EE_stack_context * p_stack = EE_hal_allocate_stack(core_id, &stackSize);
        if ( p_stack != NULL ) {
          /* Alloca il descrittore di Stack */
          EE_array_index sdi = p_pool->free_stack_descriptor_index++;
          (*pp_HDB) = &p_pool->p_hdb_array[p_pool->free_hal_descriptor_index++];
          /* The following set the link between TASK and Stack */
          (*pp_HDB)->p_SDB = &p_pool->p_sdb_array[sdi];
          p_pool->p_sdb_array[sdi].bos        = p_stack;
          p_pool->p_sdb_array[sdi].stack_size = stackSize;
          p_pool->p_scb_array[sdi].tos        = p_stack;

          ret_val = EE_TRUE;
        } else {
          ret_val = EE_FALSE;
        }
      } else {
        ret_val = EE_FALSE;
      }
    }
  } else {
    ret_val = EE_FALSE;
  }
  return ret_val;
}

void EE_os_init ( void ) {
  typedef struct {
    EE_TCB *            tcb_array;
    EE_array_size       tcb_array_size;
    EE_schedule_data *  sd_array;
    EE_array_size       sd_array_size;
  } EE_core_info;

  EE_UREG i,j;
  EE_core_info const ee_core_info[EE_K1_CORE_NUMBER] = {
    {
      &ee_core0_tcb_array[0U].tcb, EE_CORE0_TCB_ARRAY_SIZE,
      &ee_core0_sd_array[0U], EE_CORE0_SD_ARRAY_SIZE
    },
    {
      &ee_core1_tcb_array[0U].tcb, EE_CORE1_TCB_ARRAY_SIZE,
      &ee_core1_sd_array[0U], EE_CORE1_SD_ARRAY_SIZE
    },
    {
      &ee_core2_tcb_array[0U].tcb, EE_CORE2_TCB_ARRAY_SIZE,
      &ee_core2_sd_array[0U], EE_CORE2_SD_ARRAY_SIZE
    },
    {
      &ee_core3_tcb_array[0U].tcb, EE_CORE3_TCB_ARRAY_SIZE,
      &ee_core3_sd_array[0U], EE_CORE3_SD_ARRAY_SIZE
    },
    {
      &ee_core4_tcb_array[0U].tcb, EE_CORE4_TCB_ARRAY_SIZE,
      &ee_core4_sd_array[0U], EE_CORE4_SD_ARRAY_SIZE
    },
    {
      &ee_core5_tcb_array[0U].tcb, EE_CORE5_TCB_ARRAY_SIZE,
      &ee_core5_sd_array[0U], EE_CORE5_SD_ARRAY_SIZE
    },
    {
      &ee_core6_tcb_array[0U].tcb, EE_CORE6_TCB_ARRAY_SIZE,
      &ee_core6_sd_array[0U], EE_CORE6_SD_ARRAY_SIZE
    },
    {
      &ee_core7_tcb_array[0U].tcb, EE_CORE7_TCB_ARRAY_SIZE,
      &ee_core7_sd_array[0U], EE_CORE7_SD_ARRAY_SIZE
    },
    {
      &ee_core8_tcb_array[0U].tcb, EE_CORE8_TCB_ARRAY_SIZE,
      &ee_core8_sd_array[0U], EE_CORE8_SD_ARRAY_SIZE
    },
    {
      &ee_core9_tcb_array[0U].tcb, EE_CORE9_TCB_ARRAY_SIZE,
      &ee_core9_sd_array[0U], EE_CORE9_SD_ARRAY_SIZE
    },
    {
      &ee_core10_tcb_array[0U].tcb, EE_CORE10_TCB_ARRAY_SIZE,
      &ee_core10_sd_array[0U], EE_CORE10_SD_ARRAY_SIZE
    },
    {
      &ee_core11_tcb_array[0U].tcb, EE_CORE11_TCB_ARRAY_SIZE,
      &ee_core11_sd_array[0U], EE_CORE11_SD_ARRAY_SIZE
    },
    {
      &ee_core12_tcb_array[0U].tcb, EE_CORE12_TCB_ARRAY_SIZE,
      &ee_core12_sd_array[0U], EE_CORE12_SD_ARRAY_SIZE
    },
    {
      &ee_core13_tcb_array[0U].tcb, EE_CORE13_TCB_ARRAY_SIZE,
      &ee_core13_sd_array[0U], EE_CORE13_SD_ARRAY_SIZE
    },
    {
      &ee_core14_tcb_array[0U].tcb, EE_CORE14_TCB_ARRAY_SIZE,
      &ee_core14_sd_array[0U], EE_CORE14_SD_ARRAY_SIZE
    },
    {
      &ee_core15_tcb_array[0U].tcb, EE_CORE15_TCB_ARRAY_SIZE,
      &ee_core15_sd_array[0U], EE_CORE15_SD_ARRAY_SIZE
    },
    {
      &ee_rm_tcb_array[0U].tcb, EE_CORE_RM_TCB_ARRAY_SIZE,
      &ee_rm_sd_array[0U], EE_CORE_RM_SD_ARRAY_SIZE
    },
  };

  /* Initialize Kernel Descriptor block */
  KDB_WJ.p_KCB_WJ = &KCB_WJ;
  KDB_WJ.kdb.p_KCB = &KCB_WJ.kcb;
  EE_k1_spin_init_lock(&KCB_WJ.lock);

  for ( i = 0U; i < EE_K1_CORE_NUMBER; ++i) {
    EE_CDB    * const p_CDB     = &KDB_WJ.core_descriptors[i];
    EE_CCB_WL * const p_CCB_wl  = &KCB_WJ.core_ctrls[i];
    EE_HPB    * const p_hpb     = &ee_k1_pools[i];

    EE_core_info const * const p_core_info   = &ee_core_info[i];

    /* Inizialize CDB */
    p_CDB->p_CCB          = &p_CCB_wl->ccb;
    p_CDB->tcb_array      = p_core_info->tcb_array;
    p_CDB->tcb_array_size = p_core_info->tcb_array_size;
    p_CDB->sd_array       = p_core_info->sd_array;
    p_CDB->sd_array_size  = p_core_info->sd_array_size;

    /* Initialize CCB_wl */
    EE_k1_spin_init_lock(&p_CCB_wl->lock);
    p_CCB_wl->ccb.curr_tid  = INVALID_TASK_ID;
    p_CCB_wl->ccb.rq_first  = INVALID_INDEX;
    p_CCB_wl->ccb.stk_first = INVALID_INDEX;
    /* p_CCB_wl->ccb.free_SD_index = 0U; Not Needed */

    /* Initialize Schedule Data */
    {
      EE_array_index const sd_last = (p_CDB->sd_array_size - 1U);
      for ( j = 0U; j < sd_last; ++j ) {
        EE_schedule_data * const p_schedule_data = &p_CDB->sd_array[j];
        p_schedule_data->tid  = INVALID_TASK_ID;
        p_schedule_data->next = (j + 1U);
      }
      p_CDB->sd_array[sd_last].tid   = INVALID_TASK_ID;
      p_CDB->sd_array[sd_last].next  = INVALID_INDEX;
    }

    /* Initialize HAL Description Blocks */
    /* Initialize Stack data structures */
    for ( j = 0U; j < p_hpb->hal_descriptor_size; ++j ) {
      p_hpb->p_hdb_array[j].p_TA = &p_hpb->p_ta_array[j];

      if ( j < p_hpb->stack_descriptor_size ) {
        p_hpb->p_sdb_array[j].p_SCB = &p_hpb->p_scb_array[j];
      }
    }
  }
}

