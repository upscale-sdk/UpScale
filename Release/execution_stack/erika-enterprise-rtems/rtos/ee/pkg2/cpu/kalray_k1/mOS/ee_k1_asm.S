 #
 #  @file ee_k1_oo.c
 #  @brief Functions to save and restore registers for OSEK TerminateTask().
 #  @author Errico Guidieri
 #  @date 2014

#include <HAL/hal/hal.h>
#include "ee_k1_context.h"
#include "mOS_common_types_s_c.h"

# WARNING (Remembere the following when you write assembly code):
# 1) Stack have to be 8 byte alligned (so the frame have to be at least 8 byte
#    long) and grows towards "smaller" addresses. The SP (Stack Pointer) is
#    $R12.
# 2) The K1 architecture (and obviusly this means "memory accesses" too
#    in particular) is little endian so the variables are written towards
#    "bigger" addresses.
# 3) The first (bigger in address) word accessible of the function stack frame
#    is (STACK_FRAME_SIZE - 4)[SP]. So with a frame of 8 byte the two accessible
#    words are (in Order) 4[SP] and 0[SP].

# Save the RA in the stack an SP in EE_terminate_data{tid} to unwind the Stack
# in TASK termination.

  /* Macro: Internally used */
  .macro EE_hal_save_vcontext_m pFromSDB local local2
  # \local  = &p_SDB->p_SCB (\p_SDB)
  # \local2 = p_SDB->p_SCB->tos
# Allocate context on stack;
  add   $r12 = $r12, -EE_K1_TASK_CONTEX_SIZE
# \local  = p_SDB->p_SCB (\p_SDB)
  lw \local = EE_SDB_PSCB_OFFSET[\pFromSDB]
  ;;
# \local2 = pFromSDB->p_SCB->tos
  lw \local2 = EE_SCB_PTOS_OFFSET[\local]
  ;;
# Save the context on the stack
  EE_k1_task_vcontext_save $r12
  ;;
# Save tos nel context
  sw EE_K1_TASK_CONTEXT_PTOS_OFFSET[$r12] = \local2
  ;;
# pFromSDB->p_SCB->tos = SP ($r12)
  sw EE_SCB_PTOS_OFFSET[\local] = $r12
  ;;
  .endm

  .macro EE_hal_restore_vcontext_m local local2
  # \local:         &pToSDB->p_SCB
  # \local2:        p_ToSDB->p_SCB->tos, Old pTOS
# \local = pToSDB->p_SCB ($r0)
  lw \local = EE_SDB_PSCB_OFFSET[$r0]
  ;;
# \local2 = pToSDB->p_SCB->tos
  lw \local2 = EE_SCB_PTOS_OFFSET[\local]
  ;;
# Restore the context
  EE_k1_task_vcontext_restore \local2
  ;;
# Get back Old tos
  lw \local2 = EE_K1_TASK_CONTEXT_PTOS_OFFSET[\local2]
  add $r12 = $r12, EE_K1_TASK_CONTEX_SIZE
  ;;
# pToSDB->p_SCB->tos = Old tos
  sw EE_SCB_PTOS_OFFSET[\local] = \local2
  ;;
  .endm

#if 0 /* in mOS environment cannot be used. */
# Enable Interrupts
  .macro EE_hal_enableIRQ_m local
# \local = 0x00100000
  make \local = 1048576
  ;;
  hfxb $ps, \local
  ;;
  .endm
#endif /* 0 */

/* void EE_hal_save_and_restore_context( EE_SDB * pToSDB, EE_SDB * pFromSDB ) */
  .align 8
  .globl EE_hal_save_ctx_and_restore_ctx
  .type EE_hal_save_ctx_and_restore_ctx, @function
EE_hal_save_ctx_and_restore_ctx:
  # $r0 parameter:  EE_SCB * pToSDB
  # $r1 parameter:  EE_SCB * pFromSDB
# EE_hal_save_context macro
  EE_hal_save_vcontext_m $r1 $r2 $r3
  ;;
# Unconditional brach to restore context
  goto EE_hal_restore_ctx
  ;;
  .size EE_hal_save_ctx_and_restore_ctx, .-EE_hal_save_ctx_and_restore_ctx

/* void EE_hal_restore_context( EE_SDB * pToSDB ) */
  .align 8
  .globl EE_hal_restore_ctx
  .type EE_hal_restore_ctx, @function
EE_hal_restore_ctx:
  # $r0 parameter:  EE_SCB * pToSDB
  # $r1 local:      &pToSDB->pSCB, Enable Interrupts Mask
  # $r2 local:      pToSDB->pSCB->pTOS, Old pTOS
  EE_hal_restore_vcontext_m $r1 $r2
  ;;
  make  $r0 = 0
  goto  EE_scheduler_task_wrapper
  ;;
  .size EE_hal_restore_ctx, .-EE_hal_restore_ctx

/* void EE_hal_save_and_ready2stacked( EE_SDB * pToSDB, EE_task_func func,
     EE_SDB * pFromSDB) */
  .align 8
  .globl EE_hal_ready2stacked
  .type EE_hal_ready2stacked, @function
EE_hal_ready2stacked:
  # $r0 parameter:  EE_SCB * pToSDB
  # $r1 parameter:  func
  # $r2 local:      pToSDB->pSCB
# $r2 = pToSDB->pSCB ($r0)
  lw $r2  = EE_SDB_PSCB_OFFSET[$r0]
  ;;
# $r12 = pToSDB->pSCB->pTOS ($r2)
  lw $r12 = EE_SCB_PTOS_OFFSET[$r2]
  ;;
# Enable Interrupts
# EE_hal_enableIRQ_m  $r2 EG: Cannot be used in mOS environment
# Create the Stack Scratch Area for the Function and call the Task Wrapper!!!
  add   $r12, $r12, -EE_STACK_SCRATCH_AREA_SIZE
  copy  $r0 = $r1
  call  EE_scheduler_task_wrapper
  ;;
  goto EE_scheduler_task_not_terminated
  ;;
  .size EE_hal_ready2stacked, .-EE_hal_ready2stacked

/* void EE_hal_save_and_ready2stacked( EE_SDB * pToSDB, EE_task_func func, EE_SDB * pFromSDB) */
  .align 8
  .globl EE_hal_save_ctx_and_ready2stacked
  .type EE_hal_save_ctx_and_ready2stacked, @function
EE_hal_save_ctx_and_ready2stacked:
  # $r0 parameter:  EE_SCB * pToSDB
  # $r1 parameter:  func
  # $r2 parameter:  EE_SCB * pFromSDB
# EE_hal_save_vcontext macro
  EE_hal_save_vcontext_m $r2 $r3 $r4
  ;;
  goto EE_hal_ready2stacked
  ;;
  .size EE_hal_save_ctx_and_ready2stacked, .-EE_hal_save_ctx_and_ready2stacked

/* void EE_hal_terminate_ctx (EE_SDB * pToSDB) */
  .align 8
  .globl EE_hal_terminate_ctx
  .type EE_hal_terminate_ctx, @function
EE_hal_terminate_ctx:
  # $r0 parameter:  EE_SCB * pTermSDB
  # $r1 Parameter:  kern_callback
  # $r2 local:      pTermSDB->pSCB
  # $r3 local:      pTermSDB->pSCB->pTOS
# $r2 = pTermSDB->pSCB ($r0)
  lw $r2 = EE_SDB_PSCB_OFFSET[$r0]
  ;;
# $r3 = pTermSDB->pSCB->pTOS ($r2)
  lw $r3 = EE_SCB_PTOS_OFFSET[$r2]
  ;;
# Unwind the STACK + Create SCRATH Area.
  add $r12 = $r3, -EE_STACK_SCRATCH_AREA_SIZE
  icall $r1
  ;;
# This is a NORETURN Function
.never_get_here:
  goto .never_get_here
  ;;
  .size EE_hal_terminate_ctx, .-EE_hal_terminate_ctx

#==============================================================================#

/*
void EE_hal_change_context_from_isr2 ( EE_HDB * p_To, EE_task_func task_func,
  EE_HDB * p_From )
{
  EE_CORE_ID core_id = EE_get_core(core_id);
  _scoreboard_start.SCB_VCORE.PER_CPU[core_id].SFR_SPC = &EE_hal_change_context;
  mOS_RFE();
}
*/

  .global EE_hal_change_context

  .align 8
  .globl EE_hal_change_context_from_isr2
  .proc EE_hal_change_context_from_isr2
EE_hal_change_context_from_isr2:
# Prepare the Function Frame ($r16,$r17)
  add $r12, $r12, -16
  ;;
# Save two callee registers without any specific meaning to use as temp
# variables.
# Read the process Identification Register $pi in documentation file
  sd 0[$r12] = $p16
  make $r16 = _scoreboard_start
  get $r17, $pcr
  ;;
# Save two parameter ($r0,$r1) register on the stack
# Extract PID bitfiled from $pcr
  sd 8[$r12] = $p0
  extfz $r17, $r17, 15, 11
  ;;
# PID * MOS_SCOREBOARD_PER_CPU_ELEMENT_SIZE (=256) => PID << 8
  sll $r17, $r17, 8
  ;;
# $r16 = $_scoreboard_start.SCB_VCORE.PER_CPU[pid]
# $r17 = EE_hal_change_context
  add $r16, $r16, $r17
  make $r17 = EE_hal_change_context
  ;;
# Prepare the SSP to be saved in MOS_VC_REG_SSP
# Save on the shadow program counter the change context routine
# (N.B: In a bundle the register update it's the last thing so the following
#  works)
  add $r17 = $r12, 16
  sw MOS_VC_REG_SPC[$r16] = $r17
  ;;
# Save on the shadow stack pointer the desidered stack (without function frame)
  sw MOS_VC_REG_SSP[$r16] = $r17
  ;;
# Destroy the function frame (I didn't touched RA so I will let the TASK
# directly return on calling site and this call have to be transparent).
# Issue a MOS_VC_RFE scall
# Restore $p16 ($r16,$r17), maintain the 16 bytes of stack as Scratch Area
# (N.B: In a bundle the register update it's the last thing so the following
#  works)
  add $r12, $r12, 16
  ld $p16 = 0[$r12]
  ;;
  scall MOS_VC_RFE
  ;;
  .endp EE_hal_change_context_from_isr2
