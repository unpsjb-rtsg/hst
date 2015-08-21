#include "FreeRTOS.h"
#include "task.h"
#include "mbed.h"
#include "scheduler.h"
#include "scheduler_logic.h"
#include "utils.h"
#include "semphr.h"

/* Prototypes for the standard FreeRTOS callback/hook functions implemented
 * within this file. The extern "C" is required to avoid name mangling
 * between C and C++ code. */
#if defined (__cplusplus)
extern "C" {
#endif

// FreeRTOS callback/hook functions
void vApplicationMallocFailedHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );

// HST callback/hook functions
void vSchedulerDeadlineMissHook( struct TaskInfo * xTask, const TickType_t xTickCount );
void vSchedulerStartHook( void );

#if defined (__cplusplus)
}
#endif

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
    xSchedulerTaskCreate( task_body, "T03", 256, NULL, 2, NULL, 6000, 6000, 2000 );
    xSchedulerTaskCreate( task_body, "T04", 256, NULL, 3, NULL, 12000, 12000, 1000 );

	/* Create and start the scheduler task. */
	vSchedulerInit();

	/* The execution should never reach here. */
}

/**
 * Periodic task body.
 */
static void task_body( void* params )
{
	// eTCB
	struct TaskInfo *taskInfo = ( struct TaskInfo * ) params;

	// A pointer to the task's name, standard NULL terminated C string.
	char *pcTaskName = pcTaskGetTaskName( NULL );

	for (;;)
	{
		taskENTER_CRITICAL();
		pc.printf( "%d\t\t%s\tSTART\t\t%d\t%d\t\n", xTaskGetTickCount(), pcTaskName, taskInfo->uxReleaseCount, taskInfo->xCur );
		taskEXIT_CRITICAL();

		vUtilsEatCpu( taskInfo->xWcet - 100 );

		taskENTER_CRITICAL();
		pc.printf( "%d\t\t%s\tEND  \t\t%d\t%d\t\n", xTaskGetTickCount(), pcTaskName, taskInfo->uxReleaseCount, taskInfo->xCur );
		taskEXIT_CRITICAL();

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

extern void vSchedulerDeadlineMissHook( struct TaskInfo * xTask, const TickType_t xTickCount )
{
	taskDISABLE_INTERRUPTS();

	pc.printf( "Task %s missed its deadline: %d - %d\n", pcTaskGetTaskName( xTask->xHandle ), xTickCount, xTask->xAbsolutDeadline );

	DigitalOut led( LED4 );

	for( ;; )
	{
		led = 1;
		wait_ms(250);
		led = 0;
		wait_ms(250);
	}
}

#if ( configUSE_SCHEDULER_START_HOOK == 1 )
extern void vSchedulerStartHook()
{
	pc.printf("Earliest Deadline First (EDF)\nNow, shall we begin? :-) \n");
}
#endif
