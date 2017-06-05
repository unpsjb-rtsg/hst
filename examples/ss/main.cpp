#include "FreeRTOS.h"
#include "task.h"
#include "mbed.h"
#include "scheduler.h"
#include "scheduler_logic.h"
#include "utils.h"
#include "slack.h"

#define AP_MAX_DELAY 6
#define START_TASK   "S"
#define END_TASK     "E"

/* The extern "C" is required to avoid name mangling between C and C++ code. */
extern "C"
{
// FreeRTOS callback/hook functions
void vApplicationMallocFailedHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );

// HST callback/hook functions
void vSchedulerNegativeSlackHook( TickType_t xTickCount, BaseType_t xSlack );
void vSchedulerDeadlineMissHook( HstTCB_t * xTask, const TickType_t xTickCount );
void vSchedulerWcetOverrunHook( HstTCB_t * xTask, const TickType_t xTickCount );
void vSchedulerStartHook( void );
}

static void task_body( void* params );
static void aperiodic_task_body( void* params );
static void printTask( const char* start, const char* pcTaskName, const HstTCB_t * taskInfo );

/* Create a Serial port, connected to the USBTX/USBRX pins; they represent the
 * pins that route to the interface USB Serial port so you can communicate with
 * a host PC. */
static RawSerial pc( USBTX, USBRX );

static HstTCB_t *pxAperiodicTask;

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
    xSchedulerAperiodicTaskCreate( aperiodic_task_body, "TA1", 256, NULL, &pxAperiodicTask );

	/* Create and start the scheduler task. */
	vSchedulerInit();

	/* The execution should never reach here. */
	for(;;);
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
		printTask( START_TASK, pcTaskName, taskInfo );

		vUtilsEatCpu( taskInfo->xWcet );

		printTask( END_TASK, pcTaskName, taskInfo );

		vSchedulerWaitForNextPeriod();
	}

	// If the tasks ever leaves the for cycle, kill it.
	vTaskDelete( NULL );
}

static void aperiodic_task_body( void* params )
{
	HstTCB_t *pxTaskInfo = ( HstTCB_t * ) params;

	TickType_t xRandomDelay;

	vTaskDelay( ( ( rand() % AP_MAX_DELAY ) ) * 1000 );

	for (;;)
	{
		printTask( START_TASK, "A01", pxTaskInfo );

		vUtilsEatCpu( 1500 );

		/* Calculate random delay */
		xRandomDelay = ( ( rand() % AP_MAX_DELAY ) + 3 ) * 1000;

		printTask( END_TASK, "A01", pxTaskInfo );

		/* The HST scheduler will execute the task if there is enough slack available. */
		vTaskDelay( xRandomDelay );
	}

	// If the tasks ever leaves the for cycle, kill it.
	vTaskDelete( NULL );
}

static void printTask( const char* start, const char* pcTaskName, const HstTCB_t * taskInfo )
{
	ListItem_t * pxAppTasksListItem;

	vTaskSuspendAll();

	pc.printf( "%d\t%s\t%s\t%d\t%d\t%d\t", xTaskGetTickCount(), pcTaskName, start, taskInfo->uxReleaseCount, taskInfo->xCur, xAvailableSlack );

	pxAppTasksListItem = listGET_HEAD_ENTRY( pxAllTasksList );

	while( listGET_END_MARKER( pxAllTasksList ) != pxAppTasksListItem )
	{
		struct TaskInfo_Slack * s = ( struct TaskInfo_Slack * ) ( ( HstTCB_t * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem ) )->vExt;
		pc.printf( "%d\t" , s->xSlack );
		pxAppTasksListItem = listGET_NEXT( pxAppTasksListItem );
	}
	pc.printf( "\n\r" );

	xTaskResumeAll();
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

void vSchedulerDeadlineMissHook( HstTCB_t * xTask, const TickType_t xTickCount )
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

void vSchedulerNegativeSlackHook( TickType_t xTickCount, BaseType_t xSlack )
{
	taskDISABLE_INTERRUPTS();

	pc.printf( "Negative slack: %d - %d\n", xTickCount, xSlack );

	DigitalOut led( LED4 );

	for( ;; )
	{
		led = 1;
		wait_ms(1000);
		led = 0;
		wait_ms(1000);
	}
}

void vSchedulerWcetOverrunHook( HstTCB_t * xTask, const TickType_t xTickCount )
{
	taskDISABLE_INTERRUPTS();

	pc.printf( "Task %s overrun its wcet: %d - %d\n", pcTaskGetTaskName( xTask->xHandle ), xTask->xCur, xTask->xWcet );

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
	pc.printf("Rate Monotonic + Slack Stealing\n");
	pc.printf( "\nSetup -- %d\t -- \t%d\t", xTaskGetTickCount(), xAvailableSlack );

	ListItem_t * pxAppTasksListItem = listGET_HEAD_ENTRY( pxAllTasksList );

	// Print slack values for the critical instant.
	while( listGET_END_MARKER( pxAllTasksList ) != pxAppTasksListItem )
	{
		struct TaskInfo_Slack * s = ( struct TaskInfo_Slack * ) ( ( HstTCB_t * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem ) )->vExt;
		pc.printf( "%d\t" , s->xSlack );
		pxAppTasksListItem = listGET_NEXT( pxAppTasksListItem );
	}

	pc.printf("\n");
}
#endif
