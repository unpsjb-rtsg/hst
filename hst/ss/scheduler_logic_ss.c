#include "FreeRTOS.h"
#include "scheduler.h"
#include "scheduler_logic.h"
#include "slack.h"

#define MIN_SLACK 0
#define ONE_TICK ( ( TickType_t ) 1 )

static BaseType_t xUsingSlack = pdFALSE;

extern void vSchedulerNegativeSlackHook( TickType_t xTickCount, BaseType_t xSlack );
extern void vSchedulerDeadlineMissHook( struct TaskInfo * xTask, const TickType_t xTickCount );

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
		struct TaskInfo * pxAppTask = ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem );

		/* Init the slack structure. */
		struct TaskInfo_Slack * pxTaskInfoSlack = ( struct TaskInfo_Slack * ) pvPortMalloc( sizeof( struct TaskInfo_Slack ) );

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
		vSchedulerLogicAddTaskToReadyList( ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem ) );

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
		struct TaskInfo * pxAperiodicTask = ( struct TaskInfo * ) listGET_OWNER_OF_HEAD_ENTRY( pxAperiodicReadyTasksList );
		pxAperiodicTask->xCur = pxAperiodicTask->xCur + ONE_TICK;
	}
	else
	{
		if( listLIST_IS_EMPTY( pxReadyTasksList ) == pdFALSE )
		{
			/* A RTT is running -- decrement higher priority tasks slack, and
			 * increment the current task executed time. */
			struct TaskInfo * pxTask = ( struct TaskInfo * ) listGET_OWNER_OF_HEAD_ENTRY( pxReadyTasksList );
			vSlackDecrementTasksSlack( pxTask , ONE_TICK );
			pxTask->xCur = pxTask->xCur + ONE_TICK;
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

	/* ==================   Verify deadlines   ================== */
	const TickType_t xTickCount = xTaskGetTickCountFromISR();

	const ListItem_t * pxAppTasksListEndMarker = listGET_END_MARKER( pxReadyTasksList );
	ListItem_t * pxAppTasksListItem = listGET_HEAD_ENTRY( pxReadyTasksList );

	while( pxAppTasksListEndMarker != pxAppTasksListItem )
	{
		struct TaskInfo * xTask = ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem );

		if( xTask->xAbsolutDeadline < xTickCount )
		{
			vSchedulerDeadlineMissHook( ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem ), xTickCount );
		}

		pxAppTasksListItem = listGET_NEXT( pxAppTasksListItem );
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
void vSchedulerTaskSchedulerLogic( struct TaskInfo **pxCurrentTask )
{
	/* Current RTOS tick value. */
	const TickType_t xTickCount = xTaskGetTickCount();

	const ListItem_t * pxAppTasksListEndMarker = listGET_END_MARKER( pxReadyTasksList );
    ListItem_t * pxAppTasksListItem = listGET_HEAD_ENTRY( pxReadyTasksList );

    /* Suspend all ready tasks. */
    while( pxAppTasksListEndMarker != pxAppTasksListItem )
    {
    	struct TaskInfo * pxAppTask = ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem );

    	if( eTaskGetState( pxAppTask->xHandle ) == eReady )
    	{
    		vTaskSuspend( pxAppTask->xHandle );
    	}

    	pxAppTasksListItem = listGET_NEXT( pxAppTasksListItem );
    }

	/* Check if the current release of the periodic task has finished. */
	if( *pxCurrentTask != NULL )
	{
		if( ( *pxCurrentTask )->xPeriod > 0 )
		{
			if( ( *pxCurrentTask )->xFinished == 1 )
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
				/* No aperiodic tasks are running. */
				*pxCurrentTask = ( struct TaskInfo * ) listGET_OWNER_OF_HEAD_ENTRY( pxAperiodicReadyTasksList );
				vTaskResume( ( *pxCurrentTask )->xHandle );
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
	    /* Periodic task scheduling -- resume the execution of the first task
	     * in the ready list, if any. */
		if( listLIST_IS_EMPTY( pxReadyTasksList ) == pdFALSE )
		{
			*pxCurrentTask = ( struct TaskInfo * ) listGET_OWNER_OF_HEAD_ENTRY( pxReadyTasksList );
			vTaskResume( ( *pxCurrentTask )->xHandle );
		}
	}
}

/**
 * Add xTask to the appropiate ready task list.
 */
void vSchedulerLogicAddTaskToReadyList( struct TaskInfo *xTask )
{
	if( xTask->xPeriod > 0U )
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
void vSchedulerLogicRemoveTaskFromReadyList( struct TaskInfo *xTask )
{
	uxListRemove( &( xTask->xReadyListItem ) );
}

/**
 * Add pxTask as a application scheduled task by the HST.
 */
void vSchedulerLogicAddTask( struct TaskInfo * pxTask )
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
