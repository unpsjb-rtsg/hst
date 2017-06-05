#include "FreeRTOS.h"
#include "task.h"
#include "mbed.h"
#include "scheduler.h"
#include "scheduler_logic.h"
#include "utils.h"
#include "semphr.h"

#define AP_MAX_DELAY 6

/* The extern "C" is required to avoid name mangling between C and C++ code. */
extern "C"
{
// FreeRTOS callback/hook functions
void vApplicationIdleHook( void );
void vApplicationMallocFailedHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );

// HST callback/hook functions
void vSchedulerDeadlineMissHook( HstTCB_t * xTask, const TickType_t xTickCount );
void vSchedulerWcetOverrunHook( HstTCB_t * xTask, const TickType_t xTickCount );
void vSchedulerStartHook( void );
}

static void task_body( void* params );
static void aperiodic_task_body( void* params );

/* Create a Serial port, connected to the USBTX/USBRX pins; they represent the
 * pins that route to the interface USB Serial port so you can communicate with
 * a host PC. */
static Serial pc( USBTX, USBRX );

DigitalOut led1(LED1);

int main() {
	/* Set the baud rate of the serial port. */
	pc.baud( 9600 );

	vSchedulerSetup();

	/* Create the application scheduled tasks. */
	xSchedulerTaskCreate( task_body, "T01", 256, NULL, 0, NULL, 3000, 3000, 1000 );
	xSchedulerTaskCreate( task_body, "T02", 256, NULL, 1, NULL, 4000, 4000, 1000 );
	xSchedulerTaskCreate( task_body, "T03", 256, NULL, 2, NULL, 6000, 6000, 1000 );
	xSchedulerTaskCreate( task_body, "T04", 256, NULL, 3, NULL, 12000, 12000, 1000 );

    /* Aperiodic task */
    xSchedulerAperiodicTaskCreate( aperiodic_task_body, "T1A", 256, NULL, NULL );
    xSchedulerAperiodicTaskCreate( aperiodic_task_body, "T2A", 256, NULL, NULL );

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

	struct TaskInfo_DP * dp;

	for (;;)
	{
		dp = ( struct TaskInfo_DP * ) taskInfo->vExt;
		vTaskSuspendAll();
        pc.printf( "%d\t\t%s\tSTART\t\t%d\t%d\t%s\n", xTaskGetTickCount(), pcTaskName, taskInfo->uxReleaseCount, taskInfo->xCur, dp->xInUpperBand ? "H" : "L" );
		xTaskResumeAll();

		vUtilsEatCpu( taskInfo->xWcet );

		vTaskSuspendAll();
		pc.printf( "%d\t\t%s\tEND  \t\t%d\t%d\t%s\n", xTaskGetTickCount(), pcTaskName, taskInfo->uxReleaseCount, taskInfo->xCur, dp->xInUpperBand ? "H" : "L" );
		xTaskResumeAll();

		vSchedulerWaitForNextPeriod();
	}

	/* If the tasks ever leaves the for cycle, kill it. */
	vTaskDelete( NULL );
}

static void aperiodic_task_body( void* params )
{
	HstTCB_t *pxTaskInfo = ( HstTCB_t * ) params;

	TickType_t xRandomDelay;

	// A pointer to the task's name, standard NULL terminated C string.
	char *pcTaskName = pcTaskGetTaskName( NULL );

	vTaskDelay( ( ( rand() % AP_MAX_DELAY ) ) * 1000 );

	for (;;)
	{
		vTaskSuspendAll();
		pc.printf( "%d\t\t%s\tSTART\t\t%d\t%d\tM\n", xTaskGetTickCount(), pcTaskName, pxTaskInfo->uxReleaseCount, pxTaskInfo->xCur );
		xTaskResumeAll();

		vUtilsEatCpu( 1000 );

		/* Calculate random delay */
		xRandomDelay = ( ( rand() % AP_MAX_DELAY ) + 3 ) * 1000;

		vTaskSuspendAll();
		pc.printf( "%d\t\t%s\tEND  \t\t%d\t%d\tM\n", xTaskGetTickCount(), pcTaskName, pxTaskInfo->uxReleaseCount, pxTaskInfo->xCur );
		xTaskResumeAll();

		/* The HST scheduler will execute the task if there is enough slack available. */
		vTaskDelay( xRandomDelay );
	}
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
	pc.printf( "Dual Priority\n" );
	pc.printf( "Setup -- %d --", xTaskGetTickCount() );

	ListItem_t * pxAppTasksListItem = listGET_HEAD_ENTRY( pxAllTasksList );

	// Print slack values for the critical instant.
	while( listGET_END_MARKER( pxAllTasksList ) != pxAppTasksListItem )
	{
		struct TaskInfo_DP * dp = ( struct TaskInfo_DP * ) ( ( HstTCB_t * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem ) )->vExt;
		if( dp != NULL )
		{
			pc.printf( "%d\t" , dp->xPromotion );
		}
		pxAppTasksListItem = listGET_NEXT( pxAppTasksListItem );
	}

	pc.printf("\n");
}
#endif
