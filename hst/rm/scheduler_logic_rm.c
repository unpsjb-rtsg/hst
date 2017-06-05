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
void vSchedulerTaskSchedulerLogic( HstTCB_t **pxCurrentTask )
{
	*pxCurrentTask = NULL;

	/* Select the first task in the ready list, if any. */
	if( listLIST_IS_EMPTY( pxReadyTasksList ) == pdFALSE )
	{
		*pxCurrentTask = ( HstTCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxReadyTasksList );
	}
}

/**
 * Add xTask to the appropriate ready task list.
 */
void vSchedulerLogicAddTaskToReadyList( HstTCB_t *xTask )
{
	vListInsert( pxReadyTasksList, &( xTask->xReadyListItem ) );
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
void vSchedulerLogicAddTask( HstTCB_t *pxTask )
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
