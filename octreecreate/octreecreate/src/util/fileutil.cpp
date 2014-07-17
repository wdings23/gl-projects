#include "fileutil.h"

#include <stdio.h>
#include <string.h>

static char sszDirectory[256];

/*
**
*/
void getFullPath( char* szFullPath, const char* szFileName )
{
   sprintf_s( szFullPath, 256, "%s/%s", sszDirectory, szFileName );
}

/*
**
*/
void setFileDirectory( const char* szDirectory )
{
    strncpy_s( sszDirectory, szDirectory, sizeof( sszDirectory ) );
    printf( "!!! sszDirectory = %s !!!", sszDirectory );
}

/*
**
*/
void getWritePath( char* szFilePath, const char* szFileName )
{
	sprintf_s( szFilePath, 256, "%s/%s", sszDirectory, szFileName );
}