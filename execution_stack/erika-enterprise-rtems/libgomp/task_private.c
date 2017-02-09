#include "libgomp.h"

void gomp_task(gomp_task_t *parent, void (*fn) (void *), void *fn_data, int if_clause, unsigned flags);
void gomp_execute_task(gomp_task_t *task);
gomp_thread_t* gomp_scheduling(gomp_task_t *task);

inline gomp_task_t* task_malloc(gomp_thread_t *thread)
{	
	gomp_hal_lock(&thread->mem_lock);
	gomp_task_t *task = thread->free_task_list;
	if(task){
		thread->free_task_list = task->next_free_mem;
		task->mem_space = thread;
	}
	gomp_hal_unlock(&thread->mem_lock);
	return task;
}

inline void task_free(gomp_task_t *task)
{
	gomp_thread_t *thread = task->mem_space;

	gomp_hal_lock(&thread->mem_lock);
	task->next_free_mem = thread->free_task_list;
	thread->free_task_list = task;
	gomp_hal_unlock(&thread->mem_lock);
}

inline gomp_task_t* smart_task_malloc(gomp_thread_t *thread)
{
	//_printstrp("start smart_task_malloc()");
	gomp_task_t *task = task_malloc(thread);
	if(!task)
	{
		unsigned int i, p;
		gomp_thread_t *t;
		unsigned int pid = prv_proc_num;
		gomp_team_t *team = (gomp_team_t *) CURR_TEAM(pid);
		unsigned int nthreads = team->nthreads;

		for(i = 0; i < nthreads; i++)
		{
			p = i + 1 + pid;
			if(p >= nthreads)
				p -= nthreads;

			t = &team->thread[p];
			task = task_malloc(t);
			if(task)
				return task;
		}
		task = NULL;
	}
	//_printstrp("end smart_task_malloc()");
	return task;
}

inline void gomp_init_task(gomp_task_t *task, gomp_task_t *parent, void (*fn) (void *), void *data, void (*cpyfn) (void *, void *), long arg_size, long arg_align, unsigned flags)
{
	//_printstrp("start gomp_init_task()");
	task->task_q = NULL;
	task->parent = parent;
    task->child_waiting_q.task_count = 0;
	task->child_running_q.task_count = 0;

	task->status = GOMP_TASK_WAITING;
	//task->taskwait_barrier = &task->_taskwait_barrier;
	//task->_taskwait_barrier  = 0xFFFFFFFFU;
        gomp_hal_init_lock((omp_lock_t *) &task->taskwait_barrier);
	if(flags)
		task->kind = GOMP_TASK_UNTIED;
	else
		task->kind = GOMP_TASK_TIED;

	task->fn = fn;
	if(arg_size)
	{
		if(cpyfn)
			cpyfn(task->fn_data, data);
		else
		{
			unsigned int i, arg_size_word = arg_size >> 2;
            if(arg_size_word >= NUM_TASK_PARAMETERS)
            {
            	printf("[ERROR] Too many parameters");
				abort();
            }
			for(i = 0; i < arg_size_word; i++)
				task->fn_data[i] = ((unsigned int *) data)[i];
		}
    }
	//_printstrp("end gomp_init_task()");
}

inline void gomp_end_task(gomp_task_t *task)
{
	task_free(task);
}

inline void gomp_thread_wake_up(gomp_thread_t *thread)
{
// #ifdef TASK_BARRIER_SLEEP_WAKEUP
	gomp_hal_unlock(&thread->sleep_lock);
// #else // TASK_BARRIER_BUSY_WAITING
	// WAKE UP BARRIER
// #endif // TASK_BARRIER_BUSY_WAITING
}

inline void gomp_thread_sleep(gomp_thread_t *thread)
{
// #ifdef TASK_BARRIER_SLEEP_WAKEUP
	gomp_hal_lock(&thread->sleep_lock);
// #else // TASK_BARRIER_BUSY_WAITING
	// SLEEP IN BARRIER
// #endif // TASK_BARRIER_BUSY_WAITING
}

inline void gomp_taskwait_wake_up(gomp_task_t *task)
{
// #ifdef TASK_BARRIER_SLEEP_WAKEUP
	gomp_hal_unlock(&task->taskwait_barrier);
// #else // TASK_BARRIER_BUSY_WAITING
	// WAKE UP BARRIER
// #endif // TASK_BARRIER_BUSY_WAITING
}

inline void gomp_taskwait_sleep(gomp_task_t *task)
{
// #ifdef TASK_BARRIER_SLEEP_WAKEUP
	gomp_hal_lock(&task->taskwait_barrier);
// #else // TASK_BARRIER_BUSY_WAITING
	// SLEEP IN BARRIER
// #endif // TASK_BARRIER_BUSY_WAITING
}

inline void push_task(gomp_task_t *task, gomp_queue_t *queue)
{
	if(queue->task_count == 0)
	{
		queue->first_task = task;
		queue->last_task = task;
	}
	else
	{
		queue->first_task->prev_task = task;
		task->next_task = queue->first_task;
		queue->first_task = task;
	}
	++queue->task_count;
	task->task_q = queue;
}

inline unsigned int pop_task(gomp_task_t **task, gomp_queue_t *queue)
{
	if(queue->task_count == 0)
		return 0;
	else
	{
		*task = queue->last_task;
		queue->last_task = (*task)->prev_task;
		--queue->task_count;
		(*task)->task_q = NULL;
		return 1;
	}
}

inline void remove_task(gomp_task_t *task, gomp_queue_t *queue)
{
	if(queue->last_task == task)
		queue->last_task = task->prev_task;
	else if(queue->first_task == task)
		queue->first_task = task->next_task;
	else
	{
		task->prev_task->next_task = task->next_task;
		task->next_task->prev_task = task->prev_task;
	}
	--queue->task_count;
	task->task_q = NULL;
}

inline void push_child(gomp_task_t *task, gomp_queue_t *queue)
{
	if(queue->task_count == 0)
	{
		queue->first_task = task;
		queue->last_task = task;
	}
	else
	{
		queue->first_task->prev_child = task;
		task->next_child = queue->first_task;
		queue->first_task = task;
	}
	++queue->task_count;
}

inline unsigned int pop_child(gomp_task_t **task, gomp_queue_t *queue)
{
	if(queue->task_count == 0)
		return 0;
	else
	{
		*task = queue->last_task;
		queue->last_task = (*task)->prev_child;
		--queue->task_count;
		return 1;
	}
}

inline void remove_child(gomp_task_t *task, gomp_queue_t *queue)
{
	if(queue->last_task == task)
		queue->last_task = task->prev_child;
	else if(queue->first_task == task)
		queue->first_task = task->next_child;
	else
	{
		task->prev_child->next_child = task->next_child;
		task->next_child->prev_child = task->prev_child;
	}
	--queue->task_count;
}

#ifdef STATIC_TDG_ENABLED

// Promote the task to the READY queue
inline void gomp_ready(struct gomp_task *task)
{
  unsigned int pid = prv_proc_num;
  gomp_team_t *team = (gomp_team_t *) CURR_TEAM(pid);
  gomp_task_t *parent = task->parent;
  gomp_thread_t *thread = gomp_scheduling(task);

  // 2. Push the task into the READY queues
  push_task(task, &thread->task_q);
  push_child(task, &parent->child_waiting_q);
}
#endif

inline void gomp_clean_parent(gomp_task_t *parent)
{
	//_printstrp("start gomp_clean_parent");
  	if((parent->child_running_q.task_count + parent->child_waiting_q.task_count) > 0)
  	{
    	unsigned int i, task_count;

    	gomp_task_t *task = parent->child_waiting_q.first_task;
    	task_count = parent->child_waiting_q.task_count;
		for(i = 0; i < task_count; i++)
		{
			task->parent = NULL;
			task = task->next_child;
		}
		task = parent->child_running_q.first_task;
		task_count = parent->child_running_q.task_count;
		for(i = 0; i < task_count; i++){
			task->parent = NULL;
			task = task->next_child;
		}
	}
	//_printstrp("end gomp_clean_parent");
}

inline gomp_thread_t* gomp_scheduling(gomp_task_t *task)
{
	unsigned int pid = prv_proc_num;
	gomp_team_t *team = (gomp_team_t *) CURR_TEAM(pid);
	int core_map = task->tdg->map;

	if(core_map <0 || core_map >15) {
		printf("[WARNING!] In gomp_scheduling, task %d has an invalid core assigned (%d). Core 0 is used instead\n",task->tdg->id,core_map);
		core_map = 0;
	}
    return &team->thread[core_map];
	//gomp_thread_t *thread = &team->thread[pid];
	//unsigned int nthreads = team->nthreads;

	//thread->random++;
	//if(thread->random >= nthreads)
	//	thread->random -= nthreads;

    //return &team->thread[thread->random];
    //return &team->thread[task->tdg->map];
}

void gomp_start(gomp_task_t *task)
{
	task->status = GOMP_TASK_RUNNING;
	task->fn(task->fn_data);
	task->status = GOMP_TASK_FINISHED;

	if(task->kind == GOMP_TASK_UNTIED)
	{
		unsigned int pid = prv_proc_num;
		gomp_team_t *team = (gomp_team_t *) CURR_TEAM(pid);
		gomp_thread_t *thread = &team->thread[pid];
		load_context(thread->registers);
	}
}

inline void gomp_finalize(gomp_task_t *task)
{
	//_printstrp("gomp_finalize start");
	unsigned int pid = prv_proc_num;
	gomp_team_t *team = (gomp_team_t *) CURR_TEAM(pid);

	gomp_hal_lock(&team->task_lock);
	gomp_clean_parent(task);
	gomp_task_t *parent = task->parent;
	if(parent)
	{
		remove_child(task, &parent->child_running_q);
		if(parent->status == GOMP_TASK_IN_TASKWAIT)
		{
      		if(parent->child_waiting_q.task_count == 0 && parent->child_running_q.task_count == 0)
			{
				if(parent->kind == GOMP_TASK_UNTIED)
				{
					gomp_thread_t *thread = &team->thread[pid];
					gomp_task_t *gran_parent = parent->parent;
					if(gran_parent)
					{
						remove_child(parent, &gran_parent->child_running_q);
            			push_child(parent, &gran_parent->child_waiting_q);
          			}
					push_task(parent, &thread->task_q);
					gomp_thread_wake_up(thread);
				}
				else
					gomp_taskwait_wake_up(parent);
			}
		}
	}
#ifdef STATIC_TDG_ENABLED
  	gomp_deltask_dep(task);
#endif
	gomp_end_task(task);
	gomp_hal_unlock(&team->task_lock);
	//_printstrp("gomp_finalize end");
}

inline void gomp_suspending(gomp_task_t *task)
{
	//_printstrp("gomp_suspending start");
	unsigned int pid = prv_proc_num;
	gomp_team_t *team = (gomp_team_t *) CURR_TEAM(pid);
	gomp_thread_t *thread = &team->thread[pid];

	gomp_thread_t *deferred_thread = NULL;
	gomp_thread_t *suspended_thread = gomp_scheduling(task);
	if(thread->deferred_task)
		deferred_thread = gomp_scheduling(thread->deferred_task);

	gomp_hal_lock(&team->task_lock);
	gomp_task_t *parent = task->parent;
	if(parent)
	{
		remove_child(task, &parent->child_running_q);
    	push_child(task, &parent->child_waiting_q);
  	}
	push_task(task, &suspended_thread->task_q);
	gomp_thread_wake_up(suspended_thread);

	if(thread->undeferred_task)
	{
		gomp_task_t *t = thread->undeferred_task;
		thread->undeferred_task = NULL;
		if(t->parent)
      		push_child(t, &t->parent->child_waiting_q);
		gomp_hal_unlock(&team->task_lock);

		gomp_execute_task(t);
	}
	else if(thread->deferred_task)
	{
		gomp_task_t *t = thread->deferred_task;
		thread->deferred_task = NULL;
		if(t->parent)
			push_child(t, &t->parent->child_waiting_q);
		push_task(t, &deferred_thread->task_q);
		gomp_hal_unlock(&team->task_lock);

		gomp_thread_wake_up(deferred_thread);
	}
	else
		gomp_hal_unlock(&team->task_lock);
	//_printstrp("gomp_suspending end");
}

void gomp_execute_task(gomp_task_t *task)
{
	//_printstrp("start gomp_execute_task");
	unsigned int pid = prv_proc_num;
	gomp_team_t *team = (gomp_team_t *) CURR_TEAM(pid);
	gomp_thread_t *thread = &team->thread[pid];
	gomp_task_t *exe_task = thread->curr_task;
	
	//_printstrp("start execute");
	thread->curr_task = task;
	if(task->status == GOMP_TASK_WAITING)
	{
		if(task->kind == GOMP_TASK_TIED)
			gomp_start(task);
		else if(task->kind == GOMP_TASK_UNTIED)
		{
			start_context((void *) task, (void *) &task->stack[TASK_STACK_SIZE-1], (void *) thread->registers);
			if(task->status == GOMP_TASK_RUNNING)
				task->status = GOMP_TASK_SUSPENDED;
		}
	}	
	else if(task->status == GOMP_TASK_SUSPENDED)
	{
		task->status = GOMP_TASK_RUNNING;
		swap_context((void *) task->registers, (void *) thread->registers);
		if(task->status == GOMP_TASK_RUNNING)
			task->status = GOMP_TASK_SUSPENDED;
	}
	else if(task->status == GOMP_TASK_IN_TASKWAIT)
    	if(task->child_waiting_q.task_count == 0 && task->child_running_q.task_count == 0)
		{
			task->status = GOMP_TASK_RUNNING;
			swap_context((void *) task->registers, (void *) thread->registers);
			if(task->status == GOMP_TASK_RUNNING)
				task->status = GOMP_TASK_SUSPENDED;
		}
	thread->curr_task = exe_task;
	//_printstrp("end execute");

	if(task->status == GOMP_TASK_FINISHED)
		gomp_finalize(task);
	else if(task->status == GOMP_TASK_SUSPENDED)
		gomp_suspending(task);
	//_printstrp("end gomp_execute_task");
}

inline void gomp_task_scheduler()
{
	//_printstrp("start gomp_task_scheduler");
	unsigned int p, pid = prv_proc_num;
	gomp_team_t *team = (gomp_team_t *) CURR_TEAM(pid);
	unsigned int nthreads = team->nthreads;
	gomp_thread_t *thread = &team->thread[pid];
	gomp_task_t *task;
	
	gomp_hal_lock(&team->task_lock);
	while(1)
	{
		if(pop_task(&task, &thread->task_q))
		{
			if(task->parent)
			{
        		remove_child(task, &task->parent->child_waiting_q);
				push_child(task, &task->parent->child_running_q);
			}
			gomp_hal_unlock(&team->task_lock);
			
			gomp_execute_task(task);

			gomp_hal_lock(&team->task_lock);
			continue;
		}
		++team->pending_threads;
		gomp_hal_unlock(&team->task_lock);

		if(team->pending_threads == nthreads)
		{
			gomp_hal_unlock(&team->task_lock);
			p = pid+1;
			if(p >= nthreads)
				p = 0;
			gomp_thread_wake_up(&team->thread[p]);
			gomp_hal_barrier(0x0);
			team->pending_threads = 0;
			return;
		}
		
		gomp_thread_sleep(thread);
		
		if(team->pending_threads == nthreads)
		{
			gomp_hal_unlock(&team->task_lock);
			p = pid+1;
			if(p >= nthreads)
				p = 0;
			gomp_thread_wake_up(&team->thread[p]);
			gomp_hal_barrier(0x0);
			return;
		}

		gomp_hal_lock(&team->task_lock);
		--team->pending_threads;
	}
}

#ifdef STATIC_TDG_ENABLED
// The lowering of Mercurium includes the void **depend for dynamic support of dependencies
// FIXME: Mercurium task lowering seems not to support if clause 
void GOMP_task(void (*fn) (void *), void *data, void (*cpyfn) (void *, void *), long arg_size, long arg_align, int if_clause, unsigned flags __attribute__((unused)), void **depend, unsigned id)
#else
void GOMP_task(void (*fn) (void *), void *data, void (*cpyfn) (void *, void *), long arg_size, long arg_align, int if_clause, unsigned flags __attribute__((unused)))
#endif
{
	//_printstrp("start GOMP_task");
	//opt_get_cycle();
	unsigned int pid = prv_proc_num;
	gomp_team_t *team = (gomp_team_t *) CURR_TEAM(pid);

	gomp_thread_t *thread = &team->thread[pid];
	gomp_task_t *exe_task = thread->curr_task;

#ifdef STATIC_TDG_ENABLED
  	int tdg_i;
  	gomp_hal_lock(&team->task_lock);
  	tdg_i = gomp_register_new_task_in_tdg(id);
  	gomp_hal_unlock(&team->task_lock);
#endif

  	if(exe_task->status == GOMP_TASK_CUTOFF)
  	{
#ifdef STATIC_TDG_ENABLED
    	// FIXME: The thread is blocked in an active waiting until all dependencies are solved.
    	// This may be improved by releasing the thread to execute another task
		while(gomp_get_unresolved_dependencies(tdg_i));
#endif
		fn(data);
		return;
	}

	gomp_task_t *task = smart_task_malloc(thread);

	if(!task)
	{
		//_printstrp("[ERROR] OUT OF MEMORY");
		//abort();
		enum gomp_task_status status = exe_task->status;
#ifdef STATIC_TDG_ENABLED
	    // FIXME: The thread is blocked in an active waiting until all dependencies are solved.
    	// This may be improved by releasing the thread to execute another task
		while(gomp_get_unresolved_dependencies(tdg_i));
#endif
		exe_task->status = GOMP_TASK_CUTOFF;
		fn(data);
		exe_task->status = status;
		return;
	}

	gomp_init_task(task, exe_task, fn, data, cpyfn, arg_size, arg_align, flags);

	if(exe_task->kind == GOMP_TASK_UNTIED)
	{
#ifdef STATIC_TDG_ENABLED
  		gomp_set_tdg_to_task(task,tdg_i);
#endif
		if(!if_clause || WORK_FIRST_SCHED)
			thread->undeferred_task = task;
		else
			thread->deferred_task = task;
		swap_context((void *) thread->registers, (void *) exe_task->registers);
	}
	else
	{
		if(!if_clause || WORK_FIRST_SCHED)
		{
			gomp_hal_lock(&team->task_lock);
#ifdef STATIC_TDG_ENABLED
  			gomp_set_tdg_to_task(task,tdg_i);
#endif
			push_child(task, &exe_task->child_running_q);
			gomp_hal_unlock(&team->task_lock);
			
			gomp_execute_task(task);
		}
		else
		{
#ifndef STATIC_TDG_ENABLED
			gomp_thread_t *sched_thread = gomp_scheduling(task);
#endif
			gomp_hal_lock(&team->task_lock);
#ifdef STATIC_TDG_ENABLED
  	  		gomp_set_tdg_to_task(task,tdg_i);
			gomp_thread_t *sched_thread = gomp_scheduling(task);
      		// If all dependent tasks are finished, put into the ready queue
      		if(!gomp_get_unresolved_dependencies(tdg_i)) {
        		push_child(task, &exe_task->child_waiting_q);
        		push_task(task, &sched_thread->task_q);
      		}
#else
      		push_child(task, &exe_task->child_waiting_q);
			push_task(task, &sched_thread->task_q);
#endif
			gomp_hal_unlock(&team->task_lock);

			gomp_thread_wake_up(sched_thread);
		}
	}
	//opt_get_cycle();
	//_printstrp("end GOMP_task");
}

void GOMP_taskwait(void)
{
	//_printstrp("GOMP_taskwait start");
	unsigned int pid = prv_proc_num;
	gomp_team_t *team = (gomp_team_t *) CURR_TEAM(pid);
	gomp_thread_t *thread = &team->thread[pid];
	gomp_task_t *exe_task = thread->curr_task;

	if(exe_task->status == GOMP_TASK_CUTOFF)
		return;

	gomp_hal_lock(&team->task_lock);
  	if(exe_task->child_waiting_q.task_count == 0 && exe_task->child_running_q.task_count == 0)
	{
		gomp_hal_unlock(&team->task_lock);
		return;
	}
	else
	{
		exe_task->status = GOMP_TASK_IN_TASKWAIT;

		if(exe_task->kind == GOMP_TASK_UNTIED)
		{
			gomp_hal_unlock(&team->task_lock);

			swap_context((void *) thread->registers, (void *) exe_task->registers);
			return;
		}
		else
		{
			gomp_task_t *task;
			while(1)
			{
        		if(pop_child(&task, &exe_task->child_waiting_q)){
					push_child(task, &exe_task->child_running_q);
					remove_task(task, task->task_q);
					gomp_hal_unlock(&team->task_lock);
					
					gomp_execute_task(task);
					
					gomp_hal_lock(&team->task_lock);
					continue;
				}
        		if(exe_task->child_waiting_q.task_count == 0 && exe_task->child_running_q.task_count == 0)
				{
					gomp_hal_unlock(&team->task_lock);

					exe_task->status = GOMP_TASK_RUNNING;
					return;
				}
				gomp_hal_unlock(&team->task_lock);
				
				gomp_taskwait_sleep(exe_task);

				team = (gomp_team_t *) CURR_TEAM(pid);
				gomp_hal_lock(&team->task_lock);
			}
		}
	}
}
