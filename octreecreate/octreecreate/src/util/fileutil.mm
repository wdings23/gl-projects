//
//  NSObject+fileutil.m
//  animtest
//
//  Created by Tony Peng on 6/24/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "fileutil.h"

/*
**
*/
void getFullPath( char* szFullPath, const char* szFileName )
{
    NSString* bundlePath = [[NSBundle mainBundle] resourcePath];
    const char* szPath = [bundlePath cStringUsingEncoding:[NSString defaultCStringEncoding]];
    snprintf( szFullPath, _MAX_PATH, "%s/%s", szPath, szFileName );
}

/*
**
*/
void getWritePath( char* szFilePath, const char* szFileName )
{
    NSArray* aPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES );
    NSString* docDirectory = [aPaths objectAtIndex:0];
    
    const char* szDir = (const char *)[docDirectory cStringUsingEncoding:NSASCIIStringEncoding];
    sprintf( szFilePath, "%s/%s", szDir, szFileName );
}