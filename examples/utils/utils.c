#include "utils.h"

#define ONE_TICK ( ( configCPU_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL )

void vUtilsEatCpu( UBaseType_t ticks )
{
	BaseType_t xI;

	BaseType_t xLim = ( ticks * ONE_TICK ) / 20;

	for( xI = 0; xI < xLim; xI++ )
	{			
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
	}
}
