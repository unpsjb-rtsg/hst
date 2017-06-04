#include "FreeRTOS.h"
#include "scheduler.h"
#include "scheduler_logic.h"
#include "wcrt.h"
#include "semphr.h"
#include "queue.h"

#define ONE_TICK ( ( TickType_t ) 1 )

#if ( configUSE_SCHEDULER_START_HOOK == 1 )
/* Application Hooks. */
extern void vSchedulerStartHook( void );
#endif

extern void vSchedulerWcetOverrunHook( HstTCB_t *xTask, const TickType_t xTickCount );
extern void vSchedulerDeadlineMissHook( HstTCB_t *xTask, const TickType_t xTickCount );

/* Callback function called from the FreeRTOS tick interrupt service. */
void vApplicationTickHook( void );

/* HST function. */
static void prvSchedulerTaskScheduler( void * params );

/* Absolute deadlines. */
static List_t xAbsDeadlinesList;

/* Scheduler task handle. */
static TaskHandle_t xSchedulerTask = NULL;

/* Current task. */
static HstTCB_t *xCurrentTask = NULL;

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
	/* Initialze the absolute deadlines list. */
	vListInitialise( &( xAbsDeadlinesList ) );

	vSchedulerLogicSetup();
}

/**
 * AppSched_TaskCreate()
 */
BaseType_t xSchedulerTaskCreate( TaskFunction_t pxTaskCode, const char * const pcName, const uint16_t usStackDepth, void * const pvParameters, UBaseType_t uxPriority, HstTCB_t **pxCreatedTask, TickType_t xPeriod, TickType_t xDeadline, TickType_t xWcet )
{
	HstTCB_t *pxTaskHstTCB = ( HstTCB_t * ) pvPortMalloc( sizeof( HstTCB_t ) );

	BaseType_t xRslt = pdFAIL;

	if( pxTaskHstTCB != NULL )
	{
		/* Initialize the scheduler tasks TCBe members. */
		pxTaskHstTCB->xPriority = uxPriority;
		pxTaskHstTCB->xPeriod = xPeriod;
		pxTaskHstTCB->xDeadline = xDeadline;
		pxTaskHstTCB->xAbsolutDeadline = xDeadline;
		pxTaskHstTCB->xRelease = 0;
		pxTaskHstTCB->xWcet = xWcet;
		pxTaskHstTCB->xWcrt = 0;
		pxTaskHstTCB->uxReleaseCount = 0;
		pxTaskHstTCB->xCur = 0;
		pxTaskHstTCB->xHstTaskType = HST_PERIODIC;
		pxTaskHstTCB->xState = HST_READY;

		if ( pxTaskHstTCB->xPeriod == 0 )
		{
			pxTaskHstTCB->xHstTaskType = HST_APERIODIC;
		}

		/* Create the FreeRTOS task. */
		xRslt = xTaskCreate( pxTaskCode, pcName, usStackDepth, pxTaskHstTCB, TASK_PRIORITY, &( pxTaskHstTCB->xHandle ) );

		if( xRslt == pdPASS)
		{
			if( ( void * ) pxCreatedTask != NULL )
			{
				/* Pass the TCBe out. */
				*pxCreatedTask = pxTaskHstTCB;
			}

			/* Initialize task absolute deadline item. */
			if ( pxTaskHstTCB->xHstTaskType == HST_PERIODIC )
			{
		        vListInitialiseItem( &( pxTaskHstTCB->xAbsDeadlineListItem ) );
		        listSET_LIST_ITEM_OWNER( &( pxTaskHstTCB->xAbsDeadlineListItem ), pxTaskHstTCB );
		        listSET_LIST_ITEM_VALUE( &( pxTaskHstTCB->xAbsDeadlineListItem ), pxTaskHstTCB->xAbsolutDeadline );
		        vListInsert( &xAbsDeadlinesList, &( pxTaskHstTCB->xAbsDeadlineListItem ) );
			}

			/* Add the created task to the scheduler ready list. */
			vSchedulerLogicAddTask( pxTaskHstTCB );

			/* Associate the eTCB and TCB. */
			vTaskSetThreadLocalStoragePointer( pxTaskHstTCB->xHandle, 0, ( void * ) pxTaskHstTCB );

			/* The initial state of a HST scheduled task is suspended. */
			vTaskSuspend( pxTaskHstTCB->xHandle );
		}
		else
		{
			vPortFree( pxTaskHstTCB );
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
        /* Update execution time. */
		xCurrentTask->xCur = xCurrentTask->xCur + ONE_TICK;

        /* Verify for task overrun. */
		if ( ( xCurrentTask->xWcet > 0 ) && ( xCurrentTask->xCur > xCurrentTask->xWcet) )
		{
			vSchedulerWcetOverrunHook( xCurrentTask, xTaskGetTickCountFromISR() );
		}
	}

	const TickType_t xTickCount = xTaskGetTickCountFromISR();

	/* Verify deadlines. */
	if( listLIST_IS_EMPTY( &xAbsDeadlinesList ) == pdFALSE )
	{
		ListItem_t *pxAbsDeadlineListItem = listGET_HEAD_ENTRY( &xAbsDeadlinesList );

		while( listGET_END_MARKER( &xAbsDeadlinesList ) != pxAbsDeadlineListItem )
		{
			if( listGET_LIST_ITEM_VALUE( pxAbsDeadlineListItem ) < xTickCount )
			{
				/* Missed deadline. */
				vSchedulerDeadlineMissHook( ( HstTCB_t * ) listGET_LIST_ITEM_OWNER( pxAbsDeadlineListItem ), xTickCount );
			}
			else
			{
				/* As xAbsDeadlineListItem is deadline-ordered if the current
				item absolute deadline instant is greater or equal than the 
				current tick count, then the remaining items too. */
				break;
			}

			// Next absolute deadline.
			pxAbsDeadlineListItem = listGET_NEXT( pxAbsDeadlineListItem );		
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
		vTaskSuspendAll();

		if ( xCurrentTask != NULL )
		{
		    if ( xCurrentTask->xState == HST_SUSPENDED )
	        {
	        	// The current task suspended itself.
		    	if ( xCurrentTask->xHstTaskType == HST_APERIODIC )
		    	{
		    		xCurrentTask->xState = HST_FINISHED;
		    	}
	        	vSchedulerLogicRemoveTaskFromReadyList( xCurrentTask );
	        }
		    else if ( xCurrentTask->xState == HST_BLOCKED )
	        {
	        	// The current task was blocked.
	        	vSchedulerLogicRemoveTaskFromReadyList( xCurrentTask );
	        }
		    else if ( xCurrentTask->xState == HST_FINISHED )
	        {
	        	// Remove the finished task absolute deadline item from the deadline list.
	        	if ( xCurrentTask->xHstTaskType == HST_PERIODIC )
	        	{
	        		uxListRemove( &( xCurrentTask->xAbsDeadlineListItem ) );
	        	}

	        	/* A vTaskDelayUntil() or vTaskDelay() invocation terminates the
	        	 * current release of the task. */
	        	vSchedulerLogicRemoveTaskFromReadyList( xCurrentTask );
	        }
		}

		ListItem_t * pxAppTasksListItem = listGET_HEAD_ENTRY( pxAllTasksList );

		/* Suspend all ready tasks. */
		while( pxAppTasksListItem != listGET_END_MARKER( pxAllTasksList ))
		{
			HstTCB_t *pxAppTask = ( HstTCB_t * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem );

			if( eTaskGetState( pxAppTask->xHandle ) == eReady )
			{
				vTaskSuspend( pxAppTask->xHandle );
			}

			pxAppTasksListItem = listGET_NEXT( pxAppTasksListItem );
		}

        /* Scheduler logic */
		vSchedulerTaskSchedulerLogic( &xCurrentTask );

		/* Resume the execution of the selected task. */
		if ( xCurrentTask != NULL )
		{
			xCurrentTask->xState = HST_READY;
			vTaskResume( xCurrentTask->xHandle );
		}

		xTaskResumeAll();

        /* With pdTRUE this acts as a binary semaphore. */
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
	/* Wake up the scheduler task only if is an application scheduled task. */
	if( ( xCurrentTask != NULL ) && ( xCurrentTask->xHandle == xTaskGetCurrentTaskHandle() ) )
	{
		xCurrentTask->xState = HST_FINISHED;
		vTaskNotifyGiveFromISR( xSchedulerTask, NULL );
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
	/* Wake up the scheduler task only if is an application scheduled task. */
	if( ( xCurrentTask != NULL ) && ( xCurrentTask->xHandle == xTaskGetCurrentTaskHandle() ) )
	{
		xCurrentTask->xState = HST_BLOCKED;
		vTaskNotifyGiveFromISR( xSchedulerTask, NULL );
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
	/* Wake up the scheduler task only if is an application scheduled task. */
	if ( xCurrentTask != NULL && ( xCurrentTask->xHandle == xTaskGetCurrentTaskHandle() ) )
	{
		xCurrentTask->xState = HST_SUSPENDED;
		vTaskNotifyGiveFromISR( xSchedulerTask, NULL );
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
#if( configUSE_TIMERS == 1 )
	/* Check if the unblocked task was the timer task. */
	if( xTimerGetTimerDaemonTaskHandle() == ( TaskHandle_t ) pxTask )
	{
		return;
	}
#endif
    if( xSchedulerTask == ( TaskHandle_t ) pxTask )
	{
    	/* The task that is transitioned into the Ready state is the HST. */
		return;
	}

	if( xSchedulerTask != xTaskGetCurrentTaskHandle() )
	{
		/* The scheduler task is not running. Then xTask had been moved to the
		 * ready task list by FreeRTOS, because it is a new release of task or
		 * it has been unblocked/resumed.
		 */
		HstTCB_t *pxTaskInfo = ( HstTCB_t * ) pvTaskGetThreadLocalStoragePointer( pxTask, 0 );

		if( pxTaskInfo != NULL )
		{
			if( pxTaskInfo->xState == HST_FINISHED )
			{
				/* If pxTask is a new instance, update the absolute deadline of
				 * the release, reset the CPU counter and increment the release
				 * counter.
				 */
				pxTaskInfo->xAbsolutDeadline = pxTaskInfo->xRelease + pxTaskInfo->xDeadline;
				pxTaskInfo->uxReleaseCount = pxTaskInfo->uxReleaseCount + 1;
				pxTaskInfo->xState = HST_READY;
				pxTaskInfo->xCur = 0;

				if ( pxTaskInfo->xHstTaskType == HST_PERIODIC )
				{
				    /* Add task's absolute deadline into deadlines list. */
				    if( listIS_CONTAINED_WITHIN( &xAbsDeadlinesList, &( pxTaskInfo->xAbsDeadlineListItem ) ) == pdFALSE )
				    {
				    	listSET_LIST_ITEM_VALUE( &( pxTaskInfo->xAbsDeadlineListItem ), pxTaskInfo->xAbsolutDeadline );
				    	vListInsert( &xAbsDeadlinesList, &( pxTaskInfo->xAbsDeadlineListItem ) );
				    }
				}

				/* Add the task to the appropriate ready list. */
				vSchedulerLogicAddTaskToReadyList( pxTaskInfo );
			}

			/* Wake up the scheduler task, if not running */
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
