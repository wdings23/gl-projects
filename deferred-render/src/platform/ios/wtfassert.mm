    //
//  wtfassert.cpp
//  Game1
//
//  Created by Dingwings on 1/21/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "wtfassert.h"
#include "filepathutil.h"

#include "hashutil.h"

#include <vector>

static bool sbShowingAlert = false;
static bool sbAbortProcess = false;

std::vector<int> saToSkip;
static int siHash = 0;

/*
 **
 */
void wtfAssert2( long bStatement, const char* szStatement, const char* szFile, const char* szFunc, int iLine, const char* szFormat, ... )
{
	if( bStatement == 0 )
	{
        siHash = hash( szStatement );
        for( int i = 0; i < saToSkip.size(); i++ )
        {
            if( saToSkip[i] == siHash )
            {
                return;
            }
        }
        
		va_list args;
		char szLine[512];
		
		// user input print
		va_start( args, szFormat );
		vsprintf( szLine, szFormat, args );
		va_end( args );
		
		char szString[1024];
		sprintf( szString, "ASSERT: %s\n%s\n%s\nLine %d\nERROR: \"%s\"", szStatement, szFile, szFunc, iLine, szLine );
		
        OUTPUT( "%s", szString );
        
		char szFullPath[256];
		getWritePath( szFullPath, "log.txt" );
		FILE* fp = fopen( szFullPath, "a" );
		
		fprintf( fp, "%s", szString );
		fclose( fp );
        
        //NSString* outputString = [[[NSString alloc] initWithUTF8String:szString] autorelease];
        //UIAlertView* pAlert = [[UIAlertView alloc] initWithTitle:@"ASSERT" message:outputString delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
        //[pAlert release];
        
        sbShowingAlert = true;
        while( sbShowingAlert )
        {
            [NSThread sleepForTimeInterval:0.5];
            if( sbAbortProcess )
            {
                break;
            }
        }
        
        if( sbAbortProcess )
        {
            abort();
        }
        /*
         #if defined( _SIMULATOR )
//         //__asm { int 3 };
         abort();
         #else		
         __asm__ volatile( "int3" );
         #endif // _SIMULATOR
         */
	}
}

/*
**
*/
void wtfAssertAbort( void )
{
    sbAbortProcess = true;
}

/*
**
*/
void wtfAssertContinue( void )
{
    sbShowingAlert = false;
}

/*
**
*/
void wtfAssertSkip( void )
{
    saToSkip.push_back( siHash );
    sbShowingAlert = false;
}

/*
**
*/
void wtfAssertInit( void )
{
    saToSkip.clear();
}
