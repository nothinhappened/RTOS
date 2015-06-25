/* everything we need for scheduling PERIODIC tasks */

#include "error_code.h"
#include "periodic_scheduler.h"
#include <math.h>

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

task_t EmptyStruct = {0, 0, 0, 0, 1, NULL, NULL};

task_t periodic_tasks[MAXPROCESS];

static uint16_t last_runtime;

static task_t* ready_task;

void Scheduler_Init()
{
    last_runtime = Now();
}

void Scheduler_StartTask(task_descriptor_t* descriptor, uint16_t delay, uint16_t period, uint16_t wcet, voidfuncvoid_ptr task)
{
    static uint8_t id = 0;
    if (id < MAXPROCESS)
    {
        periodic_tasks[id].remaining_time = delay;
        periodic_tasks[id].period = period;
		periodic_tasks[id].wcet = wcet;
        periodic_tasks[id].is_running = 1;
		periodic_tasks[id].iteration_complete = 0;
        periodic_tasks[id].callback = task;
        periodic_tasks[id].descriptor = descriptor;
        id++;
    }
}

void Scheduler_StopTask(task_descriptor_t* task) {
    // FIXME: implement this function
	// can probably set the periodic task's is_running to 0
}

uint16_t Scheduler_Dispatch()
{
    uint8_t i;
    uint16_t now = Now();
    uint16_t elapsed = now - last_runtime;

    last_runtime = now;
    voidfuncvoid_ptr t = NULL;
    uint16_t idle_time = 0xFFFF;
    
    // update each task's remaining time, and identify the first ready task (if there is one).
    for (i = 0; i < MAXPROCESS; i++)
    {
        if (periodic_tasks[i].is_running)
        {
            // update the task's remaining time
            periodic_tasks[i].remaining_time -= elapsed;
            if (periodic_tasks[i].remaining_time <= 0)
            {
                if (t == NULL)
                {
                    // if this task is ready to run, and we haven't already selected a task to run,
                    // select this one.
                    t = periodic_tasks[i].callback;
                    /* Note this side effect. This ready_task property can be accessed via Scheduler_GetTask() */
                    ready_task = &periodic_tasks[i];
					periodic_tasks[i].iteration_complete = 0; // reset this so that we know that the execution for this task has not completed yet.
                    periodic_tasks[i].remaining_time += periodic_tasks[i].period;
                } else {
					// middle finger
					error_msg = ERR_RUN_6_PERIODIC_SCHEDULE_INVALID_SETUP;
					OS_Abort();
				}
                idle_time = 0;
            }
            else
            {
                idle_time = min((uint16_t)periodic_tasks[i].remaining_time, idle_time);
            }
        }
    }
    //if (t != NULL)
    //{
    //    // If a task was selected to run, call its function.
    //    t();
    //}
    return idle_time;
}

/* If there is a PERIODIC task that is ready to run (indentified by Scheduler_Dispatch())
 * then return the tasks descriptor and CLEAR this ready_task property
 */
task_t* Scheduler_GetTask() {
    task_t* task = ready_task;
    ready_task = &EmptyStruct;
    return task;
}
