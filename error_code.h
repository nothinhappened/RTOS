/**
 * @file   error_code.h
 *
 * @brief Error messages returned in OS_Abort().
 *        Green errors are initialization errors
 *        Red errors are run time errors
 *
 * CSC 460/560 Real Time Operating Systems - Mantis Cheng
 *
 * @author Scott Craig
 * @author Justin Tanner
 */
#ifndef __ERROR_CODE_H__
#define __ERROR_CODE_H__

enum {

/** GREEN ERRORS -- Initialize time errors. */

/** RED ERRORS -- Run time errors. */

/** User called OS_Abort() */
ERR_RUN_1_USER_CALLED_OS_ABORT,

/** Too many tasks created. Only allowed MAXPROCESS at any time.*/
ERR_RUN_2_TOO_MANY_TASKS,

/** PERIODIC task still running at end of time slot. */
ERR_RUN_3_PERIODIC_TOOK_TOO_LONG,

/** ISR made a request that only tasks are allowed. */
ERR_RUN_4_ILLEGAL_ISR_KERNEL_REQUEST,

/** RTOS Internal error in handling request. */
ERR_RUN_5_RTOS_INTERNAL_ERROR,

/** PERIODIC task overlapping another periodic task */
ERR_RUN_6_PERIODIC_SCHEDULE_INVALID_SETUP,

/** PERIODIC task overlapping another periodic task */
ERR_RUN_7_PERIODIC_ATTEMPTED_SUBSCRIBE,


ERR_RUN_8_RTOS_INTERNAL_ERROR

};


#endif
