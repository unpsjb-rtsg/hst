#include "wcrt.h"
#include "scheduler.h"
#include "scheduler_logic.h"  // pxAllTasksList
#include "utils.h"            // U_CEIL, U_FLOOR

/**
 * RTA
 * "Improved Response-Time Analysis Calculations"
 * http://doi.ieeecomputersociety.org/10.1109/REAL.1998.739773
 */
BaseType_t xWcrtCalculateTasksWcrt( void )
{
	TickType_t xW = 0U;

	ListItem_t * pxAppTasksListItem = listGET_HEAD_ENTRY( pxAllTasksList );

	HstTCB_t *pxTask = ( HstTCB_t * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem );

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
    while( listGET_END_MARKER( pxAllTasksList ) != pxAppTasksListItem )
	{
    	pxTask = ( HstTCB_t * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem );

		xT = xT + pxTask->xWcet;

		while( xT <= pxTask->xDeadline )
		{
			xW = 0;

			/* Calculates the workload of the higher priority tasks than pxTask. */
			ListItem_t * pxAppTaskHigherPrioListItem = listGET_HEAD_ENTRY( pxAllTasksList );
			do
			{
				HstTCB_t *pxHigherPrioTask = ( HstTCB_t * ) listGET_LIST_ITEM_OWNER( pxAppTaskHigherPrioListItem );

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
