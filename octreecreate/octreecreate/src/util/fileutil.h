//
//  fileutil.h
//  animtest
//
//  Created by Tony Peng on 6/24/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#ifndef __FILEUTIL_H__
#define __FILEUTIL_H__

//#define _MAX_PATH 256

#if __cplusplus
extern "C" {
#endif // __cplusplus
    
void getFullPath( char* szFullPath, const char* szFileName );
void setFileDirectory( const char* szDirectory );
    
void getWritePath( char* szFilePath, const char* szFileName );
    
#ifdef __cplusplus
}
#endif // __cplusplus
    
#endif // __FILEUTIL_H__
