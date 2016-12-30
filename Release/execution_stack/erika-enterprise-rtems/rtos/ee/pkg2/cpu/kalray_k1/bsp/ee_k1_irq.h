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

#include <HAL/hal/hal.h>

/* ISR priority level defines */
#define EE_ISR_UNMASKED _K1_INT_0
#define EE_ISR_PRI_1    _K1_INT_1
#define EE_ISR_PRI_2    _K1_INT_2
#define EE_ISR_PRI_3    _K1_INT_3
#define EE_ISR_PRI_4    _K1_INT_4
#define EE_ISR_PRI_5    _K1_INT_5
#define EE_ISR_PRI_6    _K1_INT_6
#define EE_ISR_PRI_7    _K1_INT_7
#define EE_ISR_PRI_8    _K1_INT_8
#define EE_ISR_PRI_9    _K1_INT_9
#define EE_ISR_PRI_10   _K1_INT_10
#define EE_ISR_PRI_11   _K1_INT_11
#define EE_ISR_PRI_12   _K1_INT_12
#define EE_ISR_PRI_13   _K1_INT_13
#define EE_ISR_PRI_14   _K1_INT_14
#define EE_ISR_PRI_15   _K1_INT_15

/* This is equal to _K1_INT_15 */
#define EE_ISR_PRI_MAX  _K1_MAX_INT_NUMBER

#define EE_ISR_ID_TIMER0          _K1_PE_INT_LINE_TIMER0         /*  0 */
#define EE_ISR_ID_TIMER1          _K1_PE_INT_LINE_TIMER1         /*  1 */
#define EE_ISR_ID_WATCHDOG        _K1_PE_INT_LINE_WATCHDOG       /*  2 */
#define EE_ISR_ID_IIRQ0           _K1_PE_INT_LINE_BIDIR0         /*  8 */
#define EE_ISR_ID_IIRQ1           _K1_PE_INT_LINE_BIDIR1         /*  9 */
#define EE_ISR_ID_IIRQ2           _K1_PE_INT_LINE_BIDIR2         /* 10 */
#define EE_ISR_ID_IIRQ3           _K1_PE_INT_LINE_BIDIR3         /* 11 */
#define EE_ISR_ID_SHOOTDOWN       _K1_PE_INT_LINE_SHOOTDOWN      /* 15 */

#define EE_ISR_ID_RM_CNOCRX       _K1_RM_INT_LINE_CNOCRX         /*  3 */
#define EE_ISR_ID_RM_DNOCRX       _K1_RM_INT_LINE_DNOCRX         /*  4 */
#define EE_ISR_ID_RM_DNOCTXX      _K1_RM_INT_LINE_DNOCTXX        /*  5 */
#define EE_ISR_ID_RM_DNOCERR      _K1_RM_INT_LINE_DNOCERR        /*  6 */
#define EE_ISR_ID_RM_IIRQ0        _K1_RM_INT_LINE_BIDIR0         /*  8 */
#define EE_ISR_ID_RM_IIRQ1        _K1_RM_INT_LINE_BIDIR1         /*  9 */
#define EE_ISR_ID_RM_IIRQ2        _K1_RM_INT_LINE_BIDIR2         /* 10 */
#define EE_ISR_ID_RM_IIRQ3        _K1_RM_INT_LINE_BIDIR3         /* 11 */
#define EE_ISR_ID_RM_PEWATCHDOG   _K1_RM_INT_LINE_PEWATCHDOGIRQ  /* 14 */

#define EE_ISR_NUMBER             (_K1_MAX_INT_NUMBER + 1U)      /* 16 */

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


typedef void (* EE_k1_int_handler) (int);
extern void (*__k1_int_handlers[EE_ISR_NUMBER])(int);

#endif /* INCLUDE_EE_K1_IRQ_H__ */
