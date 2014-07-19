#include "timeutil.h"
#include "Foundation/NSDate.h"

/*
**
*/
double getTime( void )
{
    return ( [[NSDate date] timeIntervalSince1970] * 1000.0 );
}