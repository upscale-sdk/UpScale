#include "ee.h"

#if !defined(EE_CONF_LIBGOMP) && !defined(EE_JOB_TEST)

enum EE_ASSERTIONS {
  EE_ASSERT_FIN = 0,
  EE_ASSERT_INIT,
  EE_ASSERT_TASK1_FIRED,
  EE_ASSERT_TASK2_FIRED,
  EE_ASSERT_TASK1_POST,
  EE_ASSERT_TASK2_ENDED,
  EE_ASSERT_TASK1_ENDED,
  EE_ASSERT_TASK3_NOT_FIRED,
  EE_ASSERT_TASK3_FIRED,
  EE_ASSERT_TASKS_ENDED,
  EE_ASSERT_DIM
};
EE_TYPEASSERTVALUE EE_assertions[EE_ASSERT_DIM];

/* Final result */
volatile EE_TYPEASSERTVALUE result;

/* Counters */
EE_UREG volatile task1_fired;
EE_UREG volatile task2_fired;
EE_UREG volatile task1_ended;
EE_UREG volatile task2_ended;
EE_UREG volatile task3_fired;
EE_UREG volatile task3_ended;
EE_UREG volatile task4_fired;
EE_UREG volatile task5_fired;
EE_UREG volatile isr2_fired;
EE_UREG volatile isr2_armed;


/* Stack Pointers */
EE_ADDR volatile task1_sp;
EE_ADDR volatile task2_sp;
EE_ADDR volatile task3_sp;
EE_ADDR volatile task4_sp;
EE_ADDR volatile task5_sp;
EE_ADDR volatile main_sp;

/* Task IDs */
TaskType task1_id;
TaskType task2_id;
TaskType task3_id;
TaskType task4_id;
TaskType task5_id;
TaskType isr2_clock_id;

/* This semaphore is initialized inside the Background Task */
SemType V;

#define CLOCK_FREQ         (__bsp_frequency/(_K1_DEFAULT_CLOCK_DIV + 1))
#define CLOCK_RELOAD_1_MS  EE_MILLI_TO_TICKS(1, CLOCK_FREQ)

static void configure_clock ( void ) {
#ifndef EE_TRACE_KERNEL
  mOS_timer_general_setup();
  mOS_timer_setup_num(0, CLOCK_RELOAD_1_MS, CLOCK_RELOAD_1_MS, EE_FALSE);
#endif /*EE_TRACE_KERNEL */
}

#ifdef EE_TRACE_KERNEL
static void throw_isr ( void )  {
  mppa_tracepoint(erika,THROW_ISR_ENTER);
  __k1_interrupt_set_num_fixed(EE_ISR_ID_TIMER0);
}
#else
#define throw_isr() ((void)0)
#endif /* EE_TRACE_KERNEL */
ISR(clock_handler) {
  ++isr2_fired;
  if ( !isr2_armed  ) {
    ActivateTask(task1_id);
  } else {
    isr2_armed = 0U;
  }
}

/*
 * TASK 1
 */
TASK(Task1)
{
  EE_ADDR curr_sp;
#ifdef EE_TRACE_TASK
  mppa_tracepoint(erika,TASK1_ENTER);
#endif /* EE_TRACE_TASK */

  task1_fired++;
  if (task1_fired == 1) {
    EE_assert(EE_ASSERT_TASK1_FIRED, EE_TRUE, EE_ASSERT_INIT);
  } else {
    isr2_armed = 1U;
  }

  curr_sp = EE_get_SP();
  if ( task1_sp == 0 ) {
    task1_sp = curr_sp;
  } else if ( task1_sp != curr_sp ) {
    EE_BREAK_POINT();
  }

  ActivateTask(task2_id);

  if (task1_fired == 1) {
    EE_assert(EE_ASSERT_TASK1_POST, EE_TRUE, EE_ASSERT_TASK2_FIRED);
  }
  PostSem(&V);

  if (task1_fired == 1) {
    EE_assert(EE_ASSERT_TASK1_ENDED, EE_TRUE, EE_ASSERT_TASK2_ENDED);
    EE_assert(EE_ASSERT_TASK3_NOT_FIRED, task3_fired == 0, EE_ASSERT_TASK1_ENDED);
  } else while ( isr2_armed ) {
#ifdef EE_TRACE_KERNEL
    throw_isr()
#endif
    ; /* Wait ISR2 release */
  }
  curr_sp = EE_get_SP();
  if ( task1_sp != curr_sp ) {
    EE_BREAK_POINT();
  }
  task1_ended++;
#ifdef EE_TRACE_TASK
  mppa_tracepoint(erika,TASK1_EXIT);
#endif /* EE_TRACE_TASK */
}

/*
 * TASK 2
 */
TASK(Task2)
{
  EE_ADDR  curr_sp;
#ifdef EE_TRACE_TASK
  mppa_tracepoint(erika,TASK2_ENTER);
#endif /* EE_TRACE_TASK */
  curr_sp = EE_get_SP();
  if ( task2_sp == 0 ) {
    task2_sp = curr_sp;
  } else if ( task2_sp != curr_sp ) {
    EE_BREAK_POINT();
  }

  task2_fired++;
  if (task2_fired == 1) {
    EE_assert(EE_ASSERT_TASK2_FIRED, EE_TRUE, EE_ASSERT_TASK1_FIRED);
  }
#ifdef EE_TRACE_TASK
  mppa_tracepoint(erika,TASK2_EXIT);
#endif /* EE_TRACE_TASK */
  WaitSem(&V);
#ifdef EE_TRACE_TASK
  mppa_tracepoint(erika,TASK2_ENTER);
#endif /* EE_TRACE_TASK */

  if (task2_fired == 1) {
    EE_assert(EE_ASSERT_TASK2_ENDED, EE_TRUE, EE_ASSERT_TASK1_POST);
  }
  ActivateTask(task3_id);
  task2_ended++;
#ifdef EE_TRACE_TASK
  mppa_tracepoint(erika,TASK2_EXIT);
#endif /* EE_TRACE_TASK */
}

TASK(Task3) {
  EE_ADDR  curr_sp;

  curr_sp = EE_get_SP();
  if ( task3_fired > 0 ) {
    if ( task3_sp == 0 ) {
      task3_sp = curr_sp;
    } else if ( task3_sp != curr_sp ) {
      EE_BREAK_POINT();
    }
  }
  ++task3_fired;
  ActivateTask(task4_id);
  if (task3_fired == 1) {
    EE_assert(EE_ASSERT_TASK3_FIRED, EE_TRUE, EE_ASSERT_TASK3_NOT_FIRED);
  }
  ++task3_ended;
}

TASK(Task4) {
  EE_ADDR  curr_sp;

  curr_sp = EE_get_SP();
  if ( task4_fired > 0 ) {
    if ( task4_sp == 0 ) {
      task4_sp = curr_sp;
    } else if ( task4_sp != curr_sp ) {
      EE_BREAK_POINT();
    }
  }
  ++task4_fired;
  ActivateTask(task5_id);

  if ( task4_fired > 1 ) {
    curr_sp = EE_get_SP();
    if ( task4_sp != curr_sp ) {
      EE_BREAK_POINT();
    }
  }
}

TASK(Task5) {
  EE_ADDR  curr_sp;

  curr_sp = EE_get_SP();
  if ( task5_fired > 0 ) {
    if ( task5_sp == 0 ) {
      task5_sp = curr_sp;
    } else if ( task5_sp != curr_sp ) {
      EE_BREAK_POINT();
    }
  }
  ++task5_fired;
}

/*
 * MAIN TASK
 */
int main(void)
{
  EE_ADDR volatile curr_sp, curr_sp_after;
  EE_UREG counter = 0;

  CreateTask( &task1_id, EE_TASK_TYPE_BASIC, TASK_FUNC(Task1), 1U, 1U, 1U, 1024 );
  CreateTask( &task2_id, EE_TASK_TYPE_EXTENDED, TASK_FUNC(Task2), 2U, 2U, 1U, 1024 );
  CreateTask( &task3_id, EE_TASK_TYPE_BASIC, TASK_FUNC(Task3), 1U, 1U, 1U, SYSTEM_STACK );
  CreateTask( &isr2_clock_id, EE_TASK_TYPE_ISR2, clock_handler, 1U, 1U, 1U, SYSTEM_STACK );
  CreateTask( &task4_id, EE_TASK_TYPE_BASIC, TASK_FUNC(Task4), 1U, 1U, 1U, SYSTEM_STACK );
  CreateTask( &task5_id, EE_TASK_TYPE_BASIC, TASK_FUNC(Task5), 2U, 2U, 1U, SYSTEM_STACK );

  /* Prio settate a 1 dalla BSP all'avvio */
  SetISR2Source(isr2_clock_id, BSP_IT_TIMER_0);
  configure_clock ();

  EE_assert(EE_ASSERT_INIT, EE_TRUE, EE_ASSERT_NIL);

  /* Initialization of the second semaphore of the example;
   * the first semaphore is initialized inside the definition */
  InitSem(&V, 0);

  curr_sp = EE_get_SP();
  ActivateTask(task1_id);

  curr_sp_after = EE_get_SP();

  if ( curr_sp != curr_sp_after ) {
    EE_BREAK_POINT();
  }

  EE_assert(
    EE_ASSERT_TASKS_ENDED, task1_ended && task2_ended && task3_ended, EE_ASSERT_TASK3_FIRED
  );
  EE_assert_range(EE_ASSERT_FIN, EE_ASSERT_INIT, EE_ASSERT_TASKS_ENDED);
  result = EE_assert_last();

  /* Forever loop: background activities (if any) should go here */
  for (;result == 1;)
  {
    curr_sp = EE_get_SP();
    if ( curr_sp != curr_sp_after ) {
      EE_BREAK_POINT();
    }

    if ( main_sp == 0 ) {
      main_sp = curr_sp;
    } else if ( main_sp != curr_sp ) {
      EE_BREAK_POINT();
    }

    while ( counter % 10000 ) {
      counter++;
      throw_isr();
    }

    /*EG: Spostato nell'inturruzione */
    /* ActivateTask(task1_id); */
    counter++;
  }
  EE_BREAK_POINT();
  return !result;
}
#endif /* !EE_CONF_LIBGOMP && !EE_JOB_TEST */

