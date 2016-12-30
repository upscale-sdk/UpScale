/* ###*B*###
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

 /** @file      ee_k1_context.h
  *  @brief     ERIKA TASK context defines and assembly macro
  *  @author    Errico Guidieri
  *  @date      2014
  */

#ifndef INCLUDE_EE_K1_CONTEXT_H__
#define INCLUDE_EE_K1_CONTEXT_H__

/** These assembly macro contains code to save and restore ERIKA Task context
    check 01_k1dp-VLIWCore.pdf chapter 7.4.1 Register Usage Conventions.

    Basically the TASK context is composed by all GPRs
    (General Purpose registers) that EABI dictate are "callee saved registers" +
    all GPRs with special meaning for EABi +
    RA (Return Address ) and PS (Program Status) SFRs
    (Special Function Registers)

    N.B. These macro cannot use scratch registers (Parameter registers:
         $r0..$r7 + "pure" scratch registers: $r32..$r64), because these are
         supposed to be usable anyware in C or Assembly functions
*/

#if (!defined(__ASSEMBLER__))
#include "ee_hal.h"
#else
#include "eecfg.h"
#endif /* !__ASSEMBLER__ */

#include <HAL/hal/context.h>

#define EE_USE_STACK_FOR_CONTEXT

/* TASK scratch AREA size */
#ifndef EE_STACK_SCRATCH_AREA_SIZE
#define EE_STACK_SCRATCH_AREA_SIZE 16
#endif /* EE_STACK_SCRATCH_AREA_SIZE */
/* Task Function Address offset in TDB */
#define EE_TDB_TASKFUNC_OFFSET     20

/* Pointer to Task Description Block (pTDB) offset in TCB */
#define EE_TCB_PTDB_OFFSET         0
/* Pointer to Stack Control Block offset in TCB */
#define EE_TCB_PSCB_OFFSET         4
/* Context data offset in TCB */
#define EE_TCB_CTX_OFFSET          8
/* Status offset in TCB */
#define EE_TCB_STATUS_OFFSET       12

/* Pointer of Stack Control Block offset in Stack Description Block (SDB) */
#define EE_SDB_PSCB_OFFSET         0
#define EE_SDB_PBOS_OFFSET         4

/* Pointer Top Of The Stack offset in SCB */
#define EE_SCB_PTOS_OFFSET         0

#define EE_K1_TASK_CONTEXT_R10R11_OFFSET   0
#define EE_K1_TASK_CONTEXT_R12R13_OFFSET   8
#define EE_K1_TASK_CONTEXT_R14R15_OFFSET   16
#define EE_K1_TASK_CONTEXT_R16R17_OFFSET   24
#define EE_K1_TASK_CONTEXT_R18R19_OFFSET   32
#define EE_K1_TASK_CONTEXT_R20R21_OFFSET   40
#define EE_K1_TASK_CONTEXT_R22R23_OFFSET   48
#define EE_K1_TASK_CONTEXT_R24R25_OFFSET   56
#define EE_K1_TASK_CONTEXT_R26R27_OFFSET   64
#define EE_K1_TASK_CONTEXT_R28R29_OFFSET   72
#define EE_K1_TASK_CONTEXT_R30R31_OFFSET   80
#define EE_K1_TASK_CONTEXT_RA_PS_OFFSET    88
#define EE_K1_TASK_CONTEXT_PTOS_OFFSET     96

#define EE_K1_TASK_CONTEX_SIZE             104



/* 26 */
#define EE_K1_TASK_CONTEX_WORDS (EE_K1_TASK_CONTEX_SIZE/sizeof(EE_STACK_T))

#ifdef __ASSEMBLER__
  .macro EE_k1_task_vcontext_save to
## Store $p10
## Access to the scoreboard (make _scoreboard_start + $pcr)
## $r6 = _scoreboard_start
## $r7 = $pcr
    sd    EE_K1_TASK_CONTEXT_R10R11_OFFSET[\to],  $p10  ## $r10 - $r11
    make  $r6, _scoreboard_start
    get   $r7, $pcr
    ;;
## Store $p12
## $r7 = Extract PID bitfiled from $pcr
    sd    EE_K1_TASK_CONTEXT_R12R13_OFFSET[\to],  $p12  ## $r12 - $r13
    extfz $r7, $r7, 15, 11
    ;;
## Store $p14
## $r7 = PID * MOS_SCOREBOARD_PER_CPU_ELEMENT_SIZE (=256) => PID << 8
    sd    EE_K1_TASK_CONTEXT_R14R15_OFFSET[\to],  $p14  ## $r14 - $r15
    sll   $r7, $r7, 8
    ;;
## Store $p16
## $r6 = $_scoreboard_start.SCB_VCORE.PER_CPU[pid]
    sd    EE_K1_TASK_CONTEXT_R16R17_OFFSET[\to],  $p16  ## $r16 - $r17
    add   $r6, $r6, $r7
    ;;
    sd    EE_K1_TASK_CONTEXT_R18R19_OFFSET[\to],  $p18  ## $r18 - $r19
    ;;
    sd    EE_K1_TASK_CONTEXT_R20R21_OFFSET[\to],  $p20  ## $r20 - $r21
    ;;
    sd    EE_K1_TASK_CONTEXT_R22R23_OFFSET[\to],  $p22  ## $r22 - $r23
    ;;
    sd    EE_K1_TASK_CONTEXT_R24R25_OFFSET[\to],  $p24  ## $r24 - $r25
    ;;
    sd    EE_K1_TASK_CONTEXT_R26R27_OFFSET[\to],  $p26  ## $r26 - $r21
    ;;
    sd    EE_K1_TASK_CONTEXT_R28R29_OFFSET[\to],  $p28  ## $r28 - $r29
    ;;
    get   $r8 = $ra  ## I use $p8 (that is scratch) to hold RA & PS
    sd    EE_K1_TASK_CONTEXT_R30R31_OFFSET[\to],  $p30  ## $r30 - $r31
    ;;
## Get VCORE PS to be saved ($r6)
    lw    $r9 = MOS_VC_REG_PS[$r6] ## Now $p8 ($r8 & $r9 contains RA PS)
    ;;
    sd    EE_K1_TASK_CONTEXT_RA_PS_OFFSET[\to] = $p8  ## $ra - $ps
    ;;
  .endm

  .macro EE_k1_task_vcontext_restore from
## Temporary Load $ra and $ps on $p10
## Access to the scoreboard
    ld    $p10  = EE_K1_TASK_CONTEXT_RA_PS_OFFSET[\from]  ## $ra - $ps
    make  $r8, _scoreboard_start
    get   $r9, $pcr
    ;;
## Restore $p30
## Restore $ra
## Extract PID bitfiled from $pcr
    ld    $p30  = EE_K1_TASK_CONTEXT_R30R31_OFFSET[\from] ## $r30 - $r31
    set   $ra   = $r10
    extfz $r9, $r9, 15, 11
    ;;
## Restore $p28
## PID * MOS_SCOREBOARD_PER_CPU_ELEMENT_SIZE (=256) => PID << 8
    ld    $p28  = EE_K1_TASK_CONTEXT_R28R29_OFFSET[\from] ## $r28 - $r29
    sll   $r9, $r9, 8
    ;;
## Restore $p26
## $r8 = $_scoreboard_start.SCB_VCORE.PER_CPU[pid]
    ld    $p26  = EE_K1_TASK_CONTEXT_R26R27_OFFSET[\from] ## $r26 - $r27
    add   $r8, $r8, $r9
    ;;
## Setting VCORE.PS
    sw    MOS_VC_REG_PS[$r8] = $r11
    ;;
    ld    $p24  = EE_K1_TASK_CONTEXT_R24R25_OFFSET[\from] ## $r24 - $r25
    ;;
    ld    $p22  = EE_K1_TASK_CONTEXT_R22R23_OFFSET[\from] ## $r22 - $r23
    ;;
    ld    $p20  = EE_K1_TASK_CONTEXT_R20R21_OFFSET[\from] ## $r20 - $r21
    ;;
    ld    $p18  = EE_K1_TASK_CONTEXT_R18R19_OFFSET[\from] ## $r18 - $r19
    ;;
    ld    $p16  = EE_K1_TASK_CONTEXT_R16R17_OFFSET[\from] ## $r16 - $r17
    ;;
    ld    $p14  = EE_K1_TASK_CONTEXT_R14R15_OFFSET[\from] ## $r14 - $r15
    ;;
    /* If from registers is $r12 this instructions is supposed to write on
       $r12 its actual value. */
    ld    $p12  = EE_K1_TASK_CONTEXT_R12R13_OFFSET[\from] ## $r12 - $r13
    ;;
    ld    $p10  = EE_K1_TASK_CONTEXT_R10R11_OFFSET[\from] ## $r10 - $r11
    ;;
  .endm

#endif /* __ASSEMBLER__ */

#endif /* INCLUDE_EE_K1_CONTEXT_H__ */
