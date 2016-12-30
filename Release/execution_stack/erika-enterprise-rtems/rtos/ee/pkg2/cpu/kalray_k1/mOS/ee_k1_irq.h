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

 /** @file  ee_k1_irq.h
  *  @brief Header with definition for Interrupt handling
  *  @author Errico Guidieri
  *  @date 2014
  */
#ifndef INCLUDE_EE_K1_IRQ_H__
#define INCLUDE_EE_k1_IRQ_H__

#include <ee_k1_vbsp.h>

/* Remapping on virtual PE event for reschedule */
#define EE_K1_RESCHEDULE_IIRQ  BSP_IT_PE_0
/* Number of IRQ */
#define EE_ISR_NUMBER          BSP_NB_IT_SOURCES

/* Macro to declare ISR: always valid */
#define DeclareIsr(f) void f ( void )

/* Declare an ISR (category 1) */
#define ISR1(f) void f ( void )

/* Define an ISR (category 2). Used only for client code. The wrapper is moved
   inside ee_tricore_intvec.c to isolate better user code from Kernel code.
   In TriCore Architecture ISR ID and ISR Priority are the same.
   I Use ISR ID as interrupt vector entry because is tied to handler name
   and I can easily reconstruct that here */
#define ISR2(f) ISR1(f)

/* OSEK Standard Macro for ISR declaration */


#ifndef EE_TRACE_KERNEL
#define ISR(f) void f ( void )
#else
#define ISR(f)                                \
static void EE_S_J(f,ISR2Body) ( void );      \
void f ( void ) {                             \
  mppa_tracepoint(erika,CONTEXT_SWITCH_EXIT); \
  EE_S_J(f,ISR2Body)();                       \
}                                             \
static void EE_S_J(f,ISR2Body) ( void )
#endif /* !EE_TRACE_KERNEL */

/* Central Handler */
extern void EE_os_it_handler (int nb, __k1_vcontext_t *ctx);

/* Interrupt Frame Restore */
extern void EE_os_int_context_restore (__k1_vcontext_t *ctx);

#endif /* INCLUDE_EE_K1_IRQ_H__ */
