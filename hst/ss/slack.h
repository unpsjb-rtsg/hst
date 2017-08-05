#include "FreeRTOS.h"

#if defined (__cplusplus)
extern "C" {
#endif

#define USE_SLACK 	1
#define USE_SLACK_K 0

extern BaseType_t xAvailableSlack;

struct TaskInfo_Slack {
	TickType_t xDi;
	BaseType_t xSlack;
	TickType_t xTtma;
	TickType_t xK;
};

typedef struct TaskInfo_Slack TaskSs_t;

/**
 *
 * @param pxTask
 * @param xTc
 */
void vSlackCalculateSlack_fixed1( HstTCB_t * pxTask, const TickType_t xTc );

/**
 *
 * @param pxTask
 * @param xTicks
 */
void vSlackGainSlack( const HstTCB_t * pxTask, const TickType_t xTicks ); //  __attribute__((always_inline));

/**
 *
 * @param pxTask
 * @param xTicks
 */
void vSlackDecrementTasksSlack( const HstTCB_t * pxTask, const TickType_t xTicks );

/**
 *
 * @param xTicks
 */
void vSlackDecrementAllTasksSlack( const TickType_t xTicks );

/**
 *
 * @param xAvailableSlack
 */
void vSlackUpdateAvailableSlack( BaseType_t * xAvailableSlack );

/**
 *
 * @return
 */
BaseType_t vSlackCalculateTasksWcrt( void );

#if defined (__cplusplus)
}
#endif
