#include "FreeRTOS.h"
#include "task.h"
#include "mbed.h"
#include "scheduler.h"
#include "scheduler_logic.h"
#include "utils.h"
#include "slack.h"

#define AP_MAX_DELAY 6

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
void vSchedulerNegativeSlackHook( TickType_t xTickCount, BaseType_t xSlack );
void vSchedulerDeadlineMissHook( struct TaskInfo * xTask, const TickType_t xTickCount );
void vSchedulerWcetOverrunHook( struct TaskInfo * xTask, const TickType_t xTickCount );

void vSchedulerStartHook( void );

#if defined (__cplusplus)
}
#endif

static void task_body( void* params );
static void aperiodic_task_body( void* params );
static void printTask( const int start, const char* pcTaskName, const struct TaskInfo * taskInfo );

/* Create a Serial port, connected to the USBTX/USBRX pins; they represent the
 * pins that route to the interface USB Serial port so you can communicate with
 * a host PC. */
static RawSerial pc( USBTX, USBRX );

struct TaskInfo * pxAperiodicTask;

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
		printTask( 0, pcTaskName, taskInfo );

		vUtilsEatCpu( taskInfo->xWcet - 10 );

		printTask( 1, pcTaskName, taskInfo );

		vSchedulerWaitForNextPeriod();
	}

	// If the tasks ever leaves the for cycle, kill it.
	vTaskDelete( NULL );
}

static void aperiodic_task_body( void* params )
{
	struct TaskInfo *pxTaskInfo = ( struct TaskInfo * ) params;

	TickType_t xRandomDelay;

	vTaskDelay( ( ( rand() % AP_MAX_DELAY ) ) * 1000 );

	for (;;)
	{
		printTask( 0, "A01", pxTaskInfo );

		vUtilsEatCpu( 1500 );

		/* Calculate random delay */
		xRandomDelay = ( ( rand() % AP_MAX_DELAY ) + 3 ) * 1000;

		printTask( 1, "A01", pxTaskInfo );

		/* The HST scheduler will execute the task if there is enough slack available. */
		vTaskDelay( xRandomDelay );
	}
}

static void printTask( const int start, const char* pcTaskName, const struct TaskInfo * taskInfo )
{
	ListItem_t * pxAppTasksListItem;

	vTaskSuspendAll();

	pc.printf( "%d\t%s\t%s\t%d\t%d\t%d\t", xTaskGetTickCount(), pcTaskName, ( start == 0 ? "S" : "E"), taskInfo->uxReleaseCount, taskInfo->xCur, xAvailableSlack );

	pxAppTasksListItem = listGET_HEAD_ENTRY( pxAllTasksList );

	while( listGET_END_MARKER( pxAllTasksList ) != pxAppTasksListItem )
	{
		struct TaskInfo_Slack * s = ( struct TaskInfo_Slack * ) ( ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem ) )->vExt;
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

void vSchedulerDeadlineMissHook( struct TaskInfo * xTask, const TickType_t xTickCount )
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

void vSchedulerWcetOverrunHook( struct TaskInfo * xTask, const TickType_t xTickCount )
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
	pc.printf("Rate Monotonic + Slack Stealing\nNow, shall we begin? :-) \n");
	pc.printf( "\nSetup -- %d\t -- \t%d\t", xTaskGetTickCount(), xAvailableSlack );

	const ListItem_t * pxAppTasksListEndMarker = listGET_END_MARKER( pxAllTasksList );
	ListItem_t * pxAppTasksListItem = listGET_HEAD_ENTRY( pxAllTasksList );

	// Print slack values for the critical instant.
	while( pxAppTasksListEndMarker != pxAppTasksListItem )
	{
		struct TaskInfo_Slack * s = ( struct TaskInfo_Slack * ) ( ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem ) )->vExt;
		pc.printf( "%d\t" , s->xSlack );
		pxAppTasksListItem = listGET_NEXT( pxAppTasksListItem );
	}

	pc.printf("\n");
}
#endif
