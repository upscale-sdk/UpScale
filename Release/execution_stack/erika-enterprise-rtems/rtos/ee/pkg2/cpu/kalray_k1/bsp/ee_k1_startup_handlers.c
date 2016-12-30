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
    @file ee_k1_startup_handlers.c
    @brief Cluster Startup Handlers as required by Kalray HAL and BSP support
           strongly influenced by nodeos/src/amp/src/startup/handler.c
    @author Errico Guidieri
    @date 2014
 */

/* Contains HAL inclusion */
#include "ee_k1_bsp_internal.h"
#include "ee_hal_internal.h"
#include "ee_kernel_k1.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

extern void _bsp_initialize(void);
extern void __k1_finish_newlib_init(void);

static void EE_k1_init_diagnostic ( void )
{
  __k1_counter_stop(_K1_DIAGNOSTIC_COUNTER0);
  __k1_counter_stop(_K1_DIAGNOSTIC_COUNTER1);
  __k1_counter_stop(_K1_DIAGNOSTIC_COUNTER2);
  __k1_counter_stop(_K1_DIAGNOSTIC_COUNTER3);
  __k1_counter_reset(_K1_DIAGNOSTIC_COUNTER0);
  __k1_counter_reset(_K1_DIAGNOSTIC_COUNTER1);
  __k1_counter_reset(_K1_DIAGNOSTIC_COUNTER2);
  __k1_counter_reset(_K1_DIAGNOSTIC_COUNTER3);

  __k1_counter_enable(_K1_DIAGNOSTIC_COUNTER0, _K1_CYCLE_COUNT,
    _K1_DIAGNOSTIC_NOT_CHAINED);
  __k1_counter_enable(_K1_DIAGNOSTIC_COUNTER1, _K1_EXEC_BUNDLES,
    _K1_DIAGNOSTIC_NOT_CHAINED);
  __k1_counter_enable(_K1_DIAGNOSTIC_COUNTER2, _K1_BR_TAKEN_STALLS,
    _K1_DIAGNOSTIC_NOT_CHAINED);
  __k1_counter_enable(_K1_DIAGNOSTIC_COUNTER3, _K1_RAW_STALLS,
    _K1_DIAGNOSTIC_NOT_CHAINED);
}

extern void _init(void) __attribute__((weak));
extern void _fini(void) __attribute__((weak));
extern void newlib_flushall ( void );

void __k1_rm_c_startup ( void )
{
  EE_array_index i;
  /* To prevent from cases where we do not have pwr_ctler nor any peripherals
   *  mapped in simu (=runner) */
  if ((__k1_uint32_t) (&__bsp_pwrctl_skip) == 0) {
    /* It can safely be called twice, and must not be removed
       (in case CNOC broadcast has not been done) */
    __k1_init_dsu_trace();
    /* This function must be called asap, to release spawner from
       mppa_base_spawn */
    __bsp_sync_timestamp_with_spawner(__k1_spawner_id());
  }
  __k1_low_level_startup((__k1_uint32_t) (&__bsp_pwrctl_skip));
  __k1_rm_init();

  /* EG: Following added by me */
  EE_k1_init_diagnostic();
  /* Init Libc here so Runtimes can use it */
  _bsp_initialize();

  __k1_finish_newlib_init();
  if ( _init ) {
    _init ();
  }
  if ( _fini ) {
    atexit (_fini);
  }
  atexit (newlib_flushall);

  /* I put system initialization here */
#ifdef EE_NO_CACHE
  EE_k1_wmb();
  __k1_dcache_disable();
#endif /* EE_NO_CACHE */

#ifdef EE_TRACE_KERNEL
  mppa_trace_init();
#endif /* EE_TRACE_KERNEL */

  EE_os_init();
  __k1_int_handlers[EE_ISR_ID_RM_IIRQ0] = EE_k1_rm_iirq0_handler;
  __k1_int_handlers[EE_ISR_ID_RM_IIRQ1] = EE_k1_rm_iirq1_handler;
  __k1_int_handlers[EE_ISR_ID_RM_IIRQ2] = EE_k1_rm_iirq2_handler;
  __k1_int_handlers[EE_ISR_ID_RM_IIRQ3] = EE_k1_rm_iirq3_handler;
  EE_k1_clear_irq_source(EE_ISR_ID_RM_IIRQ0);
  EE_k1_clear_irq_source(EE_ISR_ID_RM_IIRQ1);
  EE_k1_clear_irq_source(EE_ISR_ID_RM_IIRQ2);
  EE_k1_clear_irq_source(EE_ISR_ID_RM_IIRQ3);
  EE_k1_activate_irq_source(EE_ISR_ID_RM_IIRQ0, EE_ISR_PRI_MAX);

  StartOS(NULL);
  EE_k1_rm_task_server_setup();

  EE_hal_disableIRQ();
  EE_k1_cluster_initialize();
  /* Synchronize with the startup of the other cores */
  for ( i = 0U; i < EE_K1_PE_NUMBER; ++i ) {
    while ( !__k1_umem_read32(&KCB_WJ.core_ctrls[i].ccb.os_started) ) {
      ; /* Wait for the start of i */
    }
  }
  EE_hal_enableIRQ();
  EE_k1_rm_task_server();
  __k1_poweroff();
#ifdef EE_JOB_TEST_FROM_RM
  {
    #include "ee_assert.h"
    extern void test_assert(int test);
    extern void dummy_job ( JobTaskParam param );
    extern EE_k1_spinlock assertions_lock;
    extern EE_UREG dummy_job_param;
    StatusType status_type;
    JobType    jobId;

    EE_k1_spin_init_lock(&assertions_lock);
#ifdef NORMAL_TEST
    extern BlockableValueType comm_variables[(EE_K1_PE_NUMBER - 1U)];
    for ( i = 0U; i < (EE_K1_PE_NUMBER - 1U); ++i ) {
      InitBlockableValue(&comm_variables[i], 0U);
    }
#endif /* NORMAL_TEST */
#ifdef GOMP_TEST
    extern BlockableValueType MasterBar[EE_K1_PE_NUMBER];
    extern BlockableValueType SlaveBar[EE_K1_PE_NUMBER];
    for ( i = 0U; i < EE_K1_PE_NUMBER; ++i ) {
      InitBlockableValue(&MasterBar[i], 0U);
      InitBlockableValue(&SlaveBar[i], 0U);
    }
#endif /* GOMP_TEST */
    status_type = CreateJob(&jobId, EE_K1_PE_NUMBER, 1U, dummy_job,
      &dummy_job_param, 256U );
    test_assert( status_type == E_OK );

    status_type = ActivateJob(jobId, EE_K1_PE_NUMBER);
    test_assert( status_type == E_OK );
  }
#endif /* EE_JOB_TEST_FROM_RM */
  while ( 1 ) {
    EE_k1_idle_enter(); /* just in case */
  }
}

void __k1_do_pe_before_startup ( void )
{
  __k1_neighbor_enable();
  EE_k1_init_diagnostic();
}

void EE_k1_rm_iirq1_handler ( int r0 ) {
  //EE_k1_event_get_status()
}

void EE_k1_rm_iirq2_handler ( int r0 ) {
  //EE_k1_event_get_status()
}

void EE_k1_rm_iirq3_handler ( int r0 ) {
  //EE_k1_event_get_status()
}

