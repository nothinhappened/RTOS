/* everything we need for scheduling PERIODIC tasks */
#ifndef _PERIODIC_SCHEDULER_H_
#define _PERIODIC_SCHEDULER_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "kernel.h"

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

typedef struct
{
    uint16_t period;
    int16_t remaining_time;
	uint16_t wcet;
    uint8_t is_running;
	uint8_t iteration_complete;
    voidfuncvoid_ptr callback;
    task_descriptor_t* descriptor;
} task_t;

extern task_t EmptyStruct;


void Scheduler_Init();

void Scheduler_StartTask(task_descriptor_t* descriptor, uint16_t delay, uint16_t period, uint16_t wcet, voidfuncvoid_ptr task);
void Scheduler_StopTask(task_descriptor_t* task);

uint16_t Scheduler_Dispatch();

task_t* Scheduler_GetTask();

#ifdef __cplusplus
}
#endif

#endif /* _PERIODIC_SCHEDULER_H_ */
