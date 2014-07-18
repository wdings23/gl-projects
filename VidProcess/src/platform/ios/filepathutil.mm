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

/*
**
*/
void getWritePathWithDirectory( char* szFilePath, const char* szDirectory, const char* szFileName )
{
    NSArray* aPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES );
    NSString* docDirectory = [aPaths objectAtIndex:0];
    
    NSString* directoryName = [NSString stringWithUTF8String:szDirectory];
    
    // full path
    NSString* fullPath = [docDirectory stringByAppendingPathComponent:directoryName];
    
    const char* szDir = (const char *)[fullPath cStringUsingEncoding:NSASCIIStringEncoding];
    sprintf( szFilePath, "%s/%s", szDir, szFileName );
}

/*
**
*/
void createRootWriteDirectory( void )
{
    // get Documents folder
    NSArray* aPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES );
    NSString* docDirectory = [aPaths objectAtIndex:0];
    
    // directory
    NSString* directoryName = [NSString stringWithUTF8String:"save"];
    
    // full path
    NSString* fullPath = [docDirectory stringByAppendingPathComponent:directoryName];
    
    NSError* error = nil;
    [[NSFileManager defaultManager] createDirectoryAtPath:fullPath
                              withIntermediateDirectories:NO
                                               attributes:nil error:&error];
}

/*
**
*/
void createDirectory( const char* szDirectory )
{
    // get Documents folder
    NSArray* aPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES );
    NSString* docDirectory = [aPaths objectAtIndex:0];
    
    // directory
    NSString* directoryName = [NSString stringWithUTF8String:szDirectory];
    
    // full path
    NSString* fullPath = [docDirectory stringByAppendingPathComponent:directoryName];
    
    NSError* error = nil;
    [[NSFileManager defaultManager] createDirectoryAtPath:fullPath
                              withIntermediateDirectories:NO
                                               attributes:nil error:&error];
    
}

/*
**
*/
void getAllFilesInDirectory( char const* szDirectory,
                             std::vector<std::string>& aFileNames,
                             char const* szExtension )
{
    // get Documents folder
    NSArray* aPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES );
    NSString* docDirectory = [aPaths objectAtIndex:0];
    
    // directory
    NSString* directoryName = [NSString stringWithUTF8String:szDirectory];
    
    // full path
    NSString* fullPath = [docDirectory stringByAppendingPathComponent:directoryName];
    
    // get the contents
    NSArray *directoryContent = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:fullPath error:NULL];
    for( int i = 0; i < [directoryContent count]; i++ )
    {
        const char* szFileName = [[directoryContent objectAtIndex:i] cStringUsingEncoding:NSASCIIStringEncoding];
        std::string fileName( szFileName );
        
        if( fileName != "." && fileName != ".." )
        {
            if( szExtension )
            {
                char szTotalExtension[16];
                snprintf( szTotalExtension, sizeof( szTotalExtension ), ".%s", szExtension );
                if( fileName.find( szTotalExtension ) != std::string::npos )
                {
                    aFileNames.push_back( fileName );
                }
            }
            else
            {
                aFileNames.push_back( fileName );
            }
        }
    }
}

/*
**
*/
void deleteAllFilesInDirectory( const char* szDirectory )
{
    char szFilePath[256];
    
    std::vector<std::string> aFileNames;
    getAllFilesInDirectory( szDirectory, aFileNames );
    
    std::vector<std::string>::iterator iter = aFileNames.begin();
    for( ; iter != aFileNames.end(); ++iter )
    {
        std::string fileName = *iter;
        
        getWritePathWithDirectory( szFilePath, szDirectory, fileName.c_str() );
        NSString* filePath = [NSString stringWithUTF8String:szFilePath];
        
        NSError* error;
        [[NSFileManager defaultManager] removeItemAtPath:filePath error:&error];
    }
    
}

/*
**
*/
const char* getWriteDirectory( void )
{
    NSArray* aPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES );
    NSString* docDirectory = [aPaths objectAtIndex:0];
    
    const char* szDir = (const char *)[docDirectory cStringUsingEncoding:NSASCIIStringEncoding];
    return szDir;
}

/*
**
*/
void deleteDirectory( const char* szDirectory )
{
    NSString* filePath = [NSString stringWithUTF8String:szDirectory];
    
    NSError* error;
    [[NSFileManager defaultManager] removeItemAtPath:filePath error:&error];
}

/*
**
*/
void renameDirectory( const char* szOrig, const char* szNew )
{
    char szOrigPath[256];
	snprintf( szOrigPath, sizeof( szOrigPath ), "%s//%s", getWriteDirectory(), szOrig );
    
	char szNewPath[256];
	snprintf( szNewPath, sizeof( szNewPath ), "%s//%s", getWriteDirectory(), szNew );
    
	rename( szOrigPath, szNewPath );
}

/*
**
*/
void copyFile( const char* szSrc,
              const char* szDestDirectory )
{
    
	createDirectory( szDestDirectory );
    
    FILE* pSrc = fopen( szSrc, "rb" );
	WTFASSERT2( pSrc, "can't open %s", szSrc );
    
	char szFileName[256];
	getFileName( szFileName, szSrc, sizeof( szFileName ) );
    
	char szDest[512];
	memset( szDest, 0, sizeof( szDest ) );
	snprintf( szDest, sizeof( szDest ), "%s/%s", szDestDirectory, szFileName );
    
	FILE* pDest = fopen( szDest, "wb" );
	WTFASSERT2( pDest, "can't save to %s", szDest );
    
	int iTempSize = ( 1 << 16 );
	char* aTempBuffer = (char *)MALLOC( iTempSize );
	
	fseek( pSrc, 0, SEEK_END );
	long iFileSize = ftell( pSrc );
	fseek( pSrc, 0, SEEK_SET );
    
	// copy chunks of memory
	int iNumChunks = (int)( iFileSize / (long)iTempSize );
	int iLeftOver = ( iFileSize % iTempSize );
	for( int i = 0; i < iNumChunks; i++ )
	{
		fread( aTempBuffer, sizeof( char ), iTempSize, pSrc );
		fwrite( aTempBuffer, sizeof( char ), iTempSize, pDest );
		fseek( pDest, 0, SEEK_END );
	}
    
	if( iLeftOver > 0 )
	{
		fread( aTempBuffer, sizeof( char ), iLeftOver, pSrc );
		fwrite( aTempBuffer, sizeof( char ), iLeftOver, pDest );
	}
    
	free( aTempBuffer );
    
	fclose( pSrc );
	fclose( pDest );
}

/*
**
*/
void getFileName( char* szFileName, const char* szFullPath, size_t iFileNameSize )
{
	memset( szFileName, 0, iFileNameSize );
    
	int iLength = (int)strlen( szFullPath );
	for( int i = iLength - 1; i >= 0; i-- )
	{
		if( szFullPath[i] == '\\' ||
           szFullPath[i] == '/' )
		{
			memcpy( szFileName, &szFullPath[i+1], iLength - i - 1 );
			break;
		}
        
	}	// for i = length - 1 to 0
}