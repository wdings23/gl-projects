//
//  memutil.cpp
//  animtest
//
//  Created by Tony Peng on 7/26/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "memutil.h"
#include <string.h>
#include <stdarg.h>

/*
**
*/
void neonmemcpy( void* pDest, void* pSrc, size_t iSize )
{
#if defined( NEON )
    
    // copies 16 32-bit values at once (16x4 bytes) = 64 bytes
    size_t iRemaining = iSize & 0x3F;
    
    asm volatile(
                 "mov        r0, %2                              \n\t"
                 "startcopy:                                     \n\t"
                 "pld        [%1, #0xC0]                         \n\t"      // preload memory to cache
                 "subs       r0, r0, #0x40                       \n\t"
                 "vldm       %1!, {q0-q3}                        \n\t"      // load src
                 "vstm       %0!, {q0-q3}                        \n\t"      // store dest
                 "bgt        startcopy                           \n\t"
                 
                 :
                 : "r"(pDest), "r"(pSrc), "r"(iSize)
                 : "q0", "q1", "q2", "q3", "cc", "memory"
                 );
    
    size_t iCopiedSize = iSize - iRemaining;
    void* pRemainSrc = (void *)( (unsigned int)pSrc + iCopiedSize );
    void* pRemainDest = (void *)( (unsigned int)pDest + iCopiedSize );
    memcpy(pRemainDest, pRemainSrc, iRemaining);
#else
    
    memcpy( pDest, pSrc, iSize );
#endif // __ARM_NEON__
}


/*
**
*/
void outputLog( const char* szFormat, ... )
{
	char szString[1024];

	va_list args;
	va_start( args, szFormat );
	vsprintf( szString, szFormat, args );
	va_end( args );

	FILE* fp = fopen( "C:\\memlog.txt", "ab" );
	fprintf( fp, "%s", szString );
	fclose( fp );
}

/*
**
*/
void debugFree( const char* szName, void* pData )
{
	outputLog( "FREE %s AT 0x%08X\n", szName, (unsigned long)pData );
	free( pData );
}

/*
**
*/
void* debugRealloc( const char* szName, void* pData, size_t iSize )
{
	void* pNewData = realloc( pData, iSize );

	outputLog( "REALLOC %s ORIG: 0x%08X NEW: 0x%08X SIZE %d\n",
			  szName,
			  (unsigned long)pData,
			  (unsigned long)pNewData,
			  iSize );

	return pNewData;
}

/*
**
*/
void* debugMalloc( const char* szName, size_t iSize )
{
	void* pData = malloc( iSize );
	outputLog( "MALLOC %s AT 0x%08X\n", szName, (unsigned long)pData );

	return pData;
}