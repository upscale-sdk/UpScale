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
 * ###*E*### */

 /** @file      ee_hal.h
  *  @brief     ISA dependent part of HAL
  *  @author    Errico Guidieri
  *  @date      2014
  */

#ifndef EE_HAL_H_
#define EE_HAL_H_

/*******************************************************************************
                          HAL Types and structures
 ******************************************************************************/
#include "eecfg.h"
#include "ee_compiler_gcc.h"
#include "ee_platform_types.h"
#include "ee_k1_vbsp.h"
#include "ee_utils.h"

#ifndef __GNUC__
#error Unsupported compiler!
#endif /* !__GNUC__ */

/*******************************************************************************
                            Array Utilities
 ******************************************************************************/

/* Fill Pattern Used for Array Monitoring */
#ifndef EE_FILL_PATTERN
#define EE_FILL_PATTERN 0xA5A5A5A5U
#endif /* EE_FILL_PATTERN */

/* Use Range Designated Initializers */
#define EE_FILL_ARRAY(array) \
  = {[0 ... (sizeof(array)/sizeof(array[0]) - 1U)] = EE_FILL_PATTERN}

/*******************************************************************************
                 Utility Macros for debugging and tracing purposes
 ******************************************************************************/

/* Break point instruction, can be useful even when EE_DEBUG is not defined */
#define EE_BREAK_INST(num)      __asm__ volatile ("break " EE_S(num) "\n\t;;\n")

#define EE_BREAK_POINT_NUM      0x1FFFF /* TODO: Maybe another value have tbd */
#define EE_BREAK_POINT()        EE_BREAK_INST(EE_BREAK_POINT_NUM)
#define EE_GLOBAL_LABEL(label)  __asm__(".globl " #label "\n" #label ":")

/* !!!WARNING!!! Add this to the end of all asm inline "string". */
#define EE_K1_END_ASM_BUNDLE  "\n\t;;\n\t"

/* Context handling functions for Tasking */
EE_INLINE__ EE_ADDR EE_get_SP ( void )
{
  register EE_ADDR sp = 0U;
  __asm__ volatile ("copy %0 = $r12" EE_K1_END_ASM_BUNDLE : "=r"(sp));
  return sp;
}

EE_INLINE__ EE_CORE_ID EE_get_curr_core_id ( void ) {
  return  __k1_get_cpu_id();
}

#endif /* EE_HAL_H_ */
