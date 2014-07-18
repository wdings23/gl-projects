#include "tga.h"
//#include "filepathutil.h"
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MALLOC malloc
#define WTFASSERT2( X, ... ) assert( X )
#define FREE free


/*
**
*/
void saveTGA( const char* szFileName,
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

	FILE* fptr = fopen( szFileName, "wb" );
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
void loadTGA( unsigned char** acImage,
			  const char* szFileName,
			  unsigned int* piImageWidth,
			  unsigned int* piImageHeight )
{
	FILE* fp = fopen( szFileName, "rb" );
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
	
	int iPixDepth = (int)aBuffer[16];

	int iImageWidth = *piImageWidth;
	int iImageHeight = *piImageHeight;

	*acImage = (unsigned char *)malloc( iImageWidth * iImageHeight * 4 * sizeof( char ) );

	int iBufferStart = 18;
	int iScanLineSize = iImageWidth * 4;
	int iIndex = 0;

	if( iPixDepth == 32 )
	{
		for( int i = 0; i < iImageHeight; i++ )
		{
			iIndex = i * iImageWidth * 4;
			WTFASSERT2( iIndex < iImageHeight * iImageWidth * 4, "array out of bounds" );
			memcpy( &((*acImage)[iIndex]), &aBuffer[iBufferStart], iScanLineSize );
			
			int iCharIndex = iIndex;
			for( int j = 0; j < iImageWidth; j++ )
			{
				unsigned char cTemp = (*acImage)[iCharIndex];
				(*acImage)[iCharIndex] = (*acImage)[iCharIndex+2];
				(*acImage)[iCharIndex+2] = cTemp;
				
				iCharIndex += 4;
			}
			
			iBufferStart += iScanLineSize;
		}
	}
	else if( iPixDepth == 24 )
	{
		for( int i = 0; i < iImageHeight; i++ )
		{
			for( int j = 0; j < iImageWidth; j++ )
			{
				iIndex = ( i * iImageWidth + j ) << 2;
				int iBufferIndex = ( ( i * iImageWidth + j ) * 3 ) + iBufferStart;
				(*acImage)[iIndex] = aBuffer[iBufferIndex+2];
				(*acImage)[iIndex+1] = aBuffer[iBufferIndex+1];
				(*acImage)[iIndex+2] = aBuffer[iBufferIndex];
				(*acImage)[iIndex+3] = 255;
			}
		}

	}
		
	FREE( aBuffer );
}