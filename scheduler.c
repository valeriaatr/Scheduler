#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include "scheduler.h"

#include <assert.h>
#include <curses.h>
#include <ucontext.h>
#include <time.h>

#include "util.h"


// This is an upper limit on the number of tasks we can create.
#define MAX_TASKS 128
#define READY 0
#define BLOCK 1
#define EXIT 2

// This is the size of each task's stack memory
#define STACK_SIZE 65536

// This struct will hold the all the necessary information for each task
typedef struct task_info {
  // This field stores all the state required to switch back to this task
  ucontext_t context;
  
  // This field stores another context. This one is only used when the task
  // is exiting.
  ucontext_t exit_context;
  
  // TODO: Add fields here so you can:
  task_t task_id;
  //   a. Keep track of this task's state.
  int state;
  //   b. If the task is sleeping, when should it wake up?
  time_t wait_time;
  //   c. If the task is waiting for another task, which task is it waiting for?
  task_t task_waited;
  //   d. Was the task blocked waiting for user input? Once you successfully
  //      read input, you will need to save it here so it can be returned.
  int chr;
} task_info_t;

int current_task = 0; //< The handle of the currently-executing task
int num_tasks = 1;    //< The number of tasks created so far ---why it was =1?
task_info_t tasks[MAX_TASKS]; //< Information for every task
//int c;
/**
 * Initialize the scheduler. Programs should call this before calling any other
 * functiosn in this file.
 */
void scheduler_init() {
  // TODO: Initialize the state of the scheduler 
  for (int i=0;i<MAX_TASKS;i++){
	tasks[i].state = READY;
	tasks[i].wait_time = 0;
	tasks[i].chr = 0;
	//tasks[i].task_id = i;
  }
  //
}
//round robin scheduler, finds task that is READY and swaps contexts.
void schedule(int handle) { //the task handler
	int i;
	//round robin
  while (1){ //until you find one that is ready tasks[t].state!=READY
	
	//if task is blocked, swap context
	//if (tasks[t].state==BLOCK || tasks[t].state==EXIT){
	for(i=handle; i<num_tasks;i++){ //find next task that is active and READY
		if (tasks[i].state==READY)
			break;
		else if(tasks[i].state==BLOCK && difftime(tasks[i].wait_time,time(NULL)) == 0){
			//if it's blocked but time elapsed, unblock and reset time
			tasks[i].state=READY;
			tasks[i].wait_time=0;
			break;
		}
		else if (i==num_tasks-1){ //circle back to start
		i = -1; //i++ will set it to 0
		}
		else continue;
	}
		if(tasks[handle]==EXIT){ //if it's ready to exit, swap
			swapcontext(&tasks[handle].exit_context,&tasks[i].context);
			break;
		}
		else{
			swapcontext(&tasks[handle].context,&tasks[i].context);
			break;
		}
  }
	current_task = i; //update such that current task is the one that is READY
}
/**
 * This function will execute when a task's function returns. This allows you
 * to update scheduler states and start another task. This function is run
 * because of how the contexts are set up in the task_create function.
 */
void task_exit() {
  // TODO: Handle the end of a task's execution here
  tasks[current_task].state = EXIT;
  schedule(current_task);
}

/**
 * Create a new task and add it to the scheduler.
 *
 * \param handle  The handle for this task will be written to this location.
 * \param fn      The new task will run this function.
 */
void task_create(task_t* handle, task_fn_t fn) {
	//specify task_id
	tasks[current_task].task_id = current_task;
	
  // Claim an index for the new task
  int index = num_tasks;
  num_tasks++;
  
  // Set the task handle to this index, since task_t is just an int
  *handle = index;
 
  // We're going to make two contexts: one to run the task, and one that runs at the end of the task so we can clean up. Start with the second
  
  // First, duplicate the current context as a starting point
  getcontext(&tasks[index].exit_context);
  
  // Set up a stack for the exit context
  tasks[index].exit_context.uc_stack.ss_sp = malloc(STACK_SIZE);
  tasks[index].exit_context.uc_stack.ss_size = STACK_SIZE;
  
  // Set up a context to run when the task function returns. This should call task_exit.
  makecontext(&tasks[index].exit_context, task_exit, 0);
  
  // Now we start with the task's actual running context
  getcontext(&tasks[index].context);
  
  // Allocate a stack for the new task and add it to the context
  tasks[index].context.uc_stack.ss_sp = malloc(STACK_SIZE);
  tasks[index].context.uc_stack.ss_size = STACK_SIZE;
  
  // Now set the uc_link field, which sets things up so our task will go to the exit context when the task function finishes
  tasks[index].context.uc_link = &tasks[index].exit_context;
  
  // And finally, set up the context to execute the task function
  makecontext(&tasks[index].context, fn, 0);
  
  current_task++;
  //schedule?
}

/**
 * Wait for a task to finish. If the task has not yet finished, the scheduler should
 * suspend this task and wake it up later when the task specified by handle has exited.
 *
 * \param handle  This is the handle produced by task_create
 */
void task_wait(task_t handle) { //handle is task id;
  // TODO: Block this task until the specified task has exited.
  tasks[current_task].state = BLOCK;
  tasks[handle].state = EXIT;
  schedule(handle);
}

/**
 * The currently-executing task should sleep for a specified time. If that time is larger
 * than zero, the scheduler should suspend this task and run a different task until at least
 * ms milliseconds have elapsed.
 * 
 * \param ms  The number of milliseconds the task should sleep.
 */
void task_sleep(size_t ms) {
  // TODO: Block this task until the requested time has elapsed.
  // Hint: Record the time the task should wake up instead of the time left for it to sleep. The bookkeeping is easier this way.
  tasks[current_task].state = BLOCK;
  time_t seconds = time(NULL); //*1000? for ms
  tasks[current_task].wait_time = seconds + (int)(ms/1000); //when should it wake up? current time + given time
}

/**
 * Read a character from user input. If no input is available, the task should
 * block until input becomes available. The scheduler should run a different
 * task while this task is blocked.
 *
 * \returns The read character code
 */
int task_readchar() {
  // TODO: Block this task until there is input available.
  // To check for input, call getch(). If it returns ERR, no input was available.
  // Otherwise, getch() will returns the character code that was read.
  tasks[current_task].state = BLOCK;
  int c = getch();
  if (c!=ERR){
	tasks[current_task].state = READY;
	tasks[current_task].chr = c;
	return c;
  }
  else
	return ERR; //task is not READY since there was no input
}
