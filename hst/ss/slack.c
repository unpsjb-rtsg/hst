#include "FreeRTOS.h"
#include "scheduler.h"
#include "scheduler_logic.h" // pxBigTaskList
#include "slack.h"
#include "utils.h"

static inline TickType_t xSlackGetWorkLoad( struct TaskInfo * pxTask, const TickType_t xTc );
static inline BaseType_t prvSlackCalcSlack( struct TaskInfo * pxTask, const TickType_t xTc, const TickType_t xT, const TickType_t xWc );

BaseType_t xAvailableSlack = 0;

void vSlackCalculateSlack_fixed1( struct TaskInfo * pxTask, const TickType_t xTc )
{
	ListItem_t const *pxTaskListEndItem = listGET_END_MARKER( pxAllTasksList );  // end marker
    ListItem_t *pxTaskListItem = &( pxTask->xGenericListItem );		            // this task list item
    ListItem_t *pxHigherPrioTaskListItem = pxTaskListItem->pxPrevious;	        // higher priority task item

    struct TaskInfo_Slack * pxTaskSlack = ( struct TaskInfo_Slack * ) pxTask->vExt;

    TickType_t xXi = ( TickType_t ) 0U;
    TickType_t xDi = pxTask->xDeadline;

    if ( xTc > ( TickType_t ) 0U )
    {
    	xXi = pxTask->xRelease;
    	xDi = xXi + pxTask->xDeadline;
    }

    pxTaskSlack->xDi = xDi;

    // if xTask is the highest priority task
    if( pxTaskListEndItem == pxHigherPrioTaskListItem )
    {
    	pxTaskSlack->xSlack = xDi - xTc - pxTask->xWcet;
    	pxTaskSlack->xTtma = xDi;
    	return;
    }

    BaseType_t xKmax = 0U;
    BaseType_t xTmax = portMAX_DELAY;

    // TCB of the higher priority task.
    struct TaskInfo * pxHigherPrioTask = ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxHigherPrioTaskListItem );
    struct TaskInfo_Slack * pxHigherPrioTaskSlack = ( struct TaskInfo_Slack * ) pxHigherPrioTask->vExt;

    // Corollary 2 (follows theorem 5)
    if ( ( pxHigherPrioTaskSlack->xDi + pxHigherPrioTask->xWcet >= xDi ) && ( xDi >= pxHigherPrioTaskSlack->xTtma ) )
    {
        pxTaskSlack->xSlack = pxHigherPrioTaskSlack->xSlack - pxTask->xWcet;
        pxTaskSlack->xTtma = pxHigherPrioTaskSlack->xTtma;
        return;
    }

    // Theorem 3
    TickType_t xIntervalo = xXi + pxTask->xDeadline - pxTask->xWcrt + pxTask->xWcet;

    // Corollary 1 (follows theorem 4)
    if ( ( pxHigherPrioTaskSlack->xDi + pxHigherPrioTask->xWcet >= xIntervalo ) && ( pxHigherPrioTaskSlack->xDi + pxHigherPrioTask->xWcet <= xDi ) )
    {
    	xIntervalo = pxHigherPrioTaskSlack->xDi + pxHigherPrioTask->xWcet;
    	xKmax = pxHigherPrioTaskSlack->xSlack - pxTask->xWcet;
    	xTmax = pxHigherPrioTaskSlack->xTtma;
    }

    TickType_t xWc = xSlackGetWorkLoad( pxTask, xTc );

    // Calculate slack at xTask deadline (xDi)
    BaseType_t xK2 = prvSlackCalcSlack( pxTask, xTc, xDi, xWc );

    if ( xK2 >= xKmax )
    {
        if ( xK2 == xKmax )
        {
            if ( xTmax > xDi )
            {
                xTmax = xDi;
            }
        }
        else
        {
            xTmax = xDi;
        }
        xKmax = xK2;
    }

    TickType_t xii;

    // Find the slack in [intervalo, xDi)
    do
    {
    	pxHigherPrioTask = ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxHigherPrioTaskListItem );

    	xii = U_CEIL( xIntervalo, pxHigherPrioTask->xPeriod ) * pxHigherPrioTask->xPeriod;

		while( xii < xDi )
		{
			xK2 = prvSlackCalcSlack( pxTask, xTc, xii, xWc );

			if( xK2 > xKmax )
			{
				xKmax = xK2;
				xTmax = xii;
			}
			else if( ( xK2 == xKmax ) && ( xii < xTmax ) )
			{
				xTmax = xii;
			}

			xii = xii + pxHigherPrioTask->xPeriod;
		}

    	// Get the next higher priority task
		pxHigherPrioTaskListItem = pxHigherPrioTaskListItem->pxPrevious;
    }
    while ( pxTaskListEndItem != pxHigherPrioTaskListItem );

    pxTaskSlack->xSlack = xKmax;
    pxTaskSlack->xTtma = xTmax;
}

inline void vSlackUpdateAvailableSlack( BaseType_t * xAvailableSlack )
{
	const ListItem_t * pxAppTasksListEndMarker = listGET_END_MARKER( pxAllTasksList );
	ListItem_t * pxAppTasksListItem = listGET_HEAD_ENTRY( pxAllTasksList );

	*xAvailableSlack = ( ( struct TaskInfo_Slack * ) ( ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem ) )->vExt )->xSlack;

	while( pxAppTasksListEndMarker != pxAppTasksListItem )
	{
		if( ( ( struct TaskInfo_Slack * ) ( ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem ) )->vExt )->xSlack < *xAvailableSlack )
		{
			*xAvailableSlack = ( ( struct TaskInfo_Slack * ) ( ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem ) )->vExt )->xSlack;
		}

		pxAppTasksListItem = listGET_NEXT( pxAppTasksListItem );
	}
}

inline void vSlackDecrementAllTasksSlack( const TickType_t xTicks )
{
	const ListItem_t * pxAppTasksListEndMarker = listGET_END_MARKER( pxAllTasksList );
	ListItem_t * pxAppTasksListItem = listGET_HEAD_ENTRY( pxAllTasksList );

	while( pxAppTasksListEndMarker != pxAppTasksListItem )
	{
		struct TaskInfo_Slack * tmpTask = ( struct TaskInfo_Slack * ) ( ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem ) )->vExt;

		if( tmpTask->xSlack > 0 )
		{
			tmpTask->xSlack = tmpTask->xSlack - ( BaseType_t ) xTicks;
		}

		pxAppTasksListItem = listGET_NEXT( pxAppTasksListItem );
	}
}

inline void vSlackDecrementTasksSlack( const struct TaskInfo * pxTask, const TickType_t xTicks )
{
	const ListItem_t * pxAppTasksListEndMarker = &( pxTask->xGenericListItem );
	ListItem_t * pxAppTasksListItem = listGET_HEAD_ENTRY( pxAllTasksList );

	while( pxAppTasksListEndMarker != pxAppTasksListItem )
	{
		struct TaskInfo_Slack * tmpTask = ( struct TaskInfo_Slack * ) ( ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem ) )->vExt;

		if( tmpTask->xSlack > 0 )
		{
			tmpTask->xSlack = tmpTask->xSlack - ( BaseType_t ) xTicks;
		}

		pxAppTasksListItem = listGET_NEXT( pxAppTasksListItem );
	}
}

inline void vSlackGainSlack( const struct TaskInfo * pxTask, const TickType_t xTicks )
{
	const ListItem_t * pxAppTasksListEndMarker = listGET_END_MARKER( pxAllTasksList );
	ListItem_t * pxAppTasksListItem = listGET_NEXT( &( pxTask->xGenericListItem ) );

	while( pxAppTasksListItem != pxAppTasksListEndMarker )
	{
		( ( struct TaskInfo_Slack * ) ( ( struct TaskInfo * ) listGET_LIST_ITEM_OWNER( pxAppTasksListItem ) )->vExt )->xSlack += ( BaseType_t ) xTicks;
		pxAppTasksListItem = listGET_NEXT( pxAppTasksListItem );
	}
}

static inline TickType_t xSlackGetWorkLoad( struct TaskInfo * pxTask, const TickType_t xTc )
{
    TickType_t xW = ( TickType_t ) 0U;	// Workload
	TickType_t xA = ( TickType_t ) 0U;
	TickType_t xC = ( TickType_t ) 0U;

	const ListItem_t * pxAppTasksListEndMarker = listGET_END_MARKER( pxAllTasksList );
	ListItem_t * pxAppTasksListItem = &( pxTask->xGenericListItem );

	// Until we process all the maximum priority tasks (including pxTask)
	while( pxAppTasksListEndMarker != pxAppTasksListItem )
	{
		struct TaskInfo * pxHighPrioTask = listGET_LIST_ITEM_OWNER( pxAppTasksListItem );

		// The number of instances of pxHigherPrioTask in [0, xT)
		xA = U_FLOOR( xTc, pxHighPrioTask->xPeriod );

		if( xTc > ( TickType_t ) 0U )
		{
			if( pxHighPrioTask->xWcet == pxHighPrioTask->xCur )
			{
				if( xA >= pxHighPrioTask->uxReleaseCount )
				{
					xC = ( TickType_t ) 0U;
				}
				else
				{
					xC = pxHighPrioTask->xWcet;
				}
			}
			else
			{
				xC = pxHighPrioTask->xWcet;
			}
		}

		// Accumulated workload
		xW = xW + ( xA * pxHighPrioTask->xWcet ) + xC;

		pxAppTasksListItem = pxAppTasksListItem->pxPrevious;
	}

    return xW;
}

static inline BaseType_t prvSlackCalcSlack( struct TaskInfo * pxTask, const TickType_t xTc, const TickType_t xT, const TickType_t xWc )
{
    const ListItem_t * pxAppTasksListEndMarker = listGET_END_MARKER( pxAllTasksList );
    ListItem_t * pxAppTasksListItem = &( pxTask->xGenericListItem );

    TickType_t xW = 0;

    while( pxAppTasksListEndMarker != pxAppTasksListItem )
    {
    	struct TaskInfo * pxHighPrioTask = listGET_LIST_ITEM_OWNER( pxAppTasksListItem );

    	/* Accumulated workload of higher priority tasks in [0, xT) */
    	xW = xW + ( U_CEIL( xT, pxHighPrioTask->xPeriod ) * pxHighPrioTask->xWcet );

    	pxAppTasksListItem = pxAppTasksListItem->pxPrevious;
    }

    return ( BaseType_t ) xT - ( BaseType_t ) xTc - ( BaseType_t ) xW + ( BaseType_t ) xWc;
}
