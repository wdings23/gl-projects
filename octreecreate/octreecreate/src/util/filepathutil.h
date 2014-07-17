//
//  filepathutil.h
//  CityBenchmark
//
//  Created by Dingwings on 7/24/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef __FILEPATHUTIL_H__
#define __FILEPATHUTIL_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void getFullPath( char* szFilePath, const char* szFileName );
void getWritePath( char* szFilePath, const char* szFileName );
    
#ifdef __cplusplus
}
#endif // __cplusplus
    
#endif // __FILEPATHUTIL_H__