#include "os.h"
#include "kernel.h"
#include "error_code.h"
#include <util/delay.h>

#define THRES 5

void error_out(void) {
	error_msg = ERR_RUN_5_RTOS_INTERNAL_ERROR;
	OS_Abort();
}

void p_task_01() {
	
	while(1) {
		_delay_ms(100);
		Task_Next();
	}
}


int r_main(void) {

	Task_Create_Periodic(&p_task_01, 1, 100, 10, 0);

	return 0;
}
