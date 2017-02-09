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

EE_task_id EE_k1_int_vectors[EE_K1_PE_NUMBER][EE_ISR_NUMBER]=
  {[0 ... (EE_K1_PE_NUMBER - 1U)] = {[0 ... (EE_ISR_NUMBER - 1U)] =
    INVALID_TASK_ID}};

StatusType EE_hal_set_isr2_source ( EE_task_id task_id,
  EE_isr2_source_id isr2_id, EE_isr2_prio isr2_prio )
{
  EE_CORE_ID  const core_id = EE_get_curr_core_id();
  EE_TDB *    const p_TDB   = &EE_get_kernel()->tdb_array[task_id];
  EE_TA  *    const p_TA    = p_TDB->p_HDB->p_TA;
  p_TA->isr2_prio = isr2_prio;
  EE_k1_int_vectors[core_id][isr2_id] = task_id;

  EE_k1_clear_irq_source(isr2_id);
  EE_k1_activate_irq_source(isr2_id, isr2_prio);

  return E_OK;
}

/* K1 BSP support handler Overridden */
/* ctx contain pointer to 64 register saved on the stack */
void __k1_do_int ( int r0, void * ctx ) {
  EE_CORE_ID        cpu_id;
  EE_k1_int_handler rm_int_handler;
#ifdef EE_USER_TRACEPOINT
  mppa_tracepoint(erika,THROW_ISR_EXIT);
#endif /* EE_USER_TRACEPOINT */
  mppa_tracepoint(erika,ISR_HANDLER_ENTER, r0);
  cpu_id = EE_k1_get_cpu_id();
  rm_int_handler = (EE_k1_int_handler)__k1_umem_read32(&__k1_int_handlers[r0]);

  if ( (cpu_id == EE_K1_BOOT_CORE) && (rm_int_handler != NULL) ) {
    if ( r0 >= EE_ISR_ID_RM_IIRQ0 ) {
      EE_k1_rmb();
    }
    rm_int_handler(r0);
    mppa_tracepoint(erika,ISR_HANDLER_EXIT, r0);
  } else {
    /* ISR handler */
    if ( r0 == EE_ISR_ID_IIRQ0 )
    {
      EE_KDB EE_CONST * p_KDB;
      EE_CDB EE_CONST * p_CDB;
      EE_TDB          * p_FromTDB;
      EE_TDB          * p_ToTDB;
      EE_k1_rmb(); /* <-- Check if we could do better!!! */
      p_KDB       = EE_get_kernel();
      p_CDB       = EE_lock_and_get_curr_core();
      if ( EE_scheduler_task_current_is_preempted(p_CDB) ) {
        p_FromTDB   = &p_KDB->tdb_array[p_CDB->p_CCB->curr_tid];
        p_ToTDB     = EE_scheduler_task_dispatch(p_CDB);

        EE_unlock_curr_core();
        EE_hal_set_int_prio( p_ToTDB->p_HDB->p_TA->isr2_prio );

        mppa_tracepoint(erika,ISR_HANDLER_EXIT, r0);

        EE_hal_change_context(p_ToTDB->p_HDB, p_ToTDB->task_func,
          p_FromTDB->p_HDB);
      } else {
        EE_unlock_curr_core();
        mppa_tracepoint(erika,ISR_HANDLER_EXIT, r0);
      }

    } else {
      EE_task_id tid = EE_k1_int_vectors[cpu_id][r0];
      mppa_tracepoint(erika,ISR_HANDLER_EXIT, r0);
      if ( tid != INVALID_TASK_ID ) {
        EE_activate_isr2(tid);
      }
    }
  }
}

