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

#include "eecfg.h"
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
/* #define EE_STACK_ATTRIB EE_COMPILER_ALIGN(EE_STACK_ALIGN_SIZE) \
  EE_COMPILER_SECTION("ee_kernel_stack") */
#define EE_STACK_ATTRIB EE_COMPILER_ALIGN(EE_STACK_ALIGN_SIZE)

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
  mOS_it_disable();
}

EE_INLINE__ void EE_hal_enableIRQ( void )
{
  mOS_it_enable();
}
#define EE_VPS_IE_BIT EE_BIT(5) /* (1U << ((EE_UREG)(5))) */

/* Suspend/Resume Interrupts */
EE_INLINE__ EE_FREG EE_hal_suspendIRQ ( void )
{
  EE_CORE_ID const core_id = EE_get_curr_core_id ();
  EE_FREG flags = _scoreboard_start.SCB_VCORE.PER_CPU[core_id].SFR_PS.word;

  if ( (flags & EE_VPS_IE_BIT) != 0 ) {
    mOS_it_disable();
  }
  return flags;
}

EE_INLINE__ void EE_hal_resumeIRQ ( EE_FREG flags )
{
  if ( (flags & EE_VPS_IE_BIT) != 0 ) {
    mOS_it_enable();
  }
}

/*******************************************************************************
                    HAL For Primitives Synchronization
 ******************************************************************************/

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

EE_INLINE__ void EE_hal_broadcast_signal ( void ) {
#if (!defined(EE_SINGLECORE))
  bsp_inter_pe_event_notify((mOS_vcore_set_t)(-1),
    BSP_IT_LINE);
#endif
}

/* Resources Pool */
extern EE_HPB ee_k1_pool;

/* All Core OS initialization */
void EE_os_init ( void );

#endif /* EE_HAL_INTERNAL_H_ */
