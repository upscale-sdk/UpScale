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
    @file ee_k1_cluster_init.c
    @brief Cluster Data Structure Initialization
           strongly influenced by nodeos/src/amp/src/startup/k1_amp.c
    @author Errico Guidieri
    @date 2014
 */
#include "eecfg.h"
#include "ee_k1_bsp_internal.h"
#include "ee_k1_mppa_trace.h"
#include "ee_hal_internal.h"
#include "ee_api_k1.h"
#include "ee_api.h"
#include <stdlib.h>

#ifdef EE_CONF_LIBGOMP
extern void gomp_runtime_entry (/*TODO */);
#else
extern int  main (int argc, char *argv[], char *envp[]);
#endif /* EE_CONF_LIBGOMP */

/* MPPA IPC global data structures */
extern void * _K1_PE_STACK_ADDRESS[16];
extern void * _K1_PE_START_ADDRESS[16];
extern void * _K1_PE_ARGS_ADDRESS[16];

static __k1_uint64_t __attribute__((aligned(16)))
  ee_k1_start_stacks[EE_K1_PE_NUMBER][EE_START_WORD_STACK_SIZE];

__attribute__((noreturn)) void __stack_overflow_detected ( void )
{
    __k1_streaming_disable();
    /* If a debugger is connected, stop right here. */

    /* asm volatile ("break 0x1FFFF\n\t;;\n"); */
    EE_BREAK_INST(0x1FFFF);

    /* Byte access to the DSU timestamp register will trap.
       Works on both cluster and IO-cluter. */
    *((char volatile *)0x70084040) = '!';
    /* Just in case. */
    while (1);
}

void EE_k1_cluster_initialize ( void )
{
   EE_UREG i = 0;
   /*
    * Wake them all.
    */
   for ( i = 0U; i < EE_K1_PE_NUMBER; ++i ) {
     /* EG: Riguardo il  +1: Trucco per assegnare la fine di un array come
            inizio dello stack della data CPU */
     _K1_PE_STACK_ADDRESS[i] = (void *)ee_k1_start_stacks[i+1];
     _K1_PE_START_ADDRESS[i] = EE_k1_cluster_secondary_cpu_initialize;
#ifdef EE_CONF_LIBGOMP
     _K1_PE_ARGS_ADDRESS[i] = 0;
#else
     _K1_PE_ARGS_ADDRESS[i] = 0;
#endif /* EE_CONF_LIBGOMP */
   }

   /* Following is a memory barrier wpurge + fence
     (push write buffer + wait) */
   EE_k1_wmb();

   for ( i = 0U; i < EE_K1_PE_NUMBER; ++i ) {
      __k1_poweron(i);
   }
}

void EE_k1_cluster_secondary_cpu_initialize ( void )
{
  __k1_idle_seq_enable();
  __k1_interrupt_lowest();

#ifdef EE_NO_CACHE
  EE_k1_wmb();
  __k1_dcache_disable();
#endif /* EE_NO_CACHE */

  EE_k1_secondary_cpu_startup ();

  while ( 1 ) {
    /* this function never returns */
    __k1_poweroff();
  }
}

void EE_k1_cluster_finish ( int status )
{
  __k1_interrupt_disable_all();
  if ( __k1_spawn_type() != __MPPA_MPPA_SPAWN ) {
    __k1_syscall1(__NR_iss_exit, status);
  }
  /* __k1_syscall1(__NR_exit, status);  when ISS will not intercept this syscall
    by default */
  __k1_cluster_poweroff();
}

/* Behavior got from nodeos/src/startup.c and nodeos/src/posix/Posix_init */
void EE_k1_secondary_cpu_startup ( void )
{
#ifdef EE_CONF_LIBGOMP
  StartOS(NULL);
  gomp_runtime_entry( /*TODO */);
  exit(0);
#else /* EE_CONF_LIBGOMP */
  /* I want that main function is the IdleTask */
  StartOS(NULL);
  {
    EE_CORE_ID core_id = EE_get_curr_core_id();
    if ( core_id == EE_K1_MAIN_CORE ) {
      int res;
      k1_boot_args_t args;
      get_k1_boot_args(&args);
      /* Run the main function */
      res = main(args.argc, args.argv, args.envp);
      exit(res);
    } else while ( 1 ) {
      EE_k1_idle_enter();
    }
  }
#endif /* EE_CONF_LIBGOMP */
}

