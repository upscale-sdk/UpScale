/**
 * ERIKA Enterprise - a tiny RTOS for small microcontrollers
 *
 * Copyright (C) 2002-2014  Evidence Srl
 *
 * This file is part of ERIKA Enterprise.
 *
 * ERIKA Enterprise is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation,
 * (with a special exception described below).
 *
 * Linking this code statically or dynamically with other modules is
 * making a combined work based on this code.  Thus, the terms and
 * conditions of the GNU General Public License cover the whole
 * combination.
 *
 * As a special exception, the copyright holders of this library give you
 * permission to link this code with independent modules to produce an
 * executable, regardless of the license terms of these independent
 * modules, and to copy and distribute the resulting executable under
 * terms of your choice, provided that you also meet, for each linked
 * independent module, the terms and conditions of the license of that
 * module.  An independent module is a module which is not derived from
 * or based on this library.  If you modify this code, you may extend
 * this exception to your version of the code, but you are not
 * obligated to do so.  If you do not wish to do so, delete this
 * exception statement from your version.
 *
 * ERIKA Enterprise is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with ERIKA Enterprise; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

 /** @file   ee_hal_internal.h
  *  @brief  ISA-dependent internal part of HAL
  *  @author Errico Guidieri
  *  @date 2014
  */

#ifndef EE_HAL_INTERNAL_H_
#define EE_HAL_INTERNAL_H_

#include "ee_platform_types.h"
#include "ee_utils.h"
#include "ee_hal.h"
#include "ee_k1_irq.h"
#include "ee_hal_internal_types.h"
#include "ee_hal_std_change_context.h"
#include "ee_kernel_types.h"

/*******************************************************************************
                          Stack Utilities
 ******************************************************************************/
#define EE_STACK_ALIGN              0xFFFFFFF8U
#define EE_STACK_ALIGN_SIZE         8U
#define EE_STACK_ALIGN_INDEX        0xFFFFFFFEU
#define EE_STACK_SCRATCH_AREA_SIZE  16U
#define EE_STACK_SCRATCH_AREA_INDEX \
  (EE_STACK_SCRATCH_AREA_SIZE/sizeof(EE_STACK_T))

/* Used to place ERIKA Stacks in right section for memory protection and ORTI
   Stack filling, and handling stack alignment */
#define EE_STACK_ATTRIB EE_COMPILER_ALIGN(EE_STACK_ALIGN_SIZE) \
  EE_COMPILER_SECTION("ee_kernel_stack")

#define EE_STACK_TOS(stack) \
  ((EE_ADDR)&stack[((sizeof(stack)/sizeof(stack[0])) - 1U) & \
    EE_STACK_ALIGN_INDEX])

#define EE_STACK_BOS(stack)  ((EE_ADDR)&stack[0U])

EE_INLINE__ void EE_set_SP ( register EE_CONST_ADDR reg )
{
  __asm__ volatile ("copy $r12 = %0" EE_K1_END_ASM_BUNDLE : : "r"(reg));
}

/* Standard HAL init support */
#define EE_STACK_GUARD_AREA EE_STACK_SCRATCH_AREA_SIZE

/*******************************************************************************
                        Interrupt handling utilities
 ******************************************************************************/
/* Disable/Enable Interrupts */
EE_INLINE__ void EE_hal_disableIRQ( void )
{
  EE_k1_disableIRQ();
}

EE_INLINE__ void EE_hal_enableIRQ( void )
{
  EE_k1_enableIRQ();
}

/* Suspend/Resume Interrupts */
EE_INLINE__ EE_FREG EE_hal_suspendIRQ( void )
{
  return EE_k1_suspendIRQ();
}

EE_INLINE__ void EE_hal_resumeIRQ( EE_FREG flag )
{
  EE_k1_resumeIRQ(flag);
}

/*******************************************************************************
                    HAL For Primitives Synchronization
 ******************************************************************************/
/* Macro used to adjust flags dull variable with new priority,
   using count trailing zeros builtin */
#define EE_K1_ADJUST_FLAGS_WITH_NEW_PRIO(flags, prio) \
  (((flags) & (~(_K1_MASK_PS_IL))) | \
    ((prio) << __builtin_k1_ctz(_K1_MASK_PS_IL)))

/* Called as _first_ function of a primitive that can be called from within
 * an IRQ and from within a task. */
EE_INLINE__ EE_FREG EE_hal_begin_nested_primitive( void )
{
  return EE_hal_suspendIRQ();
}

/* Called as _last_ function of a primitive that can be called from
 * within an IRQ or a task. */
EE_INLINE__ void EE_hal_end_nested_primitive( EE_FREG flag )
{
  EE_hal_resumeIRQ(flag);
}

/* Used to get internal CPU priority. */
EE_INLINE__ EE_isr2_prio EE_hal_get_int_prio( void )
{
  return EE_k1_get_int_prio();
}

/* Used to set internal CPU priority. */
EE_INLINE__ void EE_hal_set_int_prio( EE_isr2_prio prio )
{
  EE_k1_set_int_prio(prio);
}

/*
 * Used to change internal CPU priority and return a status flags mask.
 *
 * Note:    EE_FREG param flags and return value needed only for according to
 *          HAL interface.
 */
EE_INLINE__ EE_FREG EE_hal_change_int_prio(
  EE_isr2_prio prio, EE_FREG flags )
{
  EE_k1_set_int_prio(prio);
  return EE_K1_ADJUST_FLAGS_WITH_NEW_PRIO(flags, prio);
}

/*
 * Used to raise internal CPU interrupt priority if param new_prio is greater
 * than actual priority.
 *
 * Note:    EE_FREG param flags and return value needed only for according to
 *          HAL interface.
 */

EE_INLINE__ EE_FREG EE_hal_raise_int_prio_if_less(
  EE_isr2_prio new_prio, EE_FREG flags )
{
  register EE_isr2_prio prev_prio = EE_k1_get_int_prio();
  if ( prev_prio < new_prio )
  {
    EE_k1_set_int_prio(new_prio);
    /* Mask PS.IL flags and set the new one */
    flags = EE_K1_ADJUST_FLAGS_WITH_NEW_PRIO(flags, new_prio);
  }
  return flags;
}

/*
    Used to check if interrupt priority is less than new priority to be
    set.
*/
EE_INLINE__ EE_BIT EE_hal_check_int_prio_if_higher(
  EE_isr2_prio new_prio )
{
  register EE_isr2_prio actual_prio = EE_k1_get_int_prio();
  return (actual_prio > new_prio)? 1U: 0U;
}

EE_INLINE__ void EE_hal_change_context_from_isr2 ( EE_HDB * p_To,
  EE_task_func task_func, EE_HDB * p_From )
{
  EE_hal_set_int_prio( p_To->p_TA->isr2_prio );
  EE_hal_change_context(p_To, task_func, p_From);
}

EE_INLINE__ void EE_k1_clear_irq_source ( EE_isr2_source_id isr2_id ) {
  __k1_interrupt_clear_num(isr2_id);
}

EE_INLINE__ void EE_k1_activate_irq_source ( EE_isr2_source_id isr2_id,
    EE_isr2_prio isr2_prio )
{
  if ( isr2_prio == EE_ISR_UNMASKED ) {
    isr2_prio = EE_ISR_PRI_1;
  }
  __k1_interrupt_set_priority(isr2_id, isr2_prio);
  __k1_interrupt_enable_num(isr2_id);
}

EE_INLINE__ void EE_k1_deactivate_irq_source ( EE_isr2_source_id isr2_id )
{
  __k1_interrupt_disable_num(isr2_id);
}

/* Resource Pools */
extern EE_HPB ee_k1_pools[EE_K1_CORE_NUMBER];
/* All Core OS initialization */
void EE_os_init ( void );
/* IIRQ Handler */
//void EE_iirq_handler ( void );
#endif /* EE_HAL_INTERNAL_H_ */
