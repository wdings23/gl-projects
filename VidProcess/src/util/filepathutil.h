//
//  filepathutil.h
//  CityBenchmark
//
//  Created by Dingwings on 7/24/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef __FILEPATHUTIL_H__
#define __FILEPATHUTIL_H__

#include <vector>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void setFileDirectories( const char* szDirectory, const char* szWriteDirectory );
void getFullPath( char* szFilePath, const char* szFileName );
void getWritePath( char* szFilePath, const char* szFileName );
void getWritePathWithDirectory( char* szFilePath, const char* szDirectory, const char* szFileName );
const char* getWriteDirectory( void );

void createRootWriteDirectory( void );
void createDirectory( const char* szDirectory );
void deleteAllFilesInDirectory( const char* szDirectory );
void deleteDirectory( const char* szDirectory );
void renameDirectory( const char* szOrig, const char* szNew );

void copyFile( const char* szSrc, 
			   const char* szDestDirectory );

void getFileName( char* szFileName, const char* szFullPath, size_t iFileNameSize );

#ifdef __cplusplus
}
#endif // __cplusplus

void getAllFilesInDirectory( char const* szDirectory,
                             std::vector<std::string>& aFileNames,
                             char const* szExtension = NULL );
    
#endif // __FILEPATHUTIL_H__