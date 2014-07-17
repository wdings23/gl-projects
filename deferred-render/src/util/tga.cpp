#include "tga.h"
#include "filepathutil.h"

/*
**
*/
void saveTGA( const char* szFileName,
              const char* szDirectory,
              int iWidth,
              int iHeight,
              unsigned char* acImage )
{
	unsigned char* acTemp = (unsigned char *)MALLOC( sizeof( char ) * iWidth * iHeight * 4 );
	memcpy( acTemp, acImage, sizeof( char ) * iWidth * iHeight * 4 );

	for( int iY = 0; iY < iHeight; iY++ )
	{
		for( int iX = 0; iX < iWidth; iX++ )
		{
			unsigned int iIndex = ( iY * iWidth + iX ) * 4;
			unsigned char cTemp = acTemp[iIndex];
			acTemp[iIndex] = acTemp[iIndex+2];
			acTemp[iIndex+2] = cTemp;
		}
	}
	
#if 0
	// flip
	unsigned char* acTempLine = (unsigned char *)MALLOC( sizeof( char ) * iWidth * 4 );
	for( int iY = 0; iY < ( iHeight >> 1 ); iY++ )
	{
		int iSrcIndex = iY * iWidth * 4;
		int iDestIndex = ( iHeight - iY - 1 ) * iWidth * 4;

		memcpy( acTempLine, &acTemp[iSrcIndex], sizeof( char ) * iWidth * 4 );
		memcpy( &acTemp[iSrcIndex], &acTemp[iDestIndex], sizeof( char ) * iWidth * 4 );
		memcpy( &acTemp[iDestIndex], acTempLine, sizeof( char ) * iWidth * 4 );
	}
	FREE( acTempLine );
#endif // #if 0

	char szFullPath[512];
	//getWritePath( szFullPath, szFileName );
	getWritePathWithDirectory( szFullPath, szDirectory, szFileName );
    FILE* fptr = fopen( szFullPath, "wb" );
	WTFASSERT2( fptr, "can't write to %s\n", szFileName );

	putc(0,fptr);
	putc(0,fptr);
	putc(2,fptr);                         /* uncompressed RGB */
	putc(0,fptr); putc(0,fptr);
	putc(0,fptr); putc(0,fptr);
	putc(0,fptr);
	putc(0,fptr); putc(0,fptr);           /* X origin */
	putc(0,fptr); putc(0,fptr);           /* y origin */
	putc((iWidth & 0x00FF),fptr);
	putc((iWidth & 0xFF00) / 256,fptr);
	putc((iHeight & 0x00FF),fptr);
	putc((iHeight & 0xFF00) / 256,fptr);
	putc(32,fptr);                        /* 32 bit bitmap */
	putc(0,fptr);

	fwrite( acTemp, sizeof( char ), iWidth * iHeight * 4, fptr );
	fclose( fptr );

	FREE( acTemp );
}

/*
**
*/
void loadTGA( const char* szFileName,
			  const char* szDirectory,
			  int* piImageWidth,
			  int* piImageHeight,
			  unsigned char* acImage )
{
	char szFullPath[512];
    if( strstr( szFileName, ".tga" ) )
    {
        getFullPath( szFullPath, szFileName );
    }
    else
    {
        char szFullName[128];
        snprintf( szFullName, sizeof( szFullName ), "%s.tga", szFileName );
        getFullPath( szFullPath, szFullName );
    }
    
	FILE* fp = fopen( szFullPath, "rb" );
	if( fp == NULL )
    {
        // check save directory
        getWritePathWithDirectory( szFullPath, szDirectory, szFileName );
        
        fp = fopen( szFullPath, "rb" );
    }
    
    assert( fp );

	fseek( fp, 0, SEEK_END );
	long iBufferSize = ftell( fp );
	fseek( fp, 0, SEEK_SET );
	
	unsigned char* aBuffer = (unsigned char *)MALLOC( iBufferSize );
	fread( aBuffer, sizeof( unsigned char ), iBufferSize, fp );
	fclose( fp );	
	
	fseek( fp, 12, SEEK_SET );
	int iLow = (int)aBuffer[12];
	int iHigh = (int)aBuffer[13];
	*piImageWidth = iLow | ( iHigh << 8 );
	
	iLow = (int)aBuffer[14];
	iHigh = (int)aBuffer[15];
	*piImageHeight = iLow | ( iHigh << 8 );
	
	int iImageWidth = *piImageWidth;
	int iImageHeight = *piImageHeight;

	int iBufferStart = 18;
	int iScanLineSize = iImageWidth * 4;
	int iIndex = 0;
	for( int i = 0; i < iImageHeight; i++ )
    {
		iIndex = i * iImageWidth * 4;
        WTFASSERT2( iIndex < iImageHeight * iImageWidth * 4, "array out of bounds" );
		memcpy( &acImage[iIndex], &aBuffer[iBufferStart], iScanLineSize );
		
		int iCharIndex = iIndex;
		for( int j = 0; j < iImageWidth; j++ )
		{
			unsigned char cTemp = acImage[iCharIndex];
			acImage[iCharIndex] = acImage[iCharIndex+2];
			acImage[iCharIndex+2] = cTemp;
			
			iCharIndex += 4;
		}
		
		iBufferStart += iScanLineSize;
	}
		
	FREE( aBuffer );
}