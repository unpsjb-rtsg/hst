#include "FreeRTOS.h"
#include "scheduler.h"
#include "scheduler_logic.h"
#include "slack.h"

#define MIN_SLACK 0
#define ONE_TICK ( ( TickType_t ) 1 )

static BaseType_t xUsingSlack = pdFALSE;

extern void vSchedulerNegativeSlackHook( TickType_t xTickCount, BaseType_t xSlack );

/* Periodic tasks list. */
List_t xAllTasksList;
List_t * pxAllTasksList = NULL;

/* Aperiodic tasks list. */
static List_t xAllAperiodicTasksList;
static List_t * pxAllAperiodicTasksList = NULL;

/* Ready tasks list. */
static List_t xReadyTasksList;
static List_t * pxReadyTasksList = NULL;

/* Ready aperiodic / sporadic list. */
static List_t xAperiodicReadyTasksList;
static List_t * pxAperiodicReadyTasksList = NULL;

/**
 * AppSchedLogic_Init()
 */
void vSchedulerTaskSchedulerStartLogic( void )
{
	/* Initialize the ready task ready list. */
	vListInitialise( &( xReadyTasksList ) );
	pxReadyTasksList = &( xReadyTasksList );

	/* Initialize the aperiodic task ready list. */
	vListInitialise( &( xAperiodicReadyTasksList ) );
	pxAperiodicReadyTasksList = &( xAperiodicReadyTasksList );

	ListItem_t * pxAppTasksListItem = listGET_HEAD_ENTRY( pxAllTasksList );

	/* Init periodic tasks. */
	while( listGET_END_MARKER( pxAllTasksList ) != pxAppTasksListItem )
	{
		/* Pointer to the application scheduled task. */
		HstTCB_t * pxAppTask = ( HstTCB_t * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem );

		/* Init the slack structure. */
		TaskSs_t * pxTaskInfoSlack = ( TaskSs_t * ) pvPortMalloc( sizeof( TaskSs_t ) );

		/* Initialize slack methods attributes. */
		pxTaskInfoSlack->xDi = 0;
		pxTaskInfoSlack->xSlack = 0;
		pxTaskInfoSlack->xTtma = 0;
		pxTaskInfoSlack->xK = 0;

		pxAppTask->vExt = pxTaskInfoSlack;

		vSlackCalculateSlack_fixed1( pxAppTask, 0 );

		pxTaskInfoSlack->xK = pxTaskInfoSlack->xSlack;

		/* Insert the task into the ready list. */
		vListInsert( pxReadyTasksList, &( pxAppTask->xReadyListItem ) );

		pxAppTasksListItem = listGET_NEXT( pxAppTasksListItem );
	}

	pxAppTasksListItem = listGET_HEAD_ENTRY( pxAllAperiodicTasksList );

	/* Init periodic tasks. */
	while( listGET_END_MARKER( pxAllAperiodicTasksList ) != pxAppTasksListItem )
	{
		/* Init aperiodic tasks. */
		vSchedulerLogicAddTaskToReadyList( ( HstTCB_t * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem ) );

		pxAppTasksListItem = listGET_NEXT( pxAppTasksListItem );
	}

	/* Minimal slack at the critical instant. */
	vSlackUpdateAvailableSlack( &xAvailableSlack );
}

/**
 * AppSchedLogic_Tick()
 */
BaseType_t vSchedulerTaskSchedulerTickLogic()
{
	BaseType_t xResult = pdFALSE;

	/* ==================   Update slack   ================== */
	if( xUsingSlack == pdTRUE )
	{
		/* A NRTT is using Available Slack -- decrement all slack counters, and
		 * increment the current aperiodic task executed time. */
		vSlackDecrementAllTasksSlack( ONE_TICK );
		HstTCB_t * pxAperiodicTask = ( HstTCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxAperiodicReadyTasksList );
	}
	else
	{
		if( listLIST_IS_EMPTY( pxReadyTasksList ) == pdFALSE )
		{
			/* A RTT is running -- decrement higher priority tasks slack, and
			 * increment the current task executed time. */
			HstTCB_t * pxTask = ( HstTCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxReadyTasksList );
			vSlackDecrementTasksSlack( pxTask , ONE_TICK );
		}
		else
		{
			/* The Idle task or a system scheduled task is running -- decrement
			 * all slack counters. */
			vSlackDecrementAllTasksSlack( ONE_TICK );
		}
	}

	/* Update the xAvailableSlack global counter with the minimum slack. */
	vSlackUpdateAvailableSlack( &xAvailableSlack );

	if( xAvailableSlack < 0 )
	{
		vSchedulerNegativeSlackHook( xTaskGetTickCountFromISR(), xAvailableSlack );
	}

	/* If the system slack is below the minimum, wake up the scheduler. */
	if( xUsingSlack == pdTRUE )
	{
		if( xAvailableSlack <= MIN_SLACK )
		{
			xResult = pdTRUE;
		}
	}

	return xResult;
}

/**
 * AppSchedLogic_Sched()
 */
void vSchedulerTaskSchedulerLogic( HstTCB_t **pxCurrentTask )
{
	/* Current RTOS tick value. */
	const TickType_t xTickCount = xTaskGetTickCount();

	/* Check if the current release of the periodic task has finished. */
	if( *pxCurrentTask != NULL )
	{
		if( ( *pxCurrentTask )->xHstTaskType == HST_PERIODIC )
		{
			if( ( *pxCurrentTask )->xState == HST_FINISHED )
			{
#if ( USE_SLACK_K == 0 )
				/* Recalculate slack. */
				vSlackCalculateSlack_fixed1( *pxCurrentTask, xTickCount );
#else
				struct TaskInfo_Slack * pxTaskSlack = ( struct TaskInfo_Slack * ) ( *pxCurrentTask )->vExt;
				pxTaskSlack->xSlack = pxTaskSlack->xK;
#endif

				if( ( *pxCurrentTask )->xWcet > ( *pxCurrentTask )->xCur )
				{
					/* The release executed for less than the task worst case
					 * execution time, so vSlackGainSlack() is called to assign
					 * this time to lower priority tasks.
					 */
					vSlackGainSlack( *pxCurrentTask, ( ( *pxCurrentTask )->xWcet - ( *pxCurrentTask )->xCur ) );
				}

				/* Update the available slack. */
				vSlackUpdateAvailableSlack( &xAvailableSlack );

				*pxCurrentTask = NULL;
			}
		}
	}

	if( xAvailableSlack < 0 )
	{
		vSchedulerNegativeSlackHook( xTickCount, xAvailableSlack );
	}

	if( xAvailableSlack > MIN_SLACK )
	{
		if( listLIST_IS_EMPTY( pxAperiodicReadyTasksList ) == pdFALSE )
		{
			if( xUsingSlack == pdFALSE )
			{
				/* Resume the execution of the first aperiodic task in the
				 * ready list. */
				*pxCurrentTask = ( HstTCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxAperiodicReadyTasksList );
				xUsingSlack = pdTRUE;
			}
		}
		else
		{
			xUsingSlack = pdFALSE;
		}
	}
	else
	{
		if( xUsingSlack == pdTRUE )
		{
			vTaskSuspend( ( *pxCurrentTask )->xHandle );
			xUsingSlack = pdFALSE;
		}
	}

	if( xUsingSlack == pdFALSE )
	{
	    /* Resume the execution of the first task in the ready list, if any. */
		if( listLIST_IS_EMPTY( pxReadyTasksList ) == pdFALSE )
		{
			*pxCurrentTask = ( HstTCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxReadyTasksList );
		}
	}
}

/**
 * Add xTask to the appropiate ready task list.
 */
void vSchedulerLogicAddTaskToReadyList( HstTCB_t *xTask )
{
	if( xTask->xHstTaskType == HST_PERIODIC )
	{
		vListInsert( pxReadyTasksList, &( xTask->xReadyListItem ) );
	}
	else
	{
		vListInsert( pxAperiodicReadyTasksList, &( xTask->xReadyListItem ) );
	}
}

/**
 * Remove xTask from the ready task list.
 */
void vSchedulerLogicRemoveTaskFromReadyList( HstTCB_t *xTask )
{
	uxListRemove( &( xTask->xReadyListItem ) );
}

/**
 * Add pxTask as a application scheduled task by the HST.
 */
void vSchedulerLogicAddTask( HstTCB_t * pxTask )
{
	/* Initialize the task's generic item list. */
	vListInitialiseItem( &( pxTask->xGenericListItem ) );
	listSET_LIST_ITEM_OWNER( &( pxTask->xGenericListItem ), pxTask );
	listSET_LIST_ITEM_VALUE( &( pxTask->xGenericListItem ), pxTask->xPriority );

	/* Initialize the task's ready item list. */
	vListInitialiseItem( &( pxTask->xReadyListItem ) );
	listSET_LIST_ITEM_OWNER( &( pxTask->xReadyListItem ), pxTask );
	listSET_LIST_ITEM_VALUE( &( pxTask->xReadyListItem ), pxTask->xPriority );

	/* Add the task to the correct appropiate task list. */
	if( pxTask->xPeriod > 0U )
	{
		/* Periodic task. */
		vListInsert( pxAllTasksList, &( pxTask->xGenericListItem ) );
	}
	else
	{
		/* Aperiodic task. */
		vListInsert( pxAllAperiodicTasksList, &( pxTask->xGenericListItem ) );
	}
}

/**
 * Performs any previous work needed by the HST scheduler.
 */
void vSchedulerLogicSetup( void )
{
	/* Initialize the periodic task list. */
	vListInitialise( &( xAllTasksList ) );
	pxAllTasksList = &( xAllTasksList );

	/* Initialize the aperiodic tasks list. */
	vListInitialise( &( xAllAperiodicTasksList ) );
	pxAllAperiodicTasksList = &( xAllAperiodicTasksList );
}
