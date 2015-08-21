#include "FreeRTOS.h"

#define U_CEIL( x, y )    ( ( x / y ) + ( x % y != 0 ) )
#define U_FLOOR( x, y )   ( x / y )

#if defined (__cplusplus)
extern "C" {
#endif

void vUtilsEatCpu( UBaseType_t ticks );

#if defined (__cplusplus)
}
#endif
