//
//  parseutil.cpp
//  Game1
//
//  Created by Dingwings on 2/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "parseutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
**
*/
void parseVector( tVector4* pResult, const char* szValue )
{
    const char* szStart = szValue;
    const char* szEnd = strstr( szStart, "," );
    
    char szVal[64];
    memset( szVal, 0, sizeof( szVal ) );
    memcpy( szVal, szStart, (unsigned long)szEnd - (unsigned long)szStart );
    pResult->fX = (float)atof( szVal );
    
    szStart = (char *)( szEnd + 1 );
    szEnd = strstr( szStart, "," );
    
    memset( szVal, 0, sizeof( szVal ) );
    memcpy( szVal, szStart, (unsigned long)szEnd - (unsigned long)szStart );
    pResult->fY = (float)atof( szVal );
    
    if( szEnd )
    {
        szStart = (char *)( szEnd + 1 );
        szEnd = strstr( szStart, "," );
        
        if( szEnd )
        {
            memset( szVal, 0, sizeof( szVal ) );
			memcpy( szVal, szStart, (unsigned long)szEnd - (unsigned long)szStart );
			pResult->fZ = (float)atof( szVal );
            
            szStart = (char *)( szEnd + 1 );
            strncpy( szVal, szStart, sizeof( szVal ) );
            pResult->fW = (float)atof( szVal );
        }
        else
        {
            size_t iStrLen = strlen( szValue );
			size_t iLeftOver = iStrLen - ( (size_t)szStart - (size_t)szValue );
			memset( szVal, 0, sizeof( szVal ) );
			memcpy( szVal, szStart, iLeftOver );

            pResult->fZ = (float)atof( szVal );
            pResult->fW = 1.0f;
        }
    }
    
}

/*
**
*/
void parseIntegerArray( int** aiArray, int* iNumEntries, const char* szValue )
{
    // count the number of entries;
    int iSize = 0;
    const char* szStart = szValue;
    const char* szEnd = strstr( szStart, "," );
    for( iSize = 0;; iSize++ )
    {
        if( szEnd == NULL )
        {
            ++iSize;
            break;
        }
        
        szStart = (char *)( (unsigned long)szEnd + 1 );
        szEnd = strstr( szStart, "," );
    }
    
    *iNumEntries = iSize;
    *aiArray = (int *)malloc( sizeof( int ) * iSize );
    
    // copy numbers
    szStart = szValue;
    szEnd = strstr( szStart, "," );
    for( int i = 0; i < iSize; i++ )
    {
        char szVal[16];
        if( szEnd == NULL )
        {
            szEnd = (const char *)( (unsigned long)szStart + strlen( szStart ) );
        }
        
        memset( szVal, 0, sizeof( szVal ) );
        memcpy( szVal, szStart, (unsigned int)( szEnd - szStart ) );
        (*aiArray)[i] = (int)atoi( szVal );
        
        if( szEnd )
        {
            szStart = (char *)( (unsigned long)szEnd + 1 );
            szEnd = strstr( szStart, "," );
        }
    }
}

/*
**
*/
void parseFloatArray( float** afArray, int* iNumEntries, const char* szValue )
{
    // count the number of entries;
    int iSize = 0;
    const char* szStart = szValue;
    const char* szEnd = strstr( szStart, "," );
    for( iSize = 0;; iSize++ )
    {
        if( szEnd == NULL )
        {
            ++iSize;
            break;
        }
        
        szStart = (char *)( (unsigned long)szEnd + 1 );
        szEnd = strstr( szStart, "," );
    }
    
    *iNumEntries = iSize;
    *afArray = (float *)malloc( sizeof( float ) * iSize );
    
    // copy numbers
    szStart = szValue;
    szEnd = strstr( szStart, "," );
    for( int i = 0; i < iSize; i++ )
    {
        char szVal[128];
        if( szEnd == NULL )
        {
            szEnd = (const char *)( (unsigned long)szStart + strlen( szStart ) );
        }
        
        memset( szVal, 0, sizeof( szVal ) );
        memcpy( szVal, szStart, (unsigned int)( szEnd - szStart ) );
        (*afArray)[i] = (float)atof( szVal );
        
        if( szEnd )
        {
            szStart = (char *)( (unsigned long)szEnd + 1 );
            szEnd = strstr( szStart, "," );
        }
    }
}
