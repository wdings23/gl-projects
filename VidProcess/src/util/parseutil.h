//
//  parseutil.h
//  Game1
//
//  Created by Dingwings on 2/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef __PARSEUTIL_H__
#define __PARSEUTIL_H__

#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void parseVector( tVector4* pResult, const char* szValue );
void parseIntegerArray( int** aiArray, int* iNumEntries, const char* szValue );
void parseFloatArray( float** afArray, int* iNumEntries, const char* szValue );

#ifdef __cplusplus
}
#endif // __cplusplus
    
#endif // __PARSEUTIL_H__
