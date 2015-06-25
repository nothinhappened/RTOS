/**
 * @file oskernel.cpp
 *
 * @brief A Real Time Operating System
 *
 * All the kernel functions that used to be in os.cpp are now here
 *
 * Original authors:
 * @author Scott Craig
 * @author Justin Tanner
 * 
 * Modifications (2015) by:
 * @author Fraser DeLisle
 * @author Logan Bissonnette
 */

#include <util/delay.h>
#include "oskernel.h"
#include "periodic_scheduler.h"

/** Since this is a "full-served" model, the kernel is executing using its own stack. */
volatile uint16_t kernel_sp;

/** Arguments for Task_Create() request. */
volatile create_args_t kernel_request_create_args;

/** The current kernel request. */
volatile kernel_request_t kernel_request;

/** Return value for Task_Create() request. */
volatile int kernel_request_retval;

/** The task descriptor of the currently RUNNING task. */
task_descriptor_t* cur_task = NULL;

/** This table contains all task descriptors, regardless of state, plus idler. */
task_descriptor_t task_desc[MAXPROCESS + 1];

/** The special "idle task" at the end of the descriptors array. */
task_descriptor_t* idle_task = &task_desc[MAXPROCESS];

/** The ready queue for RR tasks. Their scheduling is round-robin. */
queue_t rr_queue;

/** The ready queue for SYSTEM tasks. Their scheduling is first come, first served. */
queue_t system_queue;

/** The ready queue for tasks waiting on a SERVICE. Their scheduling is first come, first served. */
queue_t service_queue;


/** Number of tasks created so far */
static queue_t dead_pool_queue;

/** time remaining in current slot */
volatile uint8_t ticks_remaining = 0;

/* A pointer to the currently running periodic task or maybe to the last-executed periodic task if it completed and no new (periodic) task has been started  */
task_t* periodic_task = &EmptyStruct;


/*
 * FUNCTIONS
 */
/**
 *  @brief The idle task does nothing but busy loop.
 */
void idle (void)
{
    for(;;)
    {};
}


/**
 * @fn kernel_main_loop
 *
 * @brief The heart of the RTOS, the main loop where the kernel is entered and exited.
 *
 * The complete function is:
 *
 *  Loop
 *<ol><li>Select and dispatch a process to run</li>
 *<li>Exit the kernel (The loop is left and re-entered here.)</li>
 *<li>Handle the request from the process that was running.</li>
 *<li>End loop, go to 1.</li>
 *</ol>
 */
void kernel_main_loop(void)
{
    for(;;)
    {
        kernel_dispatch();

        exit_kernel();

        /* if this task makes a system call, or is interrupted,
         * the thread of control will return to here. */

        kernel_handle_request();
    }
}


/**
 * @fn kernel_dispatch
 *
 *@brief The second part of the scheduler.
 *
 * Chooses the next task to run.
 *
 */
void kernel_dispatch(void)
{
    /* If the current state is RUNNING, then select it to run again.
     * kernel_handle_request() has already determined it should be selected.
     */

    if(cur_task->state != RUNNING || cur_task == idle_task)
    {
		// look for tasks waiting on a published service
		if(service_queue.head != NULL) {
			cur_task = dequeue(&service_queue);
		}
		
		else if(system_queue.head != NULL)
        {
            cur_task = dequeue(&system_queue);
        }
        // HOW TO PROCEED: check if there are any period tasks and then set them to running (run them).
        // let them run to completion -- 
        // have an array of Periodic tasks		
		/* periodic tasks must set iteration_complete to zero when they have completed whatever they do -- otherwise the system will fail when the task over-runs the wcet. */
		else if(periodic_task->iteration_complete == 0) {
			if(Scheduler_Dispatch() != 0 ) {
				// this is probably a bad situation, because there is a periodic task running and another is ready to run X_X
				error_msg = ERR_RUN_3_PERIODIC_TOOK_TOO_LONG;
				OS_Abort();
			}
			// if the current periodic task is still running, let it run
			cur_task = periodic_task->descriptor;
		}
		/* Let's see if there is a different periodic task which is ready to run! */
        else if( Scheduler_Dispatch() == 0 ) {
			periodic_task = Scheduler_GetTask();
			// setting the ticks_remaining value here from the periodic task's wcet.
			ticks_remaining = periodic_task->wcet;
			cur_task = periodic_task->descriptor;
        }
        else if(rr_queue.head != NULL)
        {
            cur_task = dequeue(&rr_queue);
        }
        else
        {
            /* No task available, so idle. */
            cur_task = idle_task;
        }

        cur_task->state = RUNNING;
    }
}


/**
 * @fn kernel_handle_request
 *
 *@brief The first part of the scheduler.
 *
 * Perform some action based on the system call or timer tick.
 * Perhaps place the current process in a ready or waiting queue.
 */
void kernel_handle_request(void)
{
   switch(kernel_request)
    {
    case NONE:
        /* Should not happen. */
        break;

    case TIMER_EXPIRED:
        kernel_update_ticker();

        /* Round robin tasks get pre-empted on every tick. */
        if(cur_task->level == RR && cur_task->state == RUNNING)
        {
            cur_task->state = READY;
            enqueue(&rr_queue, cur_task);
        }
        break;

    case TASK_CREATE:
        kernel_request_retval = kernel_create_task();

        /* Check if new task has higher priority, and that it wasn't an ISR
         * making the request.
         */
        if(kernel_request_retval)
        {
            /* If new task is SYSTEM and cur is not, then don't run old one */
            if(kernel_request_create_args.level == SYSTEM && cur_task->level != SYSTEM)
            {
                cur_task->state = READY;
            }

            /* If cur is RR, it might be pre-empted by a new PERIODIC. */
            if(cur_task->level == RR && kernel_request_create_args.level == PERIODIC && kernel_request_create_args.start == 0)
            {
                cur_task->state = READY;
            }

            /* enqueue READY RR tasks. */
            if(cur_task->level == RR && cur_task->state == READY)
            {
                enqueue(&rr_queue, cur_task);
            }
        }
        break;

    case TASK_TERMINATE:
		if(cur_task != idle_task)
		{
        	kernel_terminate_task();
		}
        break;

    case TASK_NEXT:
		switch(cur_task->level)
		{
	    case SYSTEM:
	        if(cur_task->state != WAITING) {
				enqueue(&system_queue, cur_task);
			}
			break;

	    case PERIODIC:
			// FIXME: What should we be doing here? -- perhaps setting ticks_remaining ? or perhaps setting periodic_task->iteration_complete to 1 ?
	        //slot_task_finished = 1;
			periodic_task->iteration_complete = 1;
	        break;

	    case RR:
			if(cur_task->state != WAITING) {
				enqueue(&rr_queue, cur_task);
			}
	        break;

	    default: /* idle_task */
			break;
		}

		cur_task->state = READY;
        break;

    case TASK_GET_ARG:
        /* Should not happen. Handled in task itself. */
		// FIXME: we probably will need to do something here.
        break;
		
    default:
        /* Should never happen */
        error_msg = ERR_RUN_8_RTOS_INTERNAL_ERROR;
        OS_Abort();
        break;
    }

    kernel_request = NONE;
}


/*
 * Context switching
 */
/**
 * It is important to keep the order of context saving and restoring exactly
 * in reverse. Also, when a new task is created, it is important to
 * initialize its "initial" context in the same order as a saved context.
 *
 * Save r31 and SREG on stack, disable interrupts, then save
 * the rest of the registers on the stack. In the locations this macro
 * is used, the interrupts need to be disabled, or they already are disabled.
 */
#define    SAVE_CTX_TOP()       asm volatile (\
	"EIND = 0X3C			\n\t"\
    "push   r31             \n\t"\
    "in		r31,EIND		\n\t"\
    "push	r31				\n\t"\
	"in     r31,__SREG__    \n\t"\
    "cli                    \n\t"::); /* Disable interrupt */

#define STACK_SREG_SET_I_BIT()    asm volatile (\
    "ori    r31, 0x80        \n\t"::);

#define    SAVE_CTX_BOTTOM()       asm volatile (\
    "push   r31             \n\t"\
    "push   r30             \n\t"\
    "push   r29             \n\t"\
    "push   r28             \n\t"\
    "push   r27             \n\t"\
    "push   r26             \n\t"\
    "push   r25             \n\t"\
    "push   r24             \n\t"\
    "push   r23             \n\t"\
    "push   r22             \n\t"\
    "push   r21             \n\t"\
    "push   r20             \n\t"\
    "push   r19             \n\t"\
    "push   r18             \n\t"\
    "push   r17             \n\t"\
    "push   r16             \n\t"\
    "push   r15             \n\t"\
    "push   r14             \n\t"\
    "push   r13             \n\t"\
    "push   r12             \n\t"\
    "push   r11             \n\t"\
    "push   r10             \n\t"\
    "push   r9              \n\t"\
    "push   r8              \n\t"\
    "push   r7              \n\t"\
    "push   r6              \n\t"\
    "push   r5              \n\t"\
    "push   r4              \n\t"\
    "push   r3              \n\t"\
    "push   r2              \n\t"\
    "push   r1              \n\t"\
    "push   r0              \n\t"::);

/**
 * @brief Push all the registers and SREG onto the stack.
 */
#define    SAVE_CTX()    SAVE_CTX_TOP();SAVE_CTX_BOTTOM();

/**
 * @brief Pop all registers and the status register.
 */
#define    RESTORE_CTX()    asm volatile (\
	"EIND = 0X3C			\n\t"\
    "pop    r0                \n\t"\
    "pop    r1                \n\t"\
    "pop    r2                \n\t"\
    "pop    r3                \n\t"\
    "pop    r4                \n\t"\
    "pop    r5                \n\t"\
    "pop    r6                \n\t"\
    "pop    r7                \n\t"\
    "pop    r8                \n\t"\
    "pop    r9                \n\t"\
    "pop    r10             \n\t"\
    "pop    r11             \n\t"\
    "pop    r12             \n\t"\
    "pop    r13             \n\t"\
    "pop    r14             \n\t"\
    "pop    r15             \n\t"\
    "pop    r16             \n\t"\
    "pop    r17             \n\t"\
    "pop    r18             \n\t"\
    "pop    r19             \n\t"\
    "pop    r20             \n\t"\
    "pop    r21             \n\t"\
    "pop    r22             \n\t"\
    "pop    r23             \n\t"\
    "pop    r24             \n\t"\
    "pop    r25             \n\t"\
    "pop    r26             \n\t"\
    "pop    r27             \n\t"\
    "pop    r28             \n\t"\
    "pop    r29             \n\t"\
    "pop    r30             \n\t"\
    "pop    r31             \n\t"\
    "out    __SREG__, r31    \n\t"\
    "pop    r31             \n\t"\
	"out	EIND, r31		\n\t"\
    "pop    r31             \n\t"::);


/**
 * @fn exit_kernel
 *
 * @brief The actual context switching code begins here.
 *
 * This function is called by the kernel. Upon entry, we are using
 * the kernel stack, on top of which is the address of the instruction
 * after the call to exit_kernel().
 *
 * Assumption: Our kernel is executed with interrupts already disabled.
 *
 * The "naked" attribute prevents the compiler from adding instructions
 * to save and restore register values. It also prevents an
 * automatic return instruction.
 */
void exit_kernel(void)
{
    /*
     * The PC was pushed on the stack with the call to this function.
     * Now push on the I/O registers and the SREG as well.
     */
     SAVE_CTX();

    /*
     * The last piece of the context is the SP. Save it to a variable.
     */
    kernel_sp = SP;

    /*
     * Now restore the task's context, SP first.
     */
    SP = (uint16_t)(cur_task->sp);

    /*
     * Now restore I/O and SREG registers.
     */
    RESTORE_CTX();

    /*
     * return explicitly required as we are "naked".
     * Interrupts are enabled or disabled according to SREG
     * recovered from stack, so we don't want to explicitly
     * enable them here.
     *
     * The last piece of the context, the PC, is popped off the stack
     * with the ret instruction.
     */
    asm volatile ("ret\n"::);
}


/**
 * @fn enter_kernel
 *
 * @brief All system calls eventually enter here.
 *
 * Assumption: We are still executing on cur_task's stack.
 * The return address of the caller of enter_kernel() is on the
 * top of the stack.
 */
void enter_kernel(void)
{
    /*
     * The PC was pushed on the stack with the call to this function.
     * Now push on the I/O registers and the SREG as well.
     */
    SAVE_CTX();

    /*
     * The last piece of the context is the SP. Save it to a variable.
     */
    cur_task->sp = (uint8_t*)SP;

    /*
     * Now restore the kernel's context, SP first.
     */
    SP = kernel_sp;

    /*
     * Now restore I/O and SREG registers.
     */
    RESTORE_CTX();

    /*
     * return explicitly required as we are "naked".
     *
     * The last piece of the context, the PC, is popped off the stack
     * with the ret instruction.
     */
    asm volatile ("ret\n"::);
}


/**
 * @fn TIMER1_COMPA_vect
 *
 * @brief The interrupt handler for output compare interrupts on Timer 1
 *
 * Used to enter the kernel when a tick expires.
 *
 * Assumption: We are still executing on the cur_task stack.
 * The return address inside the current task code is on the top of the stack.
 *
 * The "naked" attribute prevents the compiler from adding instructions
 * to save and restore register values. It also prevents an
 * automatic return instruction.
 */
void TIMER1_COMPA_vect(void)
{
	//PORTB ^= _BV(PB7);		// Arduino LED
    /*
     * Save the interrupted task's context on its stack,
     * and save the stack pointer.
     *
     * On the cur_task's stack, the registers and SREG are
     * saved in the right order, but we have to modify the stored value
     * of SREG. We know it should have interrupts enabled because this
     * ISR was able to execute, but it has interrupts disabled because
     * it was stored while this ISR was executing. So we set the bit (I = bit 7)
     * in the stored value.
     */
    SAVE_CTX_TOP();

    STACK_SREG_SET_I_BIT();

    SAVE_CTX_BOTTOM();

    cur_task->sp = (uint8_t*)SP;

    /*
     * Now that we already saved a copy of the stack pointer
     * for every context including the kernel, we can move to
     * the kernel stack and use it. We will restore it again later.
     */
    SP = kernel_sp;

    /*
     * Inform the kernel that this task was interrupted.
     */
    kernel_request = TIMER_EXPIRED;

    /*
     * Prepare for next tick interrupt.
     */
    OCR1A += TICK_CYCLES;

    /*
     * Restore the kernel context. (The stack pointer is restored again.)
     */
    SP = kernel_sp;

    /*
     * Now restore I/O and SREG registers.
     */
    RESTORE_CTX();

    /*
     * We use "ret" here, not "reti", because we do not want to
     * enable interrupts inside the kernel.
     * Explicitly required as we are "naked".
     *
     * The last piece of the context, the PC, is popped off the stack
     * with the ret instruction.
     */
    asm volatile ("ret\n"::);
}


/*
 * Tasks Functions
 */
/**
 *  @brief Kernel function to create a new task.
 *
 * When creating a new task, it is important to initialize its stack just like
 * it has called "enter_kernel()"; so that when we switch to it later, we
 * can just restore its execution context on its stack.
 * @sa enter_kernel
 */
int kernel_create_task()
{
    /* The new task. */
    task_descriptor_t *p;
    uint8_t* stack_bottom;

    if (dead_pool_queue.head == NULL)
    {
        /* Too many tasks! */
        return 0;
    }

	/* idling "task" goes in last descriptor. */
	if(kernel_request_create_args.level == NULL)
	{
		p = &task_desc[MAXPROCESS];
	}
	/* Find an unused descriptor. */
	else
	{
	    p = dequeue(&dead_pool_queue);
	}

    stack_bottom = &(p->stack[MAXSTACK-1]);

    /* The stack grows down in memory, so the stack pointer is going to end up
     * pointing to the location 32 + 1 + 1 + 3 + 3 = 40 bytes above the bottom, to make
     * room for (from bottom to top):
     *   the address of Task_Terminate() to destroy the task if it ever returns,
     *   the address of the start of the task to "return" to the first time it runs,
     *   register 31,
     *   the stored SREG
	 *   the stored EIND, and *** I just added this; it's the second + 1 below.
     *   registers 30 to 0.
     */
    uint8_t* stack_top = stack_bottom - (32 + 1 + 1 + 3 + 3);

    /* Not necessary to clear the task descriptor. */
    /* memset(p,0,sizeof(task_descriptor_t)); */

    /* stack_top[0] is the byte above the stack.
     * stack_top[1] is r0. */
    stack_top[2] = (uint8_t) 0; /* r1 is the "zero" register. */
    /* stack_top[31] is r30. */
    stack_top[32] = (uint8_t) _BV(SREG_I); /* set SREG_I bit in stored SREG. */
    /* stack_top[33] is r31. */

    /* We are placing the address (16-bit) of the functions
     * onto the stack in reverse byte order (least significant first, followed
     * by most significant).  This is because the "return" assembly instructions
     * (ret and reti) pop addresses off in BIG ENDIAN (most sig. first, least sig.
     * second), even though the AT90 is LITTLE ENDIAN machine.
     */
	stack_top[35] = 0; // FIXME: this should probably be something more than just zero
    stack_top[36] = (uint8_t)((uint16_t)(kernel_request_create_args.f) >> 8);
    stack_top[37] = (uint8_t)(uint16_t)(kernel_request_create_args.f);
	stack_top[38] = 0; // FIXME: this should probably be something more than just zero
    stack_top[39] = (uint8_t)((uint16_t)Task_Terminate >> 8);
    stack_top[40] = (uint8_t)(uint16_t)Task_Terminate;

    /*
     * Make stack pointer point to cell above stack (the top).
     * Make room for 32 registers, SREG and two return addresses.
     */
    p->sp = stack_top;

    p->state = READY;
    p->arg = kernel_request_create_args.arg;
    p->level = kernel_request_create_args.level;

	switch(kernel_request_create_args.level)
	{
	case PERIODIC:
		/* Put this newly created periodic task into the periodic scheduler */
        Scheduler_StartTask(p, kernel_request_create_args.start, kernel_request_create_args.period, kernel_request_create_args.wcet, kernel_request_create_args.f);
		break;

    case SYSTEM:
    	/* Put SYSTEM and Round Robin tasks on a queue. */
        enqueue(&system_queue, p);
		break;

    case RR:
		/* Put SYSTEM and Round Robin tasks on a queue. */
        enqueue(&rr_queue, p);
		break;

	default:
		/* idle task does not go in a queue */
		break;
	}


    return 1;
}


/**
 * @brief Kernel function to destroy the current task.
 */
void kernel_terminate_task(void)
{
    /* deallocate all resources used by this task */
    cur_task->state = DEAD;
    if(cur_task->level == PERIODIC)
    {
        Scheduler_StopTask(cur_task);
    }
    enqueue(&dead_pool_queue, cur_task);
}

/*
 * Queue manipulation.
 */

/**
 * @brief Add a task the head of the queue
 *
 * @param queue_ptr the queue to insert in
 * @param task_to_add the task descriptor to add
 */
void enqueue(queue_t* queue_ptr, task_descriptor_t* task_to_add)
{
    task_to_add->next = NULL;

    if(queue_ptr->head == NULL)
    {
        /* empty queue */
        queue_ptr->head = task_to_add;
        queue_ptr->tail = task_to_add;
    }
    else
    {
        /* put task at the back of the queue */
        queue_ptr->tail->next = task_to_add;
        queue_ptr->tail = task_to_add;
    }
}


/**
 * @brief Pops head of queue and returns it.
 *
 * @param queue_ptr the queue to pop
 * @return the popped task descriptor
 */
task_descriptor_t* dequeue(queue_t* queue_ptr)
{
    task_descriptor_t* task_ptr = queue_ptr->head;

    if(queue_ptr->head != NULL)
    {
        queue_ptr->head = queue_ptr->head->next;
        task_ptr->next = NULL;
    }

    return task_ptr;
}

void initialize_dead_pool_queue() {
	int i;
	for (i = 0; i < MAXPROCESS - 1; i++)
	{
		task_desc[i].state = DEAD;
		task_desc[i].next = &task_desc[i + 1];
	}
	task_desc[MAXPROCESS - 1].next = NULL;
	dead_pool_queue.head = &task_desc[0];
	dead_pool_queue.tail = &task_desc[MAXPROCESS - 1];
}


/**
 * @brief Update the current time.
 *
 * Perhaps move to the next time slot of the PPP.
 */
void kernel_update_ticker(void)
{
    /* PORTD ^= LED_D5_RED; */
	if(periodic_task->iteration_complete == 0) {
		// I think we can set ticks_remaining when we start a periodic task -- using the wcet value.
		--ticks_remaining;

		if(ticks_remaining == 0)
		{
			/* If Periodic task still running then error */
			if(cur_task != NULL && cur_task->level == PERIODIC && periodic_task->iteration_complete == 0)
			{
				/* error handling */
				error_msg = ERR_RUN_3_PERIODIC_TOOK_TOO_LONG;
				OS_Abort();
			}
		}
	}
}

#undef SLOW_CLOCK

#ifdef SLOW_CLOCK
/**
 * @brief For DEBUGGING to make the clock run slower
 *
 * Divide CLKI/O by 64 on timer 1 to run at 125 kHz  CS3[210] = 011
 * 1 MHz CS3[210] = 010
 */
void kernel_slow_clock(void)
{
    TCCR1B &= ~(_BV(CS12) | _BV(CS10));
    TCCR1B |= (_BV(CS11));
}
#endif

/**
 *  @brief Delay function adapted from <util/delay.h>
 */
void _delay_25ms(void)
{
    //uint16_t i;

    /* 4 * 50000 CPU cycles = 25 ms */
    //asm volatile ("1: sbiw %0,1" "\n\tbrne 1b" : "=w" (i) : "0" (50000));
    _delay_ms(25);
}
