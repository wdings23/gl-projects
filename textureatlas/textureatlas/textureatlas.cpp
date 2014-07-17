// textureatlas.cpp : Defines the entry point for the console application.
//

#include <sys/types.h>
#include <dirent.h>
#include "lodepng.h"
#include "tga.h"

#include <assert.h>

#define ATLAS_WIDTH 2048
#define ATLAS_HEIGHT 2048

enum
{
	IMAGETYPE_PNG = 0,
	IMAGETYPE_TGA,

	NUM_IMAGETYPES,
};

struct CommandLine
{
	char	mszType[128];
	char	mszData[128];
};

typedef struct CommandLine tCommandLine;

struct ImageFileInfo
{
	char				mszFullPath[256];
	char				mszName[128];
	int					miWidth;
	int					miHeight;

	int					miImageType;
};

typedef struct ImageFileInfo tImageFileInfo;

struct QuadInfo
{
	int		miX;
	int		miY;

	int		miWidth;
	int		miHeight;

	int		miPNGIndex;
	int		miAtlasIndex;
};

typedef struct QuadInfo tQuadInfo;

static void listDir( const char* szDir, 
					 tImageFileInfo** paImageInfo, 
					 int* piNumInfo, 
					 int* piNumImageAlloc );

static bool makeAtlas( unsigned char* aAtlas,
					   unsigned int iAtlasWidth,
					   unsigned int iAtlasHeight,
					   unsigned int* piCurrX,
					   unsigned int* piCurrY,
					   unsigned int* piCurrHighest,
					   unsigned int iPadding,
					   unsigned char* aImage, 
					   unsigned int iImageWidth, 
					   unsigned int iImageHeight );

static void placeTexturesInAtlas( tImageFileInfo const* aImageInfo, 
								  int iAtlasWidth,
								  int iAtlasHeight,
								  int iNumTex, 
								  int iPadding,
								  int iAtlasImageType );

static void saveAtlas( tQuadInfo const* aQuadInfo,
					   tImageFileInfo const* aImageInfo,
					   int iAtlasWidth,
					   int iAtlasHeight,
					   int iNumQuads,
					   int iAtlasImageType );

static void quickSort( int* aiInput, int iLeft, int iRight );

static char sszDirectory[256];
static char sszDestination[256];

/*
**
*/
int main(int argc, char* argv[])
{
	// command line
	int iNumCommands = 0;
	tCommandLine aCommandLines[10];
	
	tCommandLine* pCommandLine = &aCommandLines[iNumCommands++];
	for( int i = 1; i < argc; i++ )
	{
		if( i % 2 == 0 )
		{
			strcpy( pCommandLine->mszData, argv[i] );
			pCommandLine = &aCommandLines[iNumCommands++];
		}
		else
		{
			strcpy( pCommandLine->mszType, &argv[i][1] );
		}
	}
	
	int iAtlasImageType = IMAGETYPE_PNG;

	// default atlas dimension
	int iAtlasWidth = ATLAS_WIDTH;
	int iAtlasHeight = ATLAS_HEIGHT;

	// root directory and padding
	const char* szDirectory = NULL;
    const char* szDestination = NULL;
	unsigned int iPadding = 0;
	bool bSortByHeight = false;
	for( int i = 0; i < iNumCommands; i++ )
	{
		tCommandLine* pCommandLine = &aCommandLines[i];
		if( !strcmp( pCommandLine->mszType, "dir" ) )
		{
			szDirectory = pCommandLine->mszData;
			strncpy( sszDirectory, pCommandLine->mszData, sizeof( sszDirectory ) );
		}
		else if( !strcmp( pCommandLine->mszType, "padding" ) )
		{
			iPadding = atoi( pCommandLine->mszData );
		}
		else if( !strcmp( pCommandLine->mszType, "width" ) )
		{
			iAtlasWidth = atoi( pCommandLine->mszData );
		}
		else if( !strcmp( pCommandLine->mszType, "height" ) )
		{
			iAtlasHeight = atoi( pCommandLine->mszData );
		}
        else if( !strcmp( pCommandLine->mszType, "destination" ) )
		{
			szDestination = pCommandLine->mszData;
			strncpy( sszDestination, pCommandLine->mszData, sizeof( sszDestination ) );
		}
		else if( !strcmp( pCommandLine->mszType, "sort_by_height" ) )
		{
			if( !strcmp( pCommandLine->mszData, "true" ) )
			{
				bSortByHeight = true;
			}
		}
		else if( !strcmp( pCommandLine->mszType, "image_type" ) )
		{
			if( !strcmp( pCommandLine->mszData, "png" ) )
			{
				iAtlasImageType = IMAGETYPE_PNG;
			}
			else if(  !strcmp( pCommandLine->mszData, "tga" ) )
			{
				iAtlasImageType = IMAGETYPE_TGA;
			}
		}
	}
	
    if( szDirectory == NULL )
    {
        printf( "usage: textureatlas -dir <directory> -destination <destination directory> -padding <padding in pixel> -width <width of the atlas> -height <height of the atlas> -sort_by_height <true/false> -image_type <png/tga>\n" );
        exit( 0 );
    }
    
    if( szDestination == NULL )
    {
        szDestination = szDirectory;
    }
    
	// atlas
	unsigned char* acAtlas = (unsigned char *)malloc( iAtlasWidth * iAtlasHeight * 4 );
	memset( acAtlas, 0, iAtlasWidth * iAtlasHeight * 4 );
	
	// get the png files
	tImageFileInfo* aPNGFileInfo = NULL;
	int iNumInfo = 0;
	int iInfoAlloc = 0;
	listDir( szDirectory, &aPNGFileInfo, &iNumInfo, &iInfoAlloc );
	placeTexturesInAtlas( aPNGFileInfo,
						  iAtlasWidth,
						  iAtlasHeight,
						  iNumInfo,
						  iPadding,
						  iAtlasImageType );

	return 0;
}

/*
**
*/
static void listDir( const char* szDir, 
					 tImageFileInfo** paImageInfo, 
					 int* piNumInfo, 
					 int* piNumImageAlloc )
{
	if( *piNumImageAlloc == 0 )
	{
		*piNumImageAlloc = 100;
		*paImageInfo = (tImageFileInfo *)malloc( sizeof( tImageFileInfo ) * *piNumImageAlloc );
		*piNumInfo = 0;
	}

	tImageFileInfo* aImageInfo = *paImageInfo;

	DIR* dp = opendir( szDir );
	if( dp != NULL )
	{
		struct dirent* ep;
		while( ( ep = readdir( dp ) ) != NULL )
		{
			//printf( "%s\n", ep->d_name );

			if( ep->d_type == DT_DIR && 
				strcmp( ep->d_name, "." ) &&
				strcmp( ep->d_name, ".." ) )
			{
				// go into directory
				char szFullPath[512];
				sprintf( szFullPath, "%s/%s", szDir, ep->d_name );
				listDir( szFullPath, paImageInfo, piNumInfo, piNumImageAlloc );
			}
			else
			{
				char szExtension[16];
				memset( szExtension, 0, sizeof( szExtension ) );
				int iLength = strlen( ep->d_name );
				for( int i = iLength - 1; i >= 0; i-- )
				{
					if( ep->d_name[i] == '.' )
					{
						memcpy( szExtension, &ep->d_name[i+1], iLength - i );
						break;
					}
				}

				if( ( !strcmp( szExtension, "png" ) || !strcmp( szExtension, "tga" ) ) && 
                    !strstr( ep->d_name, "textureatlas" ) &&
                    !strstr( ep->d_name, ".svn" ) )
				{
					// save info file name and dimension
					printf( "found image: %s\n", ep->d_name );

					unsigned char* pImage = NULL;
					unsigned int iWidth = 0;
					unsigned int iHeight = 0;

					char szFullPath[256];
					sprintf( szFullPath, "%s/%s", szDir, ep->d_name );
					
					int iImageType = IMAGETYPE_PNG;
					if( !strcmp( szExtension, "png" ) )
					{
						LodePNG_decode32_file( &pImage, &iWidth, &iHeight, szFullPath );
						iImageType = IMAGETYPE_PNG;
					}
					else if( !strcmp( szExtension, "tga" ) )
					{
						loadTGA( &pImage, szFullPath, &iWidth, &iHeight ); 
						iImageType = IMAGETYPE_TGA;
					}

					if( *piNumInfo >= *piNumImageAlloc )
					{
						*piNumImageAlloc += 100;
						*paImageInfo = (tImageFileInfo *)realloc( *paImageInfo, sizeof( tImageFileInfo ) * *piNumImageAlloc );
						aImageInfo = *paImageInfo;
					}

					tImageFileInfo* pInfo = &aImageInfo[*piNumInfo];
					
					strcpy( pInfo->mszFullPath, szFullPath );
					strcpy( pInfo->mszName, ep->d_name );
					pInfo->miHeight = iHeight;
					pInfo->miWidth = iWidth;
					pInfo->miImageType = iImageType;

					++(*piNumInfo);

					free( pImage );
				}
			}
		}

		closedir( dp );
	}
}

/*
**
*/
static bool makeAtlas( unsigned char* aAtlas,
					   unsigned int iAtlasWidth,
					   unsigned int iAtlasHeight,
					   unsigned int* piCurrX,
					   unsigned int* piCurrY,
					   unsigned int* piCurrHighest,
					   unsigned int iPadding,
					   unsigned char* aImage, 
					   unsigned int iImageWidth, 
					   unsigned int iImageHeight )
{
	// keep track so we can skip to next row
	if( *piCurrX + iImageWidth < iAtlasWidth &&
		iImageHeight > *piCurrHighest )
	{
		*piCurrHighest = iImageHeight;
	}
	
	// onto next row
	if( *piCurrX + iImageWidth > iAtlasWidth )
	{
		*piCurrY += ( *piCurrHighest + iPadding );
		*piCurrX = 0;
		*piCurrHighest = iImageHeight;

		if( *piCurrY > iAtlasHeight )
		{
			return false;
		}
	}

	if( *piCurrY + iImageHeight + iPadding >= iAtlasHeight )
	{
        return false;
	}
	
	// copy each scanline
	for( unsigned int i = 0; i < iImageHeight; i++ )
	{
		unsigned int iAtlasIndex = ( ( *piCurrY + i ) * iAtlasWidth + *piCurrX ) * 4;
		unsigned int iImageIndex = ( i * iImageWidth ) * 4;

		assert( iAtlasIndex < iAtlasWidth * iAtlasHeight * 4 );
		assert( iImageIndex < iImageWidth * iImageHeight * 4 );

		memcpy( &aAtlas[iAtlasIndex],
				&aImage[iImageIndex],
				iImageWidth * 4 );
	}
	
	*piCurrX += ( iImageWidth + iPadding );

	return true;
}

/*
**
*/
static bool intersect( int iX, 
					   int iY,
					   int iWidth,
					   int iHeight,
					   int iPadding,
					   tQuadInfo const* pQuadInfo )
{
	bool bIntersectX0 = ( iX >= pQuadInfo->miX && iX <= pQuadInfo->miX + pQuadInfo->miWidth );
	bool bIntersectX1 = ( pQuadInfo->miX >= iX && pQuadInfo->miX <= iX + iWidth );

	bool bIntersectY0 = ( iY >= pQuadInfo->miY && iY <= pQuadInfo->miY + pQuadInfo->miHeight );
	bool bIntersectY1 = ( pQuadInfo->miY >= iY && pQuadInfo->miY <= iY + iHeight );

	return ( ( bIntersectX0 || bIntersectX1 ) && ( bIntersectY0 || bIntersectY1 ) );
}

/*
**
*/
static void placeTexturesInAtlas( tImageFileInfo const* aImageInfo, 
								  int iAtlasWidth,
								  int iAtlasHeight,
								  int iNumTex, 
								  int iPadding,
								  int iAtlasImageType )
{
	int iNumQuads = 0;
	tQuadInfo* aQuadInfo = (tQuadInfo *)malloc( sizeof( tQuadInfo ) * iNumTex );
	int iCurrAtlasIndex = 0;
	int iNumAtlas = 1;

	for( int iTex = 0; iTex < iNumTex; iTex++ )
	{
		tImageFileInfo const* pImageInfo = &aImageInfo[iTex];

		// find big enough space to place the texture
		int iX = iPadding;
		int iY = iPadding;
		
		bool bFoundFreeSpace = true;
		for( int iAtlas = 0; iAtlas < iNumAtlas; iAtlas++ )
		{
			for( int iY = iPadding; iY < iAtlasHeight - iPadding; iY++ )
			{
				bFoundFreeSpace = true;
				int iSmallestHeight = 999999;
				for( int iX = iPadding; iX < iAtlasWidth - iPadding; iX++ )
				{
					// check if this coordinate is free
					for( int iQuad = 0; iQuad < iNumQuads; iQuad++ )
					{
						bFoundFreeSpace = true;
						tQuadInfo const* pQuadInfo = &aQuadInfo[iQuad];
						if( pQuadInfo->miAtlasIndex == iAtlas )
						{
							// save the smallest y so we can know how many to skip on y
							if(	intersect( iX, 
										   iY,
										   pImageInfo->miWidth,
										   pImageInfo->miHeight,
										   iPadding,
										   pQuadInfo ) )
							{
								bFoundFreeSpace = false;
								int iSkip = ( pQuadInfo->miX + pQuadInfo->miWidth + iPadding - 1 );
								iX = iSkip;	// minus 1 due to increment of the x in the loop
								
								if( iSmallestHeight > ( pQuadInfo->miY + pQuadInfo->miHeight ) - iY )
								{
									iSmallestHeight = ( pQuadInfo->miY + pQuadInfo->miHeight ) - iY;
								}

								break;

							}	// if intersect with quad

						}	// if the quad's atlas == current atlas we're checking
					}
					
					// check if it's out of bounds
					if( iX + pImageInfo->miWidth >= iAtlasWidth )
					{
						// skip to next row
						iX = iAtlasWidth; 
						bFoundFreeSpace = false;
					}

					if( iY + pImageInfo->miHeight >= iAtlasHeight )
					{
						bFoundFreeSpace = false;
					}

					// put into this quad
					if( bFoundFreeSpace )
					{
						tQuadInfo* pFoundQuad = &aQuadInfo[iNumQuads++];
						pFoundQuad->miX = iX;
						pFoundQuad->miY = iY;
						pFoundQuad->miWidth = pImageInfo->miWidth;
						pFoundQuad->miHeight = pImageInfo->miHeight;
						pFoundQuad->miPNGIndex = iTex;
						pFoundQuad->miAtlasIndex = iAtlas;

						printf( "place %s at ( %d, %d, %d, %d ) atlas = %d\n",
								pImageInfo->mszName,
								pFoundQuad->miX, 
								pFoundQuad->miY,
								pFoundQuad->miX + pFoundQuad->miWidth,
								pFoundQuad->miY + pFoundQuad->miHeight,
								pFoundQuad->miAtlasIndex );

						break;
					}

				}	// for x = padding to width - padding
				
				if( bFoundFreeSpace )
				{
					break;
				}
				else
				{
					if( iSmallestHeight < 999999 )
					{
						// skip the height
						iY += ( iSmallestHeight + iPadding - 1 );
					}
				}

			}	// for y = padding to height - padding
			
			// new atlas texture
			if( bFoundFreeSpace )
			{
				break;
			}
			else
			{
				// check if need to create new atlas
				if( iAtlas == iNumAtlas - 1 )
				{
					++iNumAtlas;
				}
			}

		}	// for atlas = 0 to num atlas

	}	// for i = 0 to num textures

	saveAtlas( aQuadInfo,
			   aImageInfo,
			   iAtlasWidth,
			   iAtlasHeight,
			   iNumQuads,
			   iAtlasImageType );

	free( aQuadInfo );
}

/*
**
*/
static void saveAtlas( tQuadInfo const* aQuadInfo,
					   tImageFileInfo const* aImageInfo,
					   int iAtlasWidth,
					   int iAtlasHeight,
					   int iNumQuads,
					   int iAtlasImageType )
{
	// folder
	char szFolder[128];
	memset( szFolder, 0, sizeof( szFolder ) );
	int iLength = (int)strlen( sszDirectory );
	for( int i = iLength - 1; i >= 0; i-- )
	{
		if( sszDirectory[i] == '\\' || sszDirectory[i] == '/' )
		{
			strcpy( szFolder, &sszDirectory[i+1] );
			break;
		}
	}
	
	// atlas info text file
	char szFullPath[256];
	sprintf( szFullPath, "%s/%s_textureatlas.txt", sszDestination, szFolder );
	
	FILE* fp = fopen( szFullPath, "wb" );
	fprintf( fp, "<atlas>\n" );

	// texture info in the atlas
	for( int i = 0; i < iNumQuads; i++ )
	{
		tQuadInfo const* pQuadInfo = &aQuadInfo[i];
		tImageFileInfo const* pImageInfo = &aImageInfo[pQuadInfo->miPNGIndex];
		
		char szAtlasName[256];
		if( iAtlasImageType == IMAGETYPE_PNG )
		{
			sprintf( szAtlasName, "%s_textureatlas%d.png", szFolder, pQuadInfo->miAtlasIndex );
		}
		else
		{
			sprintf( szAtlasName, "%s_textureatlas%d.tga", szFolder, pQuadInfo->miAtlasIndex );
		}

		fprintf( fp, "\t<texture>\n" );
		fprintf( fp, "\t\t<name>%s</name>\n", pImageInfo->mszName );
		fprintf( fp, "\t\t<atlas>%s</atlas>\n", szAtlasName );
		fprintf( fp, "\t\t<topleft>%d,%d</topleft>\n", pQuadInfo->miX, pQuadInfo->miY );
		fprintf( fp, "\t\t<bottomright>%d,%d</bottomright>\n", pQuadInfo->miX + pQuadInfo->miWidth, pQuadInfo->miY + pQuadInfo->miHeight );
		fprintf( fp, "\t</texture>\n" );

	}	// for i = 0 to num quads

	fprintf( fp, "</atlas>\n" );
	fclose( fp );

	int iLastAtlasIndex = 0;
	unsigned char* acAtlas = (unsigned char *)malloc( iAtlasWidth * iAtlasHeight * 4 * sizeof( char ) );
	memset( acAtlas, 0, iAtlasWidth * iAtlasHeight * 4 * sizeof( char ) );

	for( int i = 0; i < iNumQuads; i++ )
	{
		tQuadInfo const* pQuadInfo = &aQuadInfo[i];
		tImageFileInfo const* pImageInfo = &aImageInfo[pQuadInfo->miPNGIndex];
		
		// png
		unsigned char* acImage = NULL;
		unsigned int iWidth = 0;
		unsigned int iHeight = 0;

		if( pImageInfo->miImageType == IMAGETYPE_PNG )
		{
			LodePNG_decode32_file( &acImage, &iWidth, &iHeight, pImageInfo->mszFullPath );
		}
		else
		{
			loadTGA( &acImage, pImageInfo->mszFullPath, &iWidth, &iHeight );
		}

		// create new atlas
		if( pQuadInfo->miAtlasIndex != iLastAtlasIndex )
		{
			char szAtlasName[256];
			if( iAtlasImageType == IMAGETYPE_PNG )
			{
				sprintf( szAtlasName, "%s/%s_textureatlas%d.png", sszDestination, szFolder, iLastAtlasIndex );
			}
			else
			{
				sprintf( szAtlasName, "%s/%s_textureatlas%d.tga", sszDestination, szFolder, iLastAtlasIndex );
			}

			printf( "writing atlas: %s...\n", szAtlasName );
			
			FILE* fp = fopen( szAtlasName, "rb" );
			if( fp )
			{
				fclose( fp );

				unsigned char* acTempImage = NULL;

				unsigned int iPrevAtlasWidth = 0, iPrevAtlasHeight = 0;

				if( iAtlasImageType == IMAGETYPE_PNG )
				{
					LodePNG_decode32_file( &acTempImage, &iPrevAtlasWidth, &iPrevAtlasHeight, szAtlasName ); 
				}
				else
				{
					loadTGA( &acTempImage, szAtlasName, &iPrevAtlasWidth, &iPrevAtlasHeight );
				}

				// merge image
				for( int iY = 0; iY < (int)iPrevAtlasHeight; iY++ )
				{
					for( int iX = 0; iX < (int)iPrevAtlasWidth; iX++ )
					{
						int iIndex = ( iY * iPrevAtlasWidth + iX ) * 4;
						acAtlas[iIndex] += acTempImage[iIndex];
						acAtlas[iIndex+1] += acTempImage[iIndex+1];
						acAtlas[iIndex+2] += acTempImage[iIndex+2];
						acAtlas[iIndex+3] += acTempImage[iIndex+3];
					}
				}

				free( acTempImage );
			}
			
			if( iAtlasImageType == IMAGETYPE_PNG )
			{
				LodePNG_encode32_file( szAtlasName, acAtlas, iAtlasWidth, iAtlasHeight );
			}
			else
			{
				saveTGA( szAtlasName, iAtlasWidth, iAtlasHeight, acAtlas );
			}

			memset( acAtlas, 0, iAtlasWidth * iAtlasHeight * 4 * sizeof( char ) );
			iLastAtlasIndex = pQuadInfo->miAtlasIndex;
		}

		// scaled background to pad out the gaps
		int iPadding = 1;
		int iHeightWithPadding = iHeight + 2 * iPadding;
		int iWidthWithPadding = iWidth + 2 * iPadding;
		float fScaledHeight = (float)iHeight / (float)iHeightWithPadding;
		float fScaledWidth = (float)iWidth / (float)iWidthWithPadding;
		for( int iY = 0; iY < iHeightWithPadding; iY++ )
		{
			int iOrigY = (int)( (float)iY * fScaledHeight );
			int iAtlasIndex = ( ( ( pQuadInfo->miY - iPadding ) + iY ) * iAtlasWidth + ( pQuadInfo->miX - iPadding ) ) * 4;
			
			for( int iX = 0; iX < iWidthWithPadding; iX++ )
			{
				int iOrigX = (int)( (float)iX * fScaledWidth );
				int iOrigIndex = ( iOrigY * iWidth + iOrigX ) << 2;
				
				assert( iAtlasIndex < iAtlasWidth * iAtlasHeight * 4 );
				assert( iOrigIndex < (int)iWidth * (int)iHeight * 4 );

				memcpy( &acAtlas[iAtlasIndex], &acImage[iOrigIndex], 4 );
				iAtlasIndex += 4;
			}
		}

		// copy each scanline
		for( unsigned int iY = 0; iY < iHeight; iY++ )
		{
			int iAtlasIndex = ( ( pQuadInfo->miY + iY ) * iAtlasWidth + pQuadInfo->miX ) * 4;
			int iImageIndex = ( iY * iWidth ) * 4;

			assert( iAtlasIndex < iAtlasWidth * iAtlasHeight * 4 );
			assert( iImageIndex < (int)iWidth * (int)iHeight * 4 );

			memcpy( &acAtlas[iAtlasIndex],
					&acImage[iImageIndex],
					iWidth * 4 );
			
			// left
			iAtlasIndex = ( ( pQuadInfo->miY + iY ) * iAtlasWidth + ( pQuadInfo->miX - 1 ) ) * 4;
			iImageIndex = ( iY * iWidth ) * 4;
			memcpy( &acAtlas[iAtlasIndex],
					&acImage[iImageIndex],
					4 );

			// right
			iAtlasIndex += ( ( iWidth + 1 ) * 4 );
			iImageIndex = ( ( iY + 1 ) * iWidth ) * 4 - 4;
			memcpy( &acAtlas[iAtlasIndex],
					&acImage[iImageIndex],
					4 );
		}
		
		free( acImage );

	}	// for i = 0 to num quads
	
	// write out last atlas
	char szAtlasName[256];
	if( iAtlasImageType == IMAGETYPE_PNG )
	{
		sprintf( szAtlasName, "%s/%s_textureatlas%d.png", sszDestination, szFolder, iLastAtlasIndex );
	}
	else
	{
		sprintf( szAtlasName, "%s/%s_textureatlas%d.tga", sszDestination, szFolder, iLastAtlasIndex );
	}

	printf( "writing atlas: %s...\n", szAtlasName );

	fp = fopen( szAtlasName, "rb" );
	if( fp )
	{
		fclose( fp );

		unsigned char* acTempImage = NULL;

		unsigned int iPrevAtlasWidth = 0, iPrevAtlasHeight = 0;
		if( iAtlasImageType == IMAGETYPE_PNG )
		{
			LodePNG_decode32_file( &acTempImage, &iPrevAtlasWidth, &iPrevAtlasHeight, szAtlasName ); 
		}
		else
		{
			loadTGA( &acTempImage, szAtlasName, &iPrevAtlasWidth, &iPrevAtlasHeight );
		}

		// merge image
		for( int iY = 0; iY < (int)iPrevAtlasHeight; iY++ )
		{
			for( int iX = 0; iX < (int)iPrevAtlasWidth; iX++ )
			{
				int iIndex = ( iY * iPrevAtlasWidth + iX ) * 4;
				acAtlas[iIndex] += acTempImage[iIndex];
				acAtlas[iIndex+1] += acTempImage[iIndex+1];
				acAtlas[iIndex+2] += acTempImage[iIndex+2];
				acAtlas[iIndex+3] += acTempImage[iIndex+3];
			}
		}

		free( acTempImage );
	}
	
	if( iAtlasImageType == IMAGETYPE_PNG )
	{
		LodePNG_encode32_file( szAtlasName, acAtlas, iAtlasWidth, iAtlasHeight );
	}
	else
	{
		saveTGA( szAtlasName, iAtlasWidth, iAtlasHeight, acAtlas );
	}

	free( acAtlas );
}

/*
**
*/
static void quickSort( int* aiInput, int iLeft, int iRight )
{
	int i = iLeft, j = iRight;
	int iPivotIndex = ( iRight + iLeft ) >> 1;
	int iPivot = aiInput[iPivotIndex];

	while( i <= j )
	{
		while( aiInput[i] < iPivot )
		{
			++i;
		}

		while( aiInput[j] > iPivot )
		{
			--j;
		}

		if( i <= j )
		{
			int iTemp = aiInput[i];
			aiInput[i] = aiInput[j];
			aiInput[j] = iTemp;
			++i;
			--j;
		}
	}

	if( iLeft < iRight )
	{
		quickSort( aiInput, iLeft, j );		// left partition
		quickSort( aiInput, i, iRight );	// right partition
	}
}