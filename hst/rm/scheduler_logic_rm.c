#include "FreeRTOS.h"
#include "scheduler.h"
#include "scheduler_logic.h"

#define ONE_TICK ( ( TickType_t ) 1 )

extern void vSchedulerDeadlineMissHook( struct TaskInfo * xTask, const TickType_t xTickCount );

/* Ready tasks list. */
List_t xReadyTasksList;
List_t * pxReadyTasksList = NULL;
List_t * pxAllTasksList = NULL;

/**
 * AppSchedLogic_Init()
 */
void vSchedulerTaskSchedulerStartLogic( void )
{
	return;
}

/**
 * AppSchedLogic_Tick()
 */
BaseType_t vSchedulerTaskSchedulerTickLogic()
{
	if( listLIST_IS_EMPTY( pxReadyTasksList ) == pdFALSE )
	{
		/* Increment the current task executed time. */
		struct TaskInfo * pxTask = ( struct TaskInfo * ) listGET_OWNER_OF_HEAD_ENTRY( pxReadyTasksList );
		pxTask->xCur = pxTask->xCur + ONE_TICK;
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

	return pdFALSE;
}

/**
 * AppSchedLogic_Sched()
 */
void vSchedulerTaskSchedulerLogic( struct TaskInfo **pxCurrentTask )
{
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
		if( listIS_CONTAINED_WITHIN( pxReadyTasksList, &( ( *pxCurrentTask )->xReadyListItem ) ) == pdFALSE )
		{
			*pxCurrentTask = NULL;
		}
	}

	/* Periodic task scheduling -- resume the execution of the first task in the ready list, if any. */
	if( listLIST_IS_EMPTY( pxReadyTasksList ) == pdFALSE )
	{
		*pxCurrentTask = ( struct TaskInfo * ) listGET_OWNER_OF_HEAD_ENTRY( pxReadyTasksList );
		vTaskResume( ( *pxCurrentTask )->xHandle );
	}
}

/**
 * Add xTask to the appropriate ready task list.
 */
void vSchedulerLogicAddTaskToReadyList( struct TaskInfo *xTask )
{
	vListInsert( pxReadyTasksList, &( xTask->xReadyListItem ) );
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
	/* Initialize the task's ready item list. */
	vListInitialiseItem( &( pxTask->xReadyListItem ) );
	listSET_LIST_ITEM_OWNER( &( pxTask->xReadyListItem ), pxTask );
	listSET_LIST_ITEM_VALUE( &( pxTask->xReadyListItem ), pxTask->xPriority );

	/* Add the task to the task list. */
	vListInsert( pxReadyTasksList, &( pxTask->xReadyListItem ) );
}

/**
 * Performs any previous work needed by the HST scheduler.
 */
void vSchedulerLogicSetup( void )
{
	/* Initialize the periodic task list. */
	vListInitialise( &( xReadyTasksList ) );
	pxReadyTasksList = &( xReadyTasksList );
	pxAllTasksList = &( xReadyTasksList );
}
