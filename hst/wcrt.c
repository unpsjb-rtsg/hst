#include "wcrt.h"
#include "scheduler.h"
#include "scheduler_logic.h"  // pxBigTaskList
#include "utils.h"

/**
 * Sjodin
 */
BaseType_t xWcrtCalculateTasksWcrt( void )
{
	TickType_t xW = 0U;

	const ListItem_t * pxAppTasksListEndMarker = listGET_END_MARKER( pxAllTasksList );
	ListItem_t * pxAppTasksListItem = listGET_HEAD_ENTRY( pxAllTasksList );

	struct TaskInfo *pxTask = ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem );

	/* First task WCRT. */
	TickType_t xT = pxTask->xWcet;
    pxTask->xWcrt = xT;

    /* Check first task deadline. */
    if( pxTask->xWcrt > pxTask->xPeriod )
    {
        return pdFALSE;
    }

    /* Next task. */
    pxAppTasksListItem = listGET_NEXT( pxAppTasksListItem );

    /* Process all the periodic tasks in xTasks. */
    while( pxAppTasksListEndMarker != pxAppTasksListItem )
	{
    	pxTask = ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem );

		xT = xT + pxTask->xWcet;

		while( xT <= pxTask->xDeadline )
		{
			xW = 0;

			/* Calculates the workload of the higher priority tasks than pxTask. */
			ListItem_t * pxAppTaskHigherPrioListItem = listGET_HEAD_ENTRY( pxAllTasksList );
			do
			{
				struct TaskInfo * pxHigherPrioTask = ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTaskHigherPrioListItem );

				xW = xW + ( U_CEIL( xT, pxHigherPrioTask->xPeriod ) * pxHigherPrioTask->xWcet );

				pxAppTaskHigherPrioListItem = listGET_NEXT( pxAppTaskHigherPrioListItem );
			}
			while( pxAppTaskHigherPrioListItem != pxAppTasksListItem );

			xW = xW + pxTask->xWcet;

			if( xT == xW )
			{
				break;
			}
			else
			{
				xT = xW;
			}
		}

		if( xT > pxTask->xDeadline )
		{
			return pdFALSE;
		}

		pxTask->xWcrt = xT;

		pxAppTasksListItem = listGET_NEXT( pxAppTasksListItem );
	}

    return pdTRUE;
}
