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

void vSlackCalculateSlack_fixed1( struct TaskInfo * pxTask, const TickType_t xTc );

void vSlackGainSlack( const struct TaskInfo * pxTask, const TickType_t xTicks ); //  __attribute__((always_inline));
void vSlackDecrementTasksSlack( const struct TaskInfo * pxTask, const TickType_t xTicks );
void vSlackDecrementAllTasksSlack( const TickType_t xTicks );
void vSlackUpdateAvailableSlack( BaseType_t * xAvailableSlack );

BaseType_t vSlackCalculateTasksWcrt( void );

#if defined (__cplusplus)
}
#endif
