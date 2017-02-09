 #
 #  @file ee_k1_oo.c
 #  @brief Functions to save and restore registers for OSEK TerminateTask().
 #  @author Errico Guidieri
 #  @date 2014

#include <HAL/hal/hal.h>
#include "ee_k1_context.h"

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

  /* Macro usate internamente */
  .macro EE_hal_save_context_m pFromSDB local local2
  # Allocate context on stack;
  # \local  = &pFromSDB->pSCB (\pFromSDB)
  # \local2 = pFromSDB->pSCB-pTOS
# Create the Context Frame
# \local  = pFromSDB->pSCB (\pFromSDB)
  add   $r12 = $r12, -EE_K1_TASK_CONTEX_SIZE
  lw \local = EE_SDB_PSCB_OFFSET[\pFromSDB]
  ;;
# \local2 = pFromSDB->pSCB->pTOS
  lw \local2 = EE_SCB_PTOS_OFFSET[\local]
  ;;
# Save the context on the stack
  EE_k1_task_context_save $r12
  ;;
# Save pTOS nel context
  sw EE_K1_TASK_CONTEXT_PTOS_OFFSET[$r12] = \local2
  ;;
# pFromSDB->pSCB->pTOS =  SP ($r12)
  sw EE_SCB_PTOS_OFFSET[\local] = $r12
  ;;
  .endm

  .macro EE_hal_restore_context_m local local2
  # $r0 parameter:  EE_SCB * pToSDB
  # \local:         &pToSDB->pSCB
  # \local2:        pToSDB->pSCB->pTOS, Old pTOS
# \local = pToSDB->pSCB ($r0)
  lw \local = EE_SDB_PSCB_OFFSET[$r0]
  ;;
# \local2 = pFromSDB->pSCB->pTOS
  lw \local2 = EE_SCB_PTOS_OFFSET[\local]
  ;;
# Restore the context
  EE_k1_task_context_restore \local2
  ;;
# Get back Old pTOS
# Set the stack back, freeing the context frame
  lw \local2 = EE_K1_TASK_CONTEXT_PTOS_OFFSET[\local2]
  add $r12 = $r12, EE_K1_TASK_CONTEX_SIZE
  ;;
# pFromSDB->pSCB->pTOS =  Old pTOS
  sw EE_SCB_PTOS_OFFSET[\local] = \local2
  ;;
  .endm

  # Enable Interrupts
  .macro EE_hal_enableIRQ_m local
# \local = 0x00100000
  make \local = 1048576
  ;;
  hfxb $ps, \local
  ;;
  .endm

/* void EE_hal_save_and_restore_context( EE_SDB * pToSDB, EE_SDB * pFromSDB ) */
  .align 8
  .globl EE_hal_save_ctx_and_restore_ctx
  .type EE_hal_save_ctx_and_restore_ctx, @function
EE_hal_save_ctx_and_restore_ctx:
  # $r0 parameter:  EE_SCB * pToSDB
  # $r1 parameter:  EE_SCB * pFromSDB
# EE_hal_save_context macro
  EE_hal_save_context_m $r1 $r2 $r3
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
  EE_hal_restore_context_m $r1 $r2
  ;;
# Enable Interrupts
  EE_hal_enableIRQ_m $r1
  ;;
# Ret need it's own bundle
  ret
  ;;
  .size EE_hal_restore_ctx, .-EE_hal_restore_ctx

#define EE_HAL_R2S_FRAME_SIZE   8

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
  EE_hal_enableIRQ_m  $r2
  ;;
# Create the Stack Scratch Area for the Function!!!
  add $r12, $r12, -EE_STACK_SCRATCH_AREA_SIZE
  icall $r1
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
# EE_hal_save_context macro
  EE_hal_save_context_m $r2 $r3 $r4
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
