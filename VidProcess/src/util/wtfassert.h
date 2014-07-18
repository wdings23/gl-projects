//
//  wtfassert.h
//  Game1
//
//  Created by Dingwings on 1/21/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef __WTFASSERT_H__
#define __WTFASSERT_H__

#if defined( FINAL ) || defined( RELEASE )

#define WTFASSERT2( X, Y, ... ) \

#else
#define WTFASSERT2( X, ... ) wtfAssert2( (long)(X), #X, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__ )
#endif // FINAL

#ifdef __cplusplus 
extern "C" {
#endif // __cplusplus

void wtfAssert2( long bStatement, const char* szStatement, const char* szFile, const char* szFunc, int iLine, const char* szFormat, ... );

void wtfAssertAbort( void );
void wtfAssertContinue( void );
void wtfAssertSkip( void );
void wtfAssertInit( void );
    
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __WTFASSERT_H__
