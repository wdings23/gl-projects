//
//  filepathutil.cpp
//  CityBenchmark
//
//  Created by Dingwings on 7/24/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "filepathutil.h"

#import <Foundation/Foundation.h>

void getFullPath( char* szFilePath, const char* szFileName )
{
    NSString* pPath = [[NSBundle mainBundle] bundlePath];
    const char* pszFullPath = [pPath UTF8String];
    
    sprintf( szFilePath, "%s/%s", pszFullPath, szFileName );
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