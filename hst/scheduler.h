#include "task.h"

#define TASK_SCHEDULER_PRIORITY ( configMAX_PRIORITIES - 1 )
#define TASK_PRIORITY 			( configMAX_PRIORITIES - 2 )

/* Task types. */
typedef enum {
	HST_PERIODIC,
	HST_APERIODIC,
	HST_SPORADIC,
	HST_NONE
} HstTaskType_t;

/* Periodic task info. TCB for application scheduler. */
struct TaskInfo
{
	TaskHandle_t xHandle;	     /* FreeRTOS task reference. */

	// ----------------------
	ListItem_t xGenericListItem;     /* Points to the app scheduled list. */
	ListItem_t xReadyListItem;       /* Points to the scheduler ready list. */
	ListItem_t xAbsDeadlineListItem; /* Points to the HST absolute dealine list. */

	// ----------------------
	BaseType_t xFinished;        /* 0: running. 1: suspended (called vTaskDelayUntil). */

	// ----------------------
	UBaseType_t xPriority;	     /* Priority. */
	TickType_t xPeriod;		     /* Period. */
	TickType_t xDeadline;        /* Relative deadline. */
	TickType_t xAbsolutDeadline; /* Absolute deadline of the current release. */
	TickType_t xRelease;	     /* Most recent task release absolute time. */

	// ----------------------
	TickType_t xWcet;		     /* Worst case execution time. */
	TickType_t xWcrt;		     /* Worst case response time. */

	// ----------------------
	UBaseType_t uxReleaseCount;  /* Release counter. */

	// ----------------------
	TickType_t xCur; 		     /* Current release tick count. */

	// ----------------------
	HstTaskType_t xHstTaskType;  /* Task type. */

	// ----------------------
	void* vExt;                  /* Pointer to a scheduling policy specific structure. */
};

#if defined (__cplusplus)
extern "C" {
#endif

/* Trace blocking and suspended tasks. */
void vSchedulerTaskDelay( void );

void vSchedulerTaskReady( void* pxTask );

void vSchedulerTaskBlock( void* pxResource );

void vSchedulerTaskSuspend( void* pxTask );

/**
 * Application scheduler setup.
 */
void vSchedulerSetup( void );

/**
 * Task scheduler start.
 */
void vSchedulerInit( void );

/**
 * Create a application scheduled task.
 */
BaseType_t xSchedulerTaskCreate( TaskFunction_t pxTaskCode, const char * const pcName, const uint16_t usStackDepth, void * const pvParameters, UBaseType_t uxPriority, struct TaskInfo ** const pxCreatedTask, TickType_t xPeriod, TickType_t xDeadline, TickType_t xWcet );

/**
 * Create a aperiodic application scheduled task.
 */
#define xSchedulerAperiodicTaskCreate( pxTaskCode, pcName, usStackDepth, pvParameters, pxCreatedTask ) xSchedulerTaskCreate( ( pxTaskCode ), ( pcName ), ( usStackDepth ), ( pvParameters ), ( TASK_PRIORITY ), ( pxCreatedTask ), ( 0 ), ( 0 ), ( 0 ) )

/**
 * Suspend the caller task until its next period.
 */
void vSchedulerWaitForNextPeriod( void );


/* --- AppSched_Logic -------------------------------------------------- */

void vSchedulerLogicSetup( void );

void vSchedulerTaskSchedulerStartLogic( void );

void vSchedulerLogicAddTask( struct TaskInfo * xTask );

void vSchedulerLogicAddTaskToReadyList( struct TaskInfo *xTask );

void vSchedulerLogicRemoveTaskFromReadyList( struct TaskInfo *xTask );

void vSchedulerTaskSchedulerLogic( struct TaskInfo **xCurrentTask );

struct TaskInfo* xSchedulerGetTaskTCBe( TaskHandle_t xTask );

BaseType_t vSchedulerTaskSchedulerTickLogic( void );

/* --------------------------------------------------------------------- */

#if defined (__cplusplus)
}
#endif

