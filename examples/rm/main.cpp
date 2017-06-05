#include "FreeRTOS.h"
#include "task.h"
#include "mbed.h"
#include "scheduler.h"
#include "scheduler_logic.h"
#include "utils.h"
#include "semphr.h"

/* The extern "C" is required to avoid name mangling between C and C++ code. */
extern "C"
{
// FreeRTOS callback/hook functions
void vApplicationMallocFailedHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );

// HST callback/hook functions
void vSchedulerDeadlineMissHook( HstTCB_t * xTask, const TickType_t xTickCount );
void vSchedulerWcetOverrunHook( HstTCB_t * xTask, const TickType_t xTickCount );
void vSchedulerStartHook( void );
}

static void task_body( void* params );

/* Create a Serial port, connected to the USBTX/USBRX pins; they represent the
 * pins that route to the interface USB Serial port so you can communicate with
 * a host PC. */
static Serial pc( USBTX, USBRX );

int main() {
	/* Set the baud rate of the serial port. */
	pc.baud( 9600 );

	vSchedulerSetup();

	/* Create the application scheduled tasks. */
    xSchedulerTaskCreate( task_body, "T01", 256, NULL, 0, NULL, 3000, 3000, 1000 );
    xSchedulerTaskCreate( task_body, "T02", 256, NULL, 1, NULL, 4000, 4000, 1000 );
    xSchedulerTaskCreate( task_body, "T03", 256, NULL, 2, NULL, 6000, 6000, 1000 );
    xSchedulerTaskCreate( task_body, "T04", 256, NULL, 3, NULL, 12000, 12000, 1000 );

	/* Create and start the scheduler task. */
	vSchedulerInit();

	/* The execution should never reach here. */
	for (;;);
}

/**
 * Periodic task body.
 */
static void task_body( void* params )
{
	// eTCB
	HstTCB_t *taskInfo = ( HstTCB_t * ) params;

	// A pointer to the task's name, standard NULL terminated C string.
	char *pcTaskName = pcTaskGetTaskName( NULL );

	for (;;)
	{
		vTaskSuspendAll();
		pc.printf( "%d\t\t%s\tSTART\t\t%d\t%d\t\n", xTaskGetTickCount(), pcTaskName, taskInfo->uxReleaseCount, taskInfo->xCur );
		xTaskResumeAll();

		vUtilsEatCpu( taskInfo->xWcet );

		vTaskSuspendAll();
		pc.printf( "%d\t\t%s\tEND  \t\t%d\t%d\t\n", xTaskGetTickCount(), pcTaskName, taskInfo->uxReleaseCount, taskInfo->xCur );
		xTaskResumeAll();

		vSchedulerWaitForNextPeriod();
	}

	/* If the tasks ever leaves the for cycle, kill it. */
	vTaskDelete( NULL );
}

extern void vApplicationMallocFailedHook( void )
{
	taskDISABLE_INTERRUPTS();

	DigitalOut led( LED2 );

	for( ;; )
	{
        led = 1;
        wait_ms(1000);
        led = 0;
        wait_ms(1000);
	}
}

extern void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	taskDISABLE_INTERRUPTS();

	DigitalOut led( LED3 );

	for( ;; )
	{
        led = 1;
        wait_ms(1000);
        led = 0;
        wait_ms(1000);
	}
}

extern void vSchedulerDeadlineMissHook( HstTCB_t * xTask, const TickType_t xTickCount )
{
	taskDISABLE_INTERRUPTS();

	pc.printf( "Task %s (%d) missed its deadline: %d - %d\n", pcTaskGetTaskName( xTask->xHandle ), xTask->uxReleaseCount, xTickCount, xTask->xAbsolutDeadline );

	DigitalOut led( LED4 );

	for( ;; )
	{
		led = 1;
		wait_ms(250);
		led = 0;
		wait_ms(250);
	}
}

extern void vSchedulerWcetOverrunHook( HstTCB_t * xTask, const TickType_t xTickCount )
{
	taskDISABLE_INTERRUPTS();

	pc.printf( "Task %s (%d) overrun its wcet: %d - %d - %d\n", pcTaskGetTaskName( xTask->xHandle ), xTask->uxReleaseCount, xTask->xCur, xTask->xWcet, xTickCount );

	DigitalOut led( LED4 );

	for( ;; )
	{
		led = 1;
		wait_ms(1000);
		led = 0;
		wait_ms(1000);
	}
}

#if ( configUSE_SCHEDULER_START_HOOK == 1 )
extern void vSchedulerStartHook()
{
	pc.printf("Rate Monotonic Scheduling\n");
}
#endif
