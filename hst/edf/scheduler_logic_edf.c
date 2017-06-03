#include "FreeRTOS.h"
#include "scheduler.h"
#include "scheduler_logic.h"

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

	*pxCurrentTask = NULL;

	/* Periodic task scheduling -- resume the execution of the first task in the ready list, if any. */
	if( listLIST_IS_EMPTY( pxReadyTasksList ) == pdFALSE )
	{
		*pxCurrentTask = ( struct TaskInfo * ) listGET_OWNER_OF_HEAD_ENTRY( pxReadyTasksList );
	}
}

/**
 *
 */
void vSchedulerLogicAddTaskToReadyList( struct TaskInfo *xTask )
{
	listSET_LIST_ITEM_VALUE( &( xTask->xReadyListItem ), xTask->xAbsolutDeadline );
	vListInsert( pxReadyTasksList, &( xTask->xReadyListItem ) );
}

/**
 *
 */
void vSchedulerLogicRemoveTaskFromReadyList( struct TaskInfo *xTask )
{
	uxListRemove( &( xTask->xReadyListItem ) );
}

/**
 *
 */
void vSchedulerLogicAddTask( struct TaskInfo * pxTask )
{
	/* Initialize the task's ready item list. */
	vListInitialiseItem( &( pxTask->xReadyListItem ) );
	listSET_LIST_ITEM_OWNER( &( pxTask->xReadyListItem ), pxTask );
	listSET_LIST_ITEM_VALUE( &( pxTask->xReadyListItem ), pxTask->xDeadline );

	/* Insert the task into the ready list. */
	vListInsert( pxReadyTasksList, &( pxTask->xReadyListItem ) );
}

/**
 *
 */
void vSchedulerLogicSetup( void )
{
	/* Initialize the ready task ready list. */
	vListInitialise( &( xReadyTasksList ) );
	pxReadyTasksList = &( xReadyTasksList );
	pxAllTasksList = &( xReadyTasksList );
}
