#include "FreeRTOS.h"
#include "scheduler.h"
#include "scheduler_logic.h"
#include "wcrt.h"
#include "semphr.h"
#include "queue.h"

#if ( configUSE_SCHEDULER_START_HOOK == 1 )
/* Application Hooks. */
extern void vSchedulerStartHook( void );
#endif

extern void vSchedulerWcetOverrunHook( struct TaskInfo * xTask, const TickType_t xTickCount );

/* Callback function called from the FreeRTOS tick interrupt service. */
void vApplicationTickHook( void );

/* HST function. */
static void prvSchedulerTaskScheduler( void * params );

/* Scheduler task handle. */
static TaskHandle_t xSchedulerTask = NULL;

/* Current task. */
static struct TaskInfo * xCurrentTask = NULL;

/**
 * AppSched_Init()
 */
void vSchedulerInit( void )
{
	/* Create the scheduler task. */
	xTaskCreate( prvSchedulerTaskScheduler, NULL, 256, NULL, TASK_SCHEDULER_PRIORITY, &xSchedulerTask );

	/* Calculate the worst case response times for each task. */
	xWcrtCalculateTasksWcrt();

	/* Call the application scheduler hook function. This function should be
	 * defined by the programmer of the application scheduler if needed.
	 */
	vSchedulerTaskSchedulerStartLogic();

#if ( configUSE_SCHEDULER_START_HOOK == 1 )
	/* Call the hook function. This function should be defined by the user. */
	vSchedulerStartHook();
#endif

	/* Start the FreeRTOS scheduler. */
	vTaskStartScheduler();

	for(;;);
}

void vSchedulerSetup( void )
{
	vSchedulerLogicSetup();
}

/**
 * AppSched_TaskCreate()
 */
BaseType_t xSchedulerTaskCreate( TaskFunction_t pxTaskCode, const char * const pcName, const uint16_t usStackDepth, void * const pvParameters, UBaseType_t uxPriority, struct TaskInfo ** pxCreatedTask, TickType_t xPeriod, TickType_t xDeadline, TickType_t xWcet )
{
	struct TaskInfo *pxTaskInfo = ( struct TaskInfo * ) pvPortMalloc( sizeof( struct TaskInfo ) );

	BaseType_t xRslt = pdFAIL;

	if( pxTaskInfo != NULL )
	{
		/* Initialize the scheduler tasks TCBe members. */
		pxTaskInfo->xPriority = uxPriority;
		pxTaskInfo->xPeriod = xPeriod;
		pxTaskInfo->xDeadline = xDeadline;
		pxTaskInfo->xAbsolutDeadline = xDeadline;
		pxTaskInfo->xRelease = 0;
		pxTaskInfo->xWcet = xWcet;
		pxTaskInfo->xWcrt = 0;
		pxTaskInfo->uxReleaseCount = 0;
		pxTaskInfo->xCur = 0;
		pxTaskInfo->xFinished = 0;
		pxTaskInfo->xHstTaskType = HST_PERIODIC;

		if ( pxTaskInfo->xPeriod == 0 )
		{
			pxTaskInfo->xHstTaskType = HST_APERIODIC;
		}

		/* Create the FreeRTOS task. */
		xRslt = xTaskCreate( pxTaskCode, pcName, usStackDepth, pxTaskInfo, TASK_PRIORITY, &( pxTaskInfo->xHandle ) );

		if( xRslt == pdPASS)
		{
			if( ( void * ) pxCreatedTask != NULL )
			{
				/* Pass the TCBe out. */
				*pxCreatedTask = pxTaskInfo;
			}

			/* Add the created task to the scheduler ready list. */
			vSchedulerLogicAddTask( pxTaskInfo );

			/* Associate the eTCB and TCB. */
			vTaskSetThreadLocalStoragePointer( pxTaskInfo->xHandle, 0, ( void * ) pxTaskInfo );

			/* The initial state of a app scheduled task is suspended. */
			vTaskSuspend( pxTaskInfo->xHandle );
		}
		else
		{
			vPortFree( pxTaskInfo );
		}
	}

	return xRslt;
}

/**
 * AppSched_WaitForNextPeriod()
 */
void vSchedulerWaitForNextPeriod()
{
 	vTaskDelayUntil( &( xCurrentTask->xRelease ), xCurrentTask->xPeriod );
}

/**
 * Función invocada en cada interrupción del tick de reloj. Esta función se
 * ejecuta desde una ISR, por lo que debe ser breve, e invocar funciones de
 * FreeRTOS que terminen en "FromISR".
 *
 * AppSched_Tick()
 */
void vApplicationTickHook( void )
{
	if( xCurrentTask != NULL )
	{
		/* Verify for task overrun. */
		if ( ( xCurrentTask->xWcet > 0 ) && ( xCurrentTask->xCur > xCurrentTask->xWcet ) )
		{
			vSchedulerWcetOverrunHook( xCurrentTask, xTaskGetTickCountFromISR() );
		}
	}

	/* Returns pdTRUE if the application scheduler task must be awakened. */
	BaseType_t result = vSchedulerTaskSchedulerTickLogic();

	if( result == pdTRUE )
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		vTaskNotifyGiveFromISR( xSchedulerTask, &xHigherPriorityTaskWoken );
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
}

/**
 * Scheduler task body function.
 *
 * AppSched_TPH()
 */
static void prvSchedulerTaskScheduler( void* params )
{
    do
    {
        /* Scheduler logic */
		vSchedulerTaskSchedulerLogic( &xCurrentTask );

        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
    }
    while( pdTRUE );

    /* If the tasks ever leaves the for cycle, kill it. */
    vTaskDelete( NULL );
}

/**
 * Funcion invocada por las macros traceTASK_DELAY y traceTASK_DELAY_UNTIL. Esto
 * indica que una instancia ha finalizado. Luego se realiza un UP del semaforo
 * xSchedSem, que despertara a la tarea planificadora.
 *
 * AppSched_Delay()
 */
extern void vSchedulerTaskDelay( void )
{
	if( xCurrentTask != NULL )
	{
		/* Wake up the scheduler task only if is an application scheduled task. */
		if( xCurrentTask->xHandle == xTaskGetCurrentTaskHandle() )
		{
			xCurrentTask->xFinished = 1;

			/* A vTaskDelayUntil() or vTaskDelay() invocation terminates the
			 * current release of the task. */
			vSchedulerLogicRemoveTaskFromReadyList( xCurrentTask );

			/* Wake up the scheduler task. */
			xTaskNotifyGive( xSchedulerTask );
		}
	}
}

/**
 * Invoked by traceBLOCKING_ON_QUEUE_RECEIVE and traceBLOCKING_ON_QUEUE_SEND
 * macros.
 *
 * AppSched_Block()
 */
extern void vSchedulerTaskBlock( void* pxResource )
{
	if( xCurrentTask != NULL )
	{
		/* Wake up the scheduler task only if is an application scheduled task. */
		if( xCurrentTask->xHandle == xTaskGetCurrentTaskHandle() )
		{
			vSchedulerLogicRemoveTaskFromReadyList( xCurrentTask );

			/* Wake up the scheduler task. */
			xTaskNotifyGive( xSchedulerTask );
		}
	}
}

/**
 * We must identify if the invocation is called from the aperiodic
 * task, and not from the vSchedulerTaskSchedulerLogic().
 *
 * AppSched_Suspend()
 */
extern void vSchedulerTaskSuspend( void* pxTask )
{
	if( xSchedulerTask != xTaskGetCurrentTaskHandle() )
	{
		if( xCurrentTask != NULL )
		{
			if( xCurrentTask->xHandle == ( TaskHandle_t ) pxTask )
			{
				xCurrentTask->xFinished = 1;

				vSchedulerLogicRemoveTaskFromReadyList( xCurrentTask );

				/* Wake up the scheduler task. */
				xTaskNotifyGive( xSchedulerTask );
			}
		}
	}
}

/**
 * Invoked by the traceMOVED_TASK_TO_READY_STATE macro. This macro is called by
 * the function prvAddTaskToReadyList(). If the task is ready to execute, we
 * must wake up the scheduler thread.
 *
 * If using timers, it requires that INCLUDE_xTimerGetTimerDaemonTaskHandle, so
 * the xTimerTaskHandle can be returned by the xTimerGetTimerDaemonTaskHandle()
 * function.
 *
 * Maps to the AppSched_Ready() function in the HST model.
 */
extern void vSchedulerTaskReady( void* pxTask )
{
	if( xSchedulerTask != xTaskGetCurrentTaskHandle() )
	{
		/* The scheduler task is not running. Then xTask had been moved to the
		 * ready task list by FreeRTOS, because it is a new release of a periodic
		 * task, or it has been unblocked.
		 */
		struct TaskInfo * pxTaskInfo = NULL;

#if( configUSE_TIMERS == 1 )
		/* Check if the unblocked task was the timer task. */
		if( xTimerGetTimerDaemonTaskHandle() == ( TaskHandle_t ) pxTaskInfo )
		{
			pxTask = NULL;
		}
#else
		pxTaskInfo = ( struct TaskInfo * ) pvTaskGetThreadLocalStoragePointer( pxTask, 0 );
#endif

		if( pxTaskInfo != NULL )
		{
			if( pxTaskInfo->xFinished == 1 )
			{
				/* If pxTask is a new instance, update the absolute deadline of
				 * the release, reset the CPU counter and increment the release
				 * counter.
				 */
				pxTaskInfo->xAbsolutDeadline = pxTaskInfo->xRelease + pxTaskInfo->xDeadline;
				pxTaskInfo->uxReleaseCount = pxTaskInfo->uxReleaseCount + 1;
				pxTaskInfo->xFinished = 0;
				pxTaskInfo->xCur = 0;

				/* Add the task to the appropriate ready list. */
				vSchedulerLogicAddTaskToReadyList( pxTaskInfo );
			}

			/* Wake up the scheduler task. */
			if( xSchedulerTask != NULL )
			{
				/* If vSchedulerTaskReady is called from an ISR, we need to
				 * invoke the FromISR variant of xSemaphoreGive(). This can
				 * occur when the task is unblocked in xTaskIncrementTick().
				 */
				BaseType_t xHigherPriorityTaskWoken = pdFALSE;
				vTaskNotifyGiveFromISR( xSchedulerTask, &xHigherPriorityTaskWoken );
				portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
			}
		}
	}
}
