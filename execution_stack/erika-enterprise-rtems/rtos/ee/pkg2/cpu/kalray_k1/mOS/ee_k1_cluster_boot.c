/**
 * Copyright (C) 2013-2014 Kalray SA.
 *
 * All rights reserved.
 */

#include "eecfg.h"
#include "ee_internal.h"

#include <locked_assert.h>
#include <string.h>

/* newlib required functions and symbols */
__thread struct _reent _impure_thread_data __attribute__((weak));

extern void _init(void) __attribute__((weak));

extern void __attribute__ ((noreturn, weak)) exit(int reval);

extern int main(int argc, char *argv[], char **envp);

/* Assembly Wrappers for Interrupts an Scalls */
/* extern void ee_k1_interrupt_wrapper ( void ); Not Used */
extern void ee_k1_scall_wrapper ( void );

/* Big stack's for pe's (kind of thread stack) */
extern __k1_uint8_t _bss_start;
extern __k1_uint8_t _bss_end;
extern __k1_uint8_t _sbss_start;
extern __k1_uint8_t _sbss_end;

extern __k1_uint8_t _tbss_start;
extern __k1_uint32_t _tbss_size;

extern __k1_uint8_t _tdata_start;
extern __k1_uint32_t _tdata_size;

/* extern __k1_uint32_t _tls_size; */

/* System Call User Space Handler */
/* int
  EE_os_scall (int r0, int r1, int r2, int r3, int r4, int r5, int r6, int r7);
*/

#if (defined(EE_HAS_COMM_HOOK))
EE_k1_spinlock comm_lock;
#endif

char **environ __attribute__((weak));

static void _tls_init(void) {

  if ( _init ) {
    /* K1 EABI states that $r13 hold the thread local storage base */
    register unsigned long tls_base __asm__("r13");
    __k1_uint8_t *tls_data = (__k1_uint8_t *) tls_base;
    __k1_uint8_t *tls_bss = tls_data + (&_tbss_start - &_tdata_start);
    __k1_copy_section(tls_data, &_tdata_start, (__k1_uint32_t)&_tdata_size);

    __k1_bss_section(tls_bss,(__k1_uint32_t)&_tbss_size);

    _REENT_INIT_PTR(&_impure_thread_data);
    /* Finish initialization of reent */
    _REENT->_stdin  = &(_REENT->__sf[0]);
    _REENT->_stdout = &(_REENT->__sf[1]);
    _REENT->_stderr = &(_REENT->__sf[2]);
  }
}

#if (defined(ERIKA_HAS_SYSTEM_TIMER))
#define CLOCK_FREQ         (__bsp_frequency/(_K1_DEFAULT_CLOCK_DIV + 1))

#if defined(SYSTEM_TICK_MS)
#define CLOCK_RELOAD       EE_MILLI_TO_TICKS(SYSTEM_TICK_MS, CLOCK_FREQ)
#elif defined(SYSTEM_TICK_US)
#define CLOCK_RELOAD       EE_MICRO_TO_TICKS(SYSTEM_TICK_MS, CLOCK_FREQ)
#else
#define CLOCK_RELOAD_1_MS  EE_MILLI_TO_TICKS(1, CLOCK_FREQ)
#define CLOCK_RELOAD       CLOCK_RELOAD_1_MS
#endif /* SYSTEM_TICK_MS || SYSTEM_TICK_US || default */
#endif /* ERIKA_HAS_SYSTEM_TIMER */

void _pe_init ( void ) {
  int pid = __k1_get_cpu_id();

  __k1_uint8_t *tls_base = __k1_tls_pe_base_address(pid);
  __k1_setup_tls_pe(tls_base);

  _tls_init();

  /* I don't want to change stack for Interrupts, but it's OK for exception
   * and default handlers are good. */
  /* _scoreboard_start.SCB_VCORE.PER_CPU[pid].SFR_PS.isw = 0; */
  _scoreboard_start.SCB_VCORE.PER_CPU[pid].SFR_PS.esw = 1;

  /* _scoreboard_start.SCB_VCORE.PER_CPU[pid].SFR_IT = ee_k1_interrupt_wrapper;
   */
  _scoreboard_start.SCB_VCORE.PER_CPU[pid].SFR_SC = ee_k1_scall_wrapper;

  /* Alternative version with mOS services */
  /* mOS_register_it_handler(&ee_k1_interrupt_wrapper); */
  /* mOS_register_scall_handler(&ee_k1_scall_wrapper); */

  bsp_register_it(EE_os_it_handler, BSP_IT_PE_0);
  bsp_register_it(EE_os_it_handler, BSP_IT_PE_1);
  bsp_register_it(EE_os_it_handler, BSP_IT_PE_2);
  bsp_register_it(EE_os_it_handler, BSP_IT_PE_3);
  bsp_register_it(EE_os_it_handler, BSP_IT_PE_4);
  bsp_register_it(EE_os_it_handler, BSP_IT_PE_5);
  bsp_register_it(EE_os_it_handler, BSP_IT_PE_6);
  bsp_register_it(EE_os_it_handler, BSP_IT_PE_7);
  bsp_register_it(EE_os_it_handler, BSP_IT_PE_8);
  bsp_register_it(EE_os_it_handler, BSP_IT_PE_9);
  bsp_register_it(EE_os_it_handler, BSP_IT_PE_10);
  bsp_register_it(EE_os_it_handler, BSP_IT_PE_11);
  bsp_register_it(EE_os_it_handler, BSP_IT_PE_12);
  bsp_register_it(EE_os_it_handler, BSP_IT_PE_13);
  bsp_register_it(EE_os_it_handler, BSP_IT_PE_14);
  bsp_register_it(EE_os_it_handler, BSP_IT_PE_15);

#if (defined(ERIKA_HAS_SYSTEM_TIMER))
  bsp_register_it(EE_os_it_handler, BSP_IT_TIMER_0);
  mOS_timer_general_setup();
  mOS_timer_setup_num(0, CLOCK_RELOAD, CLOCK_RELOAD, 0);
#endif /* ERIKA_HAS_SYSTEM_TIMER */

  __builtin_k1_wpurge();
  __builtin_k1_fence();

}

/* Communication Hook Handler Body */
#if (defined(EE_HAS_COMM_HOOK))

void communication_hook_handler_body ( void ) {
  /* Put this in some kind of cycle */
  while ( 1 ) {
    EE_k1_spin_lock(&comm_lock);
    CommunicationHook();
    EE_k1_spin_unlock(&comm_lock);

    EE_k1_optimized_task_preemption_point();
  }
}
#endif /* EE_HAS_COMM_HOOK */

void __attribute__((section(TARGET_TEXT)))
  _do_master_pe(uint32_t old_sp __attribute__((unused)))
{
  int ret_val;
  k1_boot_args_t args;

  /* newlib initialization */
  __k1_bss_section(((__k1_uint8_t*) &_bss_start), ((__k1_uint32_t)&_bss_end) - ((__k1_uint32_t) &_bss_start));
  __k1_bss_section(((__k1_uint8_t*) &_sbss_start), ((__k1_uint32_t)&_sbss_end) - ((__k1_uint32_t) &_sbss_start));

  _pe_init();

  if ( _init ) {
    _init ();
  }

  get_k1_boot_args(&args);

  if ( _init ) {
    environ = args.envp;
  }

  /* Configure data structures for Kernel */
  EE_os_init();
  /* Interrupts enabling it's done inside StartOS */
  mOS_set_it_level(0);
  /* mOS_it_enable(); */
#if (defined(EE_HAS_COMM_HOOK))
  EE_k1_spin_init_lock(&comm_lock);
#endif

  /* Catch this context as Idle TASK */
  StartOS(OSDEFAULTAPPMODE);

  /* Write Barrier */
  __builtin_k1_wpurge();
  __builtin_k1_fence();

#if (!defined(EE_SINGLECORE))
  mOS_it_disable();

  /* Synchronize with the startup of the other cores */
  for (unsigned int i = 1U; i < EE_K1_CORE_NUMBER; ++i ) {
    while ( !__k1_umem_read32(&KCB_WJ.core_ctrls[i].ccb.os_started) ) {
      ; /* Wait for the start of i */
    }
  }
  mOS_it_enable();

#if (defined(EE_CONF_LIBGOMP))
#if (defined(EE_HAS_COMM_HOOK))
  {
    EE_CDB * const p_CDB          = EE_get_curr_core();
    EE_TDB * const p_idle_task    = p_CDB->p_idle_task;

    p_idle_task->task_func = communication_hook_handler_body;

    EE_hal_start_idle_task ( &p_idle_task->HDB,
      communication_hook_handler_body );
  }
#endif /* EE_HAS_COMM_HOOK */
  ret_val = 0;
#else
  ret_val = main(args.argc, args.argv, args.envp);
#endif  /* EE_CONF_LIBGOMP */
#else
  ret_val = main(args.argc, args.argv, args.envp);
#endif /* !EE_SINGLECORE */
  if ( exit ) {
    exit ( ret_val );
  }
}

void __attribute__((section(TARGET_TEXT)))
  _do_slave_pe ( uint32_t old_sp __attribute__((unused)) )
{
  _pe_init();

#if (!defined(EE_SINGLECORE))
  mOS_set_it_level(0);
  while (!__k1_umem_read32(&KCB_WJ.core_ctrls[EE_K1_MAIN_CORE].ccb.os_started))
  {
    ; /* cycle until master pe has done what he needs */
  }

  /* Catch this context as Idle TASK */
  StartOS(OSDEFAULTAPPMODE);

  do {
#if (defined(EE_HAS_COMM_HOOK))

    if ( EE_k1_spin_trylock(&comm_lock) ) {
      CommunicationHook();
      EE_k1_spin_unlock(&comm_lock);
    }
#endif /* EE_HAS_COMM_HOOK */
#if (!defined(EE_K1_FULL_PREEMPTION))
    /* Disable User Interrupt Line before go to sleep and handle that after
     * wake-up */
    mOS_it_disable_num(MOS_VC_IT_USER_0);

#if (defined(ERIKA_HAS_SYSTEM_TIMER))
    mOS_it_disable_num(MOS_VC_IT_TIMER_0);
#endif /* ERIKA_HAS_SYSTEM_TIMER */

    /* Sleep until the event will wake me up */
    mOS_idle1();

    /* Enter in a critical section */
    /* mOS_it_disable(); */

    /* Invalidate the cache and prepare to read data structures*/
    /* mOS_dinval(); */

    /* Restore "User" interrupt line */
    /* mOS_it_clear_num(MOS_VC_IT_USER_0); */
    /* mOS_it_enable_num(MOS_VC_IT_USER_0); */
    mOS_pe_event_clear(BSP_EV_LINE);
    mOS_pe_event_clear(BSP_IT_LINE);
    mOS_it_clear_num(10); /*  EE_ISR_ID_RM_IIRQ2 */
#if (defined(ERIKA_HAS_SYSTEM_TIMER))
    mOS_it_enable_num(MOS_VC_IT_TIMER_0);
#endif /* ERIKA_HAS_SYSTEM_TIMER */

    EE_k1_optimized_task_preemption_point();

#else
    /* Sleep until the event will wake me up */
    mOS_idle1();
    /* Restore "User" interrupt line */
    mOS_pe_event_clear(BSP_EV_LINE);
    mOS_pe_event_clear(BSP_IT_LINE);
    mOS_it_clear_num(10); /*  EE_ISR_ID_RM_IIRQ2 */
#endif /* !EE_K1_FULL_PREEMPTION */
  } while ( 1 );
#endif /* !EE_SINGLECORE */
}

extern void _fini(void) __attribute__((weak));

extern int __mppa_power_base_exit_return_status __attribute__ ((weak));

#define LOCKED_BUF_SIZE 32
char locked_buf[BSP_NB_PE_MAX][LOCKED_BUF_SIZE] __attribute__ ((section(".locked_data")));

extern unsigned long INTERNAL_RAM_SIZE;

int EE_os_scall(int r0, int r1, int r2, int r3 __attribute__((unused)),
    int r4 __attribute__((unused)), int r5 __attribute__((unused)),
    int r6 __attribute__((unused)), int r7)
{
  int ret = -1;
  mOS_enable_hw_loop();
  int pid = __k1_get_cpu_id();

  switch ((int) r7) {
    case __NR_exit:
      if ( _fini ) {
        if ( &__mppa_power_base_exit_return_status ) {
          __mppa_power_base_exit_return_status = r0;
        }

        _fini();
      }

      mOS_exit((__k1_spawn_type() != __MPPA_MPPA_SPAWN), r0);

      break;
    case __NR_write:
    {
      int i;
      char *ptr = (char *) r1;
      ret = r2;

      while ( r2 ) {
        for ( i = 0; i < LOCKED_BUF_SIZE && i < r2; ++i ) {
          locked_buf[pid][i] = ptr[i];
        }
        /* EG: ??? It should correspond to __printf_jtag(ptr, r2); of
         * mOS/std_runtime/utask */
        __k1_club_syscall2(4094, (unsigned int) locked_buf[pid], LOCKED_BUF_SIZE > r2 ? r2 : LOCKED_BUF_SIZE);
        r2 = r2 > LOCKED_BUF_SIZE ? r2 - LOCKED_BUF_SIZE : 0;
        ptr += LOCKED_BUF_SIZE;
      }
    }
    break;
    case __NR_cache_flush:
      __builtin_k1_wpurge();
      __builtin_k1_fence();
      mOS_dinval();
      ret = 0;
      break;
    case __NR_probe_address_space:
    {
      /* This would be so much easier if we had a proper BSP */
      uint64_t addr = ((uint64_t) (uint32_t) r0) | (((uint64_t) (uint32_t) r1) << 32);
      ret = 0;

      if ( &VIRT_U_MEM_PAG == 0 ) {
        if (addr >= (uint64_t) (uint32_t) &VIRT_U_MEM_START && addr < (uint64_t) (uint32_t) &VIRT_U_MEM_END) {
          ret = 1;
        }
      } else {
        if (addr >= (uint64_t) (uint32_t) &VIRT_U_MEM_START && addr < (uint64_t) (uint32_t) &INTERNAL_RAM_SIZE) {
          ret = 1;
        }
      }

      if ( addr >= (uint64_t) (uint32_t) CLUSTER_PERIPH_START && addr < (uint64_t) (uint32_t) CLUSTER_PERIPH_END ) {
        ret = 1;
      }
#ifdef __k1io__
      if ( addr >= (uint64_t) (uint32_t) &DDR_START && addr < ((uint64_t) (uint32_t) &DDR_START) + ((uint64_t) (uint32_t) &DDR_SIZE)) {
        ret = 1;
      }

      if ( addr >= (uint64_t) (uint32_t) IO_PERIPH_START && addr < (uint64_t) (uint32_t) IO_PERIPH_END ) {
        ret = 1;
      }
#endif
      if ( &VIRT_U_MEM_PAG != 0 ) {
        if(addr >= (uint64_t) (uint32_t) &VIRT_U_MEM_PAG && addr < (uint64_t) (uint32_t) &VIRT_U_MEM_END){
          ret = 1;
        }
      }
      break;
    }
    default:
      ret = 0;
      break;
  }

  return ret;
}

