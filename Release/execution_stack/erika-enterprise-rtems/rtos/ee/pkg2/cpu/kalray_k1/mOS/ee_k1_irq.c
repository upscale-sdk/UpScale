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

/**
    @file ee_k1_irq.c
    @brief Interrupt Request Handler (based on Kalray BSP support).
    @author Errico Guidieri
    @date 2014
 */


/* I need Kernel inclusion for Common Context and Service Protection */
#include "ee_internal.h"

EE_task_id EE_k1_int_vectors[EE_K1_CORE_NUMBER][EE_ISR_NUMBER]=
  {[0 ... (EE_K1_CORE_NUMBER - 1U)] = {[0 ... (EE_ISR_NUMBER - 1U)] =
    INVALID_TASK_ID}};

StatusType EE_hal_set_isr2_source ( EE_task_id task_id,
  EE_isr2_source_id isr2_id, EE_isr2_prio isr2_prio )
{
  EE_CORE_ID          const core_id = EE_get_curr_core_id();
  EE_TDB  EE_CONST *  const p_TDB   = &EE_get_kernel()->tdb_array[task_id];
  EE_TA  *            const p_TA    = p_TDB->HDB.p_TA;
  p_TA->isr2_prio = isr2_prio;
  EE_k1_int_vectors[core_id][isr2_id] = task_id;

  /* Interrupt Service Routine registration for OS: Unique Handler */
  bsp_register_it(EE_os_it_handler, isr2_id);
  /* TODO: Configure the Priority for the interrupts, remapping bsp virtual
   * events on real interrupts */
  /* mOS_configure_int(real_isr2_id, isr2_prio); */
  return E_OK;
}

/* K1 BSP support handler Overridden */
/* ctx contain pointer to 64 register saved on the stack */
void EE_os_it_handler ( int ev_src, __k1_vcontext_t *ctx ) {
  EE_CORE_ID        cpu_id;
#ifdef EE_USER_TRACEPOINT
  mppa_tracepoint(erika,THROW_ISR_EXIT);
#endif /* EE_USER_TRACEPOINT */
  mppa_tracepoint(erika,ISR_HANDLER_ENTER, ev_src);
  cpu_id = EE_get_curr_core_id();
  /* ISR handler */
  if ( ev_src >= EE_K1_RESCHEDULE_IIRQ )
  {
    EE_k1_optimized_task_preemption_point();
    mppa_tracepoint(erika,ISR_HANDLER_EXIT, ev_src);
  } else {
    EE_task_id tid = EE_k1_int_vectors[cpu_id][ev_src];
    mppa_tracepoint(erika,ISR_HANDLER_EXIT, r0);
    if ( tid != INVALID_TASK_ID ) {
      EE_activate_isr2(tid);
      /* EE_os_int_context_restore(ctx); */
      /* Since I Save and restore PS in TASK Context when I will be back here
       * I will be still in ISR CTX. So I can let the IRQ wrapper
       * epilogue to restore the previous context status */
    }
  }
}

