//
//  wtfassert.cpp
//  Game1
//
//  Created by Dingwings on 1/21/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "wtfassert.h"
#include "filepathutil.h"

static bool sbShowingAlert = false;

/*
**
*/
void wtfAssert2( long bStatement, const char* szStatement, const char* szFile, const char* szFunc, int iLine, const char* szFormat, ... )
{
	if( bStatement == 0 )
	{
		va_list args;
		char szLine[512];
		
		// user input print
		va_start( args, szFormat );
		vsprintf( szLine, szFormat, args );
		va_end( args );
		
		char szString[1024];
		sprintf( szString, "ASSERT: %s\n%s\n%s\nLine %d\nERROR: \"%s\"", szStatement, szFile, szFunc, iLine, szLine );
		
        /*showAlert( szString, "OK" );
        sbShowingAlert = true;
        
        while( sbShowingAlert )
        {
            [NSThread sleepForTimeInterval:.5];
        }*/
        
        OUTPUT( "%s", szString );
        
		char szFullPath[256];
		getWritePath( szFullPath, "log.txt" );
		FILE* fp = fopen( szFullPath, "a" );
		
		fprintf( fp, "%s", szString );
		fclose( fp );
        
#if defined( _SIMULATOR )
		//__asm { int 3 };
		abort();
#else		
		abort();//__asm__ volatile( "int3" );
#endif // _SIMULATOR
	}
}
