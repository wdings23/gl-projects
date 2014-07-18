#include "timeutil.h"

/*
**
*/
double getTime( void )
{
    return ( [[NSDate date] timeIntervalSince1970] * 1000.0 );
}