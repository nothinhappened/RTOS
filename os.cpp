/**
 * @file os.cpp
 *
 * @brief A Real Time Operating System
 *
 * Our implementation of the operating system described by Mantis Cheng in os.h.
 *
 * Original authors:
 * @author Scott Craig
 * @author Justin Tanner
 * 
 * Modifications (2015) by:
 * @author Fraser DeLisle
 * @author Logan Bissonnette
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "kernel.h"
#include "os.h"
#include "oskernel.h"
#include "periodic_scheduler.h"
#include "tasks/tasks.h"

/** @brief main function provided by user application. The first task to run. */
extern int r_main();

struct service {
	int16_t data;
	queue_t list_of_tasks_who_are_subscribed;
};

static SERVICE service_array[255];

/**
 * @brief Setup the RTOS and create main() as the first SYSTEM level task.
 *
 * Point of entry from the C runtime crt0.S.
 */
void OS_Init()
{
    /* Set up the clocks */

    TCCR1B |= (_BV(CS11));
	TCCR5A = 0; // timer5 in normal mode.
	TCCR5B |= (_BV(CS51));

#ifdef SLOW_CLOCK
    kernel_slow_clock();
#endif

    /*
     * Initialize dead pool to contain all but last task descriptor.
     *
     * DEAD == 0, already set in .init4
     */
	initialize_dead_pool_queue();

	/* Create idle "task" */
    kernel_request_create_args.f = (voidfuncvoid_ptr)idle;
    kernel_request_create_args.level = NULL;
    kernel_create_task();

    /* Create "main" task as SYSTEM level. */
    kernel_request_create_args.f = (voidfuncvoid_ptr)r_main;
    kernel_request_create_args.level = SYSTEM;
    kernel_create_task();

    /* First time through. Select "main" task to run first. */
    cur_task = task_desc;
    cur_task->state = RUNNING;
    dequeue(&system_queue);

    /* Set up Timer 1 Output Compare interrupt,the TICK clock. */
    TIMSK1 |= _BV(OCIE1A);
	volatile uint32_t time = TCNT1;
	volatile uint32_t tickstest = TICK_CYCLES;
	volatile uint32_t test = time + tickstest;
	OCR1A = test;
    OCR1A = TCNT1 + TICK_CYCLES;
    /* Clear flag. */
    TIFR1 = _BV(OCF1A);
	
	
	/* Set up the periodic task scheduler */
	Scheduler_Init();
	
    /*
     * The main loop of the RTOS kernel.
     */
    kernel_main_loop();
}


/** @brief Abort the execution of this RTOS due to an unrecoverable error.
 */
void OS_Abort(void)
{
    uint8_t i, j;
    uint8_t flashes, mask;

    Disable_Interrupt();

    /* Initialize port for output */
    DDRD = LED_RED_MASK | LED_GREEN_MASK;

    if(error_msg < ERR_RUN_1_USER_CALLED_OS_ABORT)
    {
        flashes = error_msg + 1;
        mask = LED_GREEN_MASK;
    }
    else
    {
        flashes = error_msg + 1 - ERR_RUN_1_USER_CALLED_OS_ABORT;
        mask = LED_RED_MASK;
    }

    for(;;)
    {
        PORTD = (uint8_t)(LED_RED_MASK | LED_GREEN_MASK);

        for(i = 0; i < 100; ++i)
        {
               _delay_25ms();
        }

        PORTD = (uint8_t) 0;

        for(i = 0; i < 40; ++i)
        {
               _delay_25ms();
        }


        for(j = 0; j < flashes; ++j)
        {
            PORTD = mask;

            for(i = 0; i < 10; ++i)
            {
                _delay_25ms();
            }

            PORTD = (uint8_t) 0;

            for(i = 0; i < 10; ++i)
            {
                _delay_25ms();
            }
        }

        for(i = 0; i < 20; ++i)
        {
            _delay_25ms();
        }
    }
}

 /**
   * \param f a parameterless function to be created as a process instance
   * \param arg an integer argument to be assigned to this process instance
   * \param period its execution period in TICKs
   * \param wcet its worst-case execution time in TICKs, must be less than "period"
   * \param start its start time in TICKs
   * \return 0 if not successful; otherwise non-zero.
   * \sa Task_GetArg()
   *
   *  A new process is created to execute the parameterless
   *  function \a f with an initial parameter \a arg, which is retrieved
   *  by a call to Task_GetArg().  If a new process cannot be
   *  created, 0 is returned; otherwise, it returns non-zero.
   *
   * \sa \ref policy
   */
int8_t Task_Create_Periodic(void(*f)(void), int16_t arg, uint16_t period, uint16_t wcet, uint16_t start) {
	    int retval;
	    uint8_t sreg;

	    sreg = SREG;
	    Disable_Interrupt();

	    kernel_request_create_args.f = (voidfuncvoid_ptr)f;
	    kernel_request_create_args.arg = arg;
	    kernel_request_create_args.level = (uint8_t)PERIODIC;
        kernel_request_create_args.period = period;
        kernel_request_create_args.wcet = wcet;
        kernel_request_create_args.start = start;

	    kernel_request = TASK_CREATE;
	    enter_kernel();

	    retval = kernel_request_retval;
	    SREG = sreg;

	    return retval;
}

int8_t Task_Create_System(void (*f)(void), int16_t arg) {
	    int retval;
	    uint8_t sreg;

	    sreg = SREG;
	    Disable_Interrupt();

	    kernel_request_create_args.f = (voidfuncvoid_ptr)f;
	    kernel_request_create_args.arg = arg;
	    kernel_request_create_args.level = SYSTEM;

	    kernel_request = TASK_CREATE;
	    enter_kernel();

	    retval = kernel_request_retval;
	    SREG = sreg;

	    return retval;
}

int8_t Task_Create_RR(void (*f)(void), int16_t arg) {
	int retval;
	uint8_t sreg;

	sreg = SREG;
	Disable_Interrupt();

	kernel_request_create_args.f = (voidfuncvoid_ptr)f;
	kernel_request_create_args.arg = arg;
	kernel_request_create_args.level = RR;

	kernel_request = TASK_CREATE;
	enter_kernel();

	retval = kernel_request_retval;
	SREG = sreg;

	return retval;
}


/**
  * @brief The calling task gives up its share of the processor voluntarily.
  */
void Task_Next()
{
    uint8_t volatile sreg;

    sreg = SREG;
    Disable_Interrupt();

    kernel_request = TASK_NEXT;
    enter_kernel();

    SREG = sreg;
}


/**
  * @brief The calling task terminates itself.
  */
void Task_Terminate()
{
    uint8_t sreg;

    sreg = SREG;
    Disable_Interrupt();

    kernel_request = TASK_TERMINATE;
    enter_kernel();

    SREG = sreg;
}


/** @brief Retrieve the assigned parameter.
 */
int Task_GetArg(void)
{
    int arg;
    uint8_t sreg;

    sreg = SREG;
    Disable_Interrupt();

    arg = cur_task->arg;

    SREG = sreg;

    return arg;
}

/**
 * \return a non-NULL SERVICE descriptor if successful; NULL otherwise.
 *
 *  Initialize a new, non-NULL SERVICE descriptor.
 */
SERVICE *Service_Init() {
	static uint8_t next_service = 0;
	return &service_array[next_service++];
}

/**  
  * \param s an Service descriptor
  * \param v pointer to memory where the received value will be written
  *
  * The calling task waits for the next published value associated with service "s".
  * More than one task may wait for a service. When a new value "v" is published to
  * "s", all waiting tasks resume and obtain the same value. 
  */
void Service_Subscribe( SERVICE *s, int16_t *v ) {
	if(cur_task->level != PERIODIC) {
		enqueue(&s->list_of_tasks_who_are_subscribed, cur_task);
		cur_task->state = WAITING;
		Task_Next();
		cur_task->state = RUNNING;
		*v = s->data;
	} else {
		error_msg = ERR_RUN_7_PERIODIC_ATTEMPTED_SUBSCRIBE;
		OS_Abort();
	}
}


/**  
  * \param e a Service descriptor
  *
  * The calling task publishes a new value "v" to service "s". All waiting tasks on
  * service "s" will be resumed and receive a copy of this value "v". 
  * Values generated by services without subscribers will be lost.
  */
void Service_Publish( SERVICE *s, int16_t v ) {
	s->data = v;
	while (s->list_of_tasks_who_are_subscribed.head != NULL) {
		enqueue(&service_queue, dequeue(&s->list_of_tasks_who_are_subscribed));
	}
}

   
  /*=====  System Clock API ===== */
  
/**  
  * Returns the number of milliseconds since OS_Init(). Note that this number
  * wraps around after it overflows as an unsigned integer. The arithmetic
  * of 2's complement will take care of this wrap-around behaviour if you use
  * this number correctly.
  * Let  T = Now() and we want to know when Now() reaches T+1000.
  * Now() is always increasing. Even if Now() wraps around, (Now() - T) always
  * >= 0. As long as the duration of interest is less than the wrap-around time,
  * then (Now() - T >= 1000) would mean we have reached T+1000.
  * However, we cannot compare Now() against T directly due to this wrap-around
  * behaviour.
  * Now() will wrap around every 65536 milliseconds. Therefore, for measurement
  * purposes, it should be used for durations less than 65 seconds.
  */
uint16_t Now() { // number of milliseconds since the RTOS boots.
	static uint16_t millies = 0;
	millies += TCNT5 / ((F_CPU / TIMER_PRESCALER) / 1000);
	return millies;
}


/**
 * Runtime entry point into the program; just start the RTOS.  The application layer must define r_main() for its entry point.
 */
int main()
{
	OS_Init();
	return 0;
}
