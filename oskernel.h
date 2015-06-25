/* All the kernel functions that used to be in os.cpp are now here */
#ifndef _OSKERNEL_H_
#define _OSKERNEL_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "kernel.h"
#include "error_code.h"

/** This table contains all task descriptors, regardless of state, plus idler. */
extern task_descriptor_t task_desc[MAXPROCESS + 1];

/** Arguments for Task_Create() request. */
//static volatile create_args_t kernel_request_create_args;
extern volatile create_args_t kernel_request_create_args;

/** The task descriptor of the currently RUNNING task. */
extern task_descriptor_t* cur_task;

/** The ready queue for SYSTEM tasks. Their scheduling is first come, first served. */
extern queue_t system_queue;

/** The ready queue for tasks waiting on a SERVICE. Their scheduling is first come, first served. */
extern queue_t service_queue;

/** The current kernel request. */
extern volatile kernel_request_t kernel_request;

/** Return value for Task_Create() request. */
extern volatile int kernel_request_retval;



/* Forward declarations */

/* kernel */
void kernel_main_loop(void);
void kernel_dispatch(void);
void kernel_handle_request(void);

/* context switching */
void exit_kernel(void) __attribute((noinline, naked));
void enter_kernel(void) __attribute((noinline, naked));
extern "C" void TIMER1_COMPA_vect(void) __attribute__ ((signal, naked));

int kernel_create_task();
void kernel_terminate_task(void);

/* queues */
void enqueue(queue_t* queue_ptr, task_descriptor_t* task_to_add);
task_descriptor_t* dequeue(queue_t* queue_ptr);

void initialize_dead_pool_queue();

void kernel_update_ticker(void);

void idle (void);
void _delay_25ms(void);

#ifdef __cplusplus
}
#endif

#endif /* _OSKERNEL_H_ */
