#include "outpututil.h"
#include "stdarg.h"

/*
**
*/
void debugOutput( const char* szFormat, ... )
{												
    char szOutput[1024];
	
	va_list args;								
	va_start( args, szFormat );					
	vsprintf( szOutput, szFormat, args );		
	va_end( args );

#if defined( WIN32 )
	OutputDebugStringA( szOutput );
#else
    printf( "%s\n", szOutput );
#endif // WIN32
    
}