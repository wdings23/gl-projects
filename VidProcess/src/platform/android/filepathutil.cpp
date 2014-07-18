/*
 * =====================================================================================
 *
 *       Filename:  fileutil.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/29/2013 12:40:33
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "filepathutil.h"

#include <string.h>
#if defined( WIN32 )
#include <direct.h>
#endif // WIN32
#include <dirent.h>

#if defined( ANDROID ) || defined( MACOS ) || defined( IOS )
#include <sys/stat.h>
#endif // ANDROID

static char sszDirectory[256];
static char sszWriteDirectory[256];

/*
**
*/
void getFullPath( char* szFullPath, const char* szFileName )
{
   snprintf( szFullPath, 256, "%s/%s", sszDirectory, szFileName );
}

/*
**
*/
void setFileDirectories( const char* szDirectory, const char* szWriteDirectory )
{
    strncpy( sszDirectory, szDirectory, sizeof( sszDirectory ) );
    OUTPUT( "!!! sszDirectory = %s !!!", sszDirectory );

	strncpy( sszWriteDirectory, szWriteDirectory, sizeof( sszWriteDirectory ) );
    OUTPUT( "!!! sszWriteDirectory = %s !!!", sszWriteDirectory );
}

/*
**
*/
void getWritePath( char* szFullPath, const char* szFileName )
{
    snprintf( szFullPath, 256, "%s/%s", sszWriteDirectory, szFileName );
}

/*
**
*/
void getWritePathWithDirectory( char* szFilePath, const char* szDirectory, const char* szFileName )
{
	snprintf( szFilePath, 256, "%s/%s/%s", sszWriteDirectory, szDirectory, szFileName );
}

/*
**
*/
void createRootWriteDirectory( void )
{
    char szFilePath[512];
	snprintf( szFilePath, sizeof( szFilePath ), "%s", sszWriteDirectory );
	
#if defined( WIN32 )
    _mkdir( szFilePath );
#else
    mkdir( szFilePath, S_IRWXU );
#endif // WIN32
}

/*
**
*/
void createDirectory( const char* szDirectory )
{
	char szFilePath[512];
	snprintf( szFilePath, sizeof( szFilePath ), "%s/%s", sszWriteDirectory, szDirectory );

#if defined( WIN32 )
    _mkdir( szFilePath );
#else
    mkdir( szFilePath, S_IRWXU );
#endif // WIN32

}

/*
**
*/
void getAllFilesInDirectory( char const* szDirectory,
                             std::vector<std::string>& aFileNames,
                             const char* szExtension )
{
	aFileNames.clear();

	char szWritePath[512];
	snprintf( szWritePath, sizeof( szWritePath ), "%s/%s/", sszWriteDirectory, szDirectory );
	

	DIR* pDir;
	struct dirent* pEnt;
	if( ( pDir = opendir( szWritePath ) ) != NULL ) 
	{
		while( ( pEnt = readdir( pDir ) ) != NULL ) 
		{
			std::string fileName( pEnt->d_name );
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

	closedir( pDir );
}

/*
**
*/
const char* getWriteDirectory( void )
{
	return sszWriteDirectory;
}

/*
**
*/
void deleteAllFilesInDirectory( const char* szDirectory )
{
	std::vector<std::string> aFileNames;
	getAllFilesInDirectory( szDirectory, aFileNames );

	char szFullPath[512];

	std::vector<std::string>::iterator iter = aFileNames.begin();
	for( ; iter != aFileNames.end(); ++iter )
	{
		const char* szFileName = (*iter).c_str();
		
		if( *iter != "." && *iter != ".." )
		{
			snprintf( szFullPath, sizeof( szFullPath ), "%s/%s/%s", 
					  getWriteDirectory(), 
					  szDirectory, 
					  szFileName );
			remove( szFullPath );
		}

	}	// for iter = files names begin to end
}

/*
**
*/
void deleteDirectory( const char* szDirectory )
{
	std::vector<std::string> aFileNames;
	getAllFilesInDirectory( szDirectory, aFileNames );

	char szFullPath[512];

	std::vector<std::string>::iterator iter = aFileNames.begin();
	for( ; iter != aFileNames.end(); ++iter )
	{
		const char* szFileName = (*iter).c_str();
		
		if( *iter != "." && *iter != ".." )
		{
			snprintf( szFullPath, sizeof( szFullPath ), "%s/%s/%s", 
					  getWriteDirectory(), 
					  szDirectory, 
					  szFileName );
			remove( szFullPath );
		}

	}	// for iter = files names begin to end

	snprintf( szFullPath, sizeof( szFullPath ), "%s/%s", 
					  getWriteDirectory(), 
					  szDirectory );
	
	rmdir( szFullPath );
}

/*
**
*/
void renameDirectory( const char* szOrig, const char* szNew )
{
	char szOrigPath[256];
	snprintf( szOrigPath, sizeof( szOrigPath ), "%s/%s", getWriteDirectory(), szOrig );

	char szNewPath[256];
	snprintf( szNewPath, sizeof( szNewPath ), "%s/%s", getWriteDirectory(), szNew );

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

	FREE( aTempBuffer );

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