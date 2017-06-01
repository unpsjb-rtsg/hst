#include "FreeRTOS.h"
#include "scheduler.h"
#include "scheduler_logic.h"

#define ONE_TICK ( ( TickType_t ) 1 )

extern void vSchedulerDeadlineMissHook( struct TaskInfo * xTask, const TickType_t xTickCount );

/* Periodic tasks list. */
List_t xAllTasksList;
List_t * pxAllTasksList = NULL;

/* Promotion times list */
static List_t xPromotionList;

/* Ready tasks list. */
static List_t xReadyTasksListA;	// Upper band
static List_t xReadyTasksListB;	// Middle band
static List_t xReadyTasksListC;	// Lower band

/* Pointers to ready task lists. */
static List_t * pxReadyTasksListA = NULL;
static List_t * pxReadyTasksListB = NULL;
static List_t * pxReadyTasksListC = NULL;

/**
 * AppSchedLogic_Init()
 */
void vSchedulerTaskSchedulerStartLogic( void )
{
	/* Initialze the promotion times list. */
	vListInitialise( &( xPromotionList ) );

	/* Initialize the ready tasks list. */
	vListInitialise( &( xReadyTasksListA ) );
	vListInitialise( &( xReadyTasksListB ) );
	vListInitialise( &( xReadyTasksListC ) );

	pxReadyTasksListA = &( xReadyTasksListA );
	pxReadyTasksListB = &( xReadyTasksListB );
	pxReadyTasksListC = &( xReadyTasksListC );

	ListItem_t * pxAppTasksListItem = listGET_HEAD_ENTRY( pxAllTasksList );

	/* Move tasks to ready tasks lists. */
	while( listGET_END_MARKER( pxAllTasksList ) != pxAppTasksListItem )
	{
		/* Pointer to the application scheduled task. */
		struct TaskInfo * pxAppTask = ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem );

		if( pxAppTask->xPeriod > 0 )
		{
			/* Init the dual priority parameters structure. */
			struct TaskInfo_DP * pxTaskInfoDP = ( struct TaskInfo_DP * ) pvPortMalloc( sizeof( struct TaskInfo_DP ) );
			pxTaskInfoDP->xInUpperBand = pdFALSE;
			pxTaskInfoDP->xPromotion = pxAppTask->xDeadline - pxAppTask->xWcrt;

			pxAppTask->vExt = pxTaskInfoDP;

			/* Add item to promotion list */
			vListInitialiseItem( &( pxTaskInfoDP->xPromotionListItem ) );
			listSET_LIST_ITEM_OWNER( &( pxTaskInfoDP->xPromotionListItem ), pxAppTask );
			listSET_LIST_ITEM_VALUE( &( pxTaskInfoDP->xPromotionListItem ), pxTaskInfoDP->xPromotion );
			vListInsert( &xPromotionList, &( pxTaskInfoDP->xPromotionListItem ) );

			/* Insert the periodic task into the lower band ready list. */
			vListInsert( pxReadyTasksListC, &( pxAppTask->xReadyListItem ) );
		}
		else
		{
			/* Insert the aperiodic task into the middle band ready list. */
			vListInsert( pxReadyTasksListB, &( pxAppTask->xReadyListItem ) );
		}

		pxAppTasksListItem = listGET_NEXT( pxAppTasksListItem );
	}
}

/**
 * AppSchedLogic_Tick()
 */
BaseType_t vSchedulerTaskSchedulerTickLogic()
{
	const TickType_t xTickCount = xTaskGetTickCountFromISR();
	BaseType_t xReturn = pdFALSE;

	/* Task promotion ======================================================  */

	if( listLIST_IS_EMPTY( &xPromotionList ) == pdFALSE )
	{
		ListItem_t *pxPromotionListItem = listGET_HEAD_ENTRY( &xPromotionList );

		while( listGET_END_MARKER( &xPromotionList ) != pxPromotionListItem )
		{
			if( xTickCount == listGET_LIST_ITEM_VALUE( pxPromotionListItem ) )
			{
				/* Removes the task promotion time from the list. */
				uxListRemove( pxPromotionListItem );

				struct TaskInfo * pxTask = ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxPromotionListItem );

				/* Removes the task from the lower band ready list. */
				uxListRemove( &( pxTask->xReadyListItem ) );

				/* Set the priority of the task for its execution in the upper band. */
				listSET_LIST_ITEM_VALUE( &( pxTask->xReadyListItem ), pxTask->xPriority );

				/* Moves the promoted task to the higher band ready list. */
				vListInsert( pxReadyTasksListA, &( pxTask->xReadyListItem ) );
				( ( struct TaskInfo_DP * ) pxTask->vExt )->xInUpperBand = pdTRUE;

				/* As the task was promoted, a context switch is needed to be
				performed. */
				xReturn = pdTRUE;
			}
			else
			{
				/* As xPromotionListItem is promotion-ordered if the current
				item promotion time is greater than the tick count, then the
				remaining items too. */
				break;
			}

			pxPromotionListItem = listGET_NEXT( pxPromotionListItem );
		}
	}

	/* Increment the current task executed time ============================= */

	if( listLIST_IS_EMPTY( pxReadyTasksListA ) == pdFALSE )
	{
		struct TaskInfo * pxTask = ( struct TaskInfo * ) listGET_OWNER_OF_HEAD_ENTRY( pxReadyTasksListA );
		pxTask->xCur = pxTask->xCur + ONE_TICK;
	}
	else if( listLIST_IS_EMPTY( pxReadyTasksListB ) == pdFALSE )
	{
		struct TaskInfo * pxTask = ( struct TaskInfo * ) listGET_OWNER_OF_HEAD_ENTRY( pxReadyTasksListB );
		pxTask->xCur = pxTask->xCur + ONE_TICK;
	}
	else if( listLIST_IS_EMPTY( pxReadyTasksListC ) == pdFALSE )
	{
		struct TaskInfo * pxTask = ( struct TaskInfo * ) listGET_OWNER_OF_HEAD_ENTRY( pxReadyTasksListC );
		pxTask->xCur = pxTask->xCur + ONE_TICK;
	}

	/* Verify deadlines ===================================================== */

	const ListItem_t * pxAppTasksListEndMarker = listGET_END_MARKER( pxReadyTasksListA );
	ListItem_t * pxAppTasksListItem = listGET_HEAD_ENTRY( pxReadyTasksListA );

	while( pxAppTasksListEndMarker != pxAppTasksListItem )
	{
		struct TaskInfo * xTask = ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem );

		if( xTask->xAbsolutDeadline < xTickCount )
		{
			vSchedulerDeadlineMissHook( ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem ), xTickCount );
		}

		pxAppTasksListItem = listGET_NEXT( pxAppTasksListItem );
	}

	return xReturn;
}

/**
 * Upper Band: RM
 * Middle Band: FIFO
 * Lower Band: FIFO
 *
 * AppSchedLogic_Sched()
 */
void vSchedulerTaskSchedulerLogic( struct TaskInfo **pxCurrentTask )
{
	const ListItem_t * pxAppTasksListEndMarker = listGET_END_MARKER( pxAllTasksList );
    ListItem_t * pxAppTasksListItem = listGET_HEAD_ENTRY( pxAllTasksList );

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
		if( ( *pxCurrentTask )->xFinished == 1 )
		{
			*pxCurrentTask = NULL;
		}
	}

	/* Resume the execution of the first task in the higher priority ready list, if any. */
	if( listLIST_IS_EMPTY( pxReadyTasksListA ) == pdFALSE )
	{
		*pxCurrentTask = ( struct TaskInfo * ) listGET_OWNER_OF_HEAD_ENTRY( pxReadyTasksListA );
	}
	else if( listLIST_IS_EMPTY( pxReadyTasksListB ) == pdFALSE )
	{
		*pxCurrentTask = ( struct TaskInfo * ) listGET_OWNER_OF_HEAD_ENTRY( pxReadyTasksListB );
	}
	else if( listLIST_IS_EMPTY( pxReadyTasksListC ) == pdFALSE )
	{
		*pxCurrentTask = ( struct TaskInfo * ) listGET_OWNER_OF_HEAD_ENTRY( pxReadyTasksListC );
	}

	if( *pxCurrentTask != NULL )
	{
		vTaskResume( ( *pxCurrentTask )->xHandle );
	}
}

/**
 * Add xTask to the appropiate ready task list.
 */
void vSchedulerLogicAddTaskToReadyList( struct TaskInfo *xTask )
{
	if( xTask->xPeriod > 0U )
	{
		struct TaskInfo_DP * pxTaskDP = ( struct TaskInfo_DP * ) xTask->vExt;

		if( pxTaskDP->xInUpperBand == pdTRUE )
		{
			vListInsert( pxReadyTasksListA, &( xTask->xReadyListItem ) );
		}
		else
		{
			vListInsert( pxReadyTasksListC, &( xTask->xReadyListItem ) );

			if( listIS_CONTAINED_WITHIN( &xPromotionList, &( pxTaskDP->xPromotionListItem ) ) == pdFALSE )
			{
				listSET_LIST_ITEM_VALUE( &( pxTaskDP->xPromotionListItem ), xTaskGetTickCount() + pxTaskDP->xPromotion );
				vListInsert( &xPromotionList, &( pxTaskDP->xPromotionListItem ) );
			}
		}
	}
	else
	{
		vListInsert( pxReadyTasksListB, &( xTask->xReadyListItem ) );
	}
}

/**
 * Remove xTask from the ready task list.
 */
void vSchedulerLogicRemoveTaskFromReadyList( struct TaskInfo *xTask )
{
	if( xTask->xFinished == 1 )
	{
		/* Check if the task finished before its promotion time -- in which
		case its entry in the promotion list must be removed. */
		struct TaskInfo_DP * pxTaskDP = ( struct TaskInfo_DP * ) xTask->vExt;
		if( pxTaskDP != NULL )
		{
			if( pxTaskDP->xInUpperBand == pdFALSE )
			{
				uxListRemove( &( pxTaskDP->xPromotionListItem ) );
			}

			pxTaskDP->xInUpperBand = pdFALSE;
		}
	}

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
	listSET_LIST_ITEM_VALUE( &( pxTask->xReadyListItem ), 0 );

	/* Periodic task. */
	vListInsert( pxAllTasksList, &( pxTask->xGenericListItem ) );
}

/**
 * Performs any previous work needed by the HST scheduler.
 */
void vSchedulerLogicSetup( void )
{
	/* Initialize the periodic task list. */
	vListInitialise( &( xAllTasksList ) );
	pxAllTasksList = &( xAllTasksList );
}
