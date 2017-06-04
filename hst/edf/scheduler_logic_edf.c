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

	/* Periodic task scheduling -- resume the execution of the first task in the ready list, if any. */
	if( listLIST_IS_EMPTY( pxReadyTasksList ) == pdFALSE )
	{
		*pxCurrentTask = ( HstTCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxReadyTasksList );
	}
}

/**
 *
 */
void vSchedulerLogicAddTaskToReadyList( HstTCB_t *xTask )
{
	listSET_LIST_ITEM_VALUE( &( xTask->xReadyListItem ), xTask->xAbsolutDeadline );
	vListInsert( pxReadyTasksList, &( xTask->xReadyListItem ) );
}

/**
 *
 */
void vSchedulerLogicRemoveTaskFromReadyList( HstTCB_t *xTask )
{
	uxListRemove( &( xTask->xReadyListItem ) );
}

/**
 *
 */
void vSchedulerLogicAddTask( HstTCB_t *pxTask )
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
