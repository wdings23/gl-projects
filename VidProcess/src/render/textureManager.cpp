#include "textureManager.h"
#include "filepathutil.h"
#include "timeutil.h"
#include "lodepng.h"
#include "hashutil.h"

struct PVRHeader
{
    unsigned int miHeaderLength;
    unsigned int miHeight;
    unsigned int miWidth;
    unsigned int miMIPMaps;
    unsigned int miFlags;
    unsigned int miDataLength;
    unsigned int miBPP;
    unsigned int miBitMaskRed;
    unsigned int miBitMaskGreen;
    unsigned int miBitMaskBlue;
    unsigned int miBitMaskAlpha;
    unsigned int miPVRTag;
    unsigned int miNumSurfaces;
};

typedef struct PVRHeader tPVRHeader;

/*
**
*/
CTextureManager* CTextureManager::mpInstance = NULL;
CTextureManager* CTextureManager::instance( void )
{
	if( mpInstance == NULL )
	{
		mpInstance = new CTextureManager();
	}

	return mpInstance;
}

/*
**
*/
static void readTargaDimensions( const char* pszFileName, 
						  int* iWidth,
						  int* iHeight )
{
    char szFullPath[512];
    getFullPath( szFullPath, pszFileName );
    
	*iWidth = *iHeight = 0;
	
	FILE* fp = fopen( szFullPath, "rb" );
    /*OUTPUT( "%s : %d szFullPath = %s\n",
            __FUNCTION__,
            __LINE__,
           szFullPath );
	*/
    if( fp == NULL )
    {
        getWritePath( szFullPath, pszFileName );
        fp = fopen( szFullPath, "rb" );
    }
    
    WTFASSERT2( fp, "can't load texture %s\n", pszFileName );
	
	int iOriginX = 0, iOriginY = 0;
	int iColorBits = 0;
	fgetc( fp );
	fgetc( fp );
	fgetc( fp );
	fgetc( fp ); fgetc( fp );
	fgetc( fp ); fgetc( fp );
	fgetc( fp );
	iOriginX |= fgetc( fp ); iOriginX |= fgetc( fp ) << 8;
	iOriginY |= fgetc( fp ); iOriginY |= fgetc( fp ) << 8;
	*iWidth |= fgetc( fp ); *iWidth |= fgetc( fp ) << 8;
	*iHeight |= fgetc( fp ); *iHeight |= fgetc( fp ) << 8;
	iColorBits = fgetc( fp )>>3;
	
	fclose( fp );
}

/*
**
*/
void CTextureManager::readTarga( const char* pszFileName,
                                 int* iWidth,
                                 int* iHeight,
                                 unsigned char** aImage )
{
	//int iOriginX = 0, iOriginY = 0;
	//int iColorBits = 0;
	
	*iWidth = *iHeight = 0;
	int iImageWidth = 0, iImageHeight = 0;
	
    char szFullPath[512];
    
    if( strstr( pszFileName, ".tga" ) )
    {
        getFullPath( szFullPath, pszFileName );
    }
    else
    {
        char szFullName[128];
        snprintf( szFullName, sizeof( szFullName ), "%s.tga", pszFileName );
        getFullPath( szFullPath, szFullName );
    }
    
    
	FILE* fp = fopen( szFullPath, "rb" );
	if( fp == NULL )
    {
        // check save directory
        getWritePath( szFullPath, pszFileName );
        
        fp = fopen( szFullPath, "rb" );
    }
    
    assert( fp );
	
	fseek( fp, 0, SEEK_END );
	long iBufferSize = ftell( fp );
	fseek( fp, 0, SEEK_SET );
	
	unsigned char* aBuffer = (unsigned char *)MALLOC( iBufferSize );
	fread( aBuffer, sizeof( unsigned char ), iBufferSize, fp );
	fclose( fp );	
	
	iImageWidth = (int)aBuffer[12]; iImageWidth |= (int)( aBuffer[13] << 8 );
	iImageHeight = (int)aBuffer[14]; iImageHeight |= (int)( aBuffer[15] << 8 );
	
	*iWidth = iImageWidth; *iHeight = iImageHeight;
	*aImage = (unsigned char *)MALLOC( iImageWidth * iImageHeight * 4 * sizeof( unsigned char ) );
	
	char cBitsPerPixel = aBuffer[16];
	
	if( cBitsPerPixel == 24 )
	{
		int iBufferStart = 18;
		
		for( int iY = 0; iY < iImageHeight; iY++ )
		{
			for( int iX = 0; iX < iImageWidth; iX++ )
			{
				int iIndex = ( iY * iImageWidth + iX ) << 2;
				int iImageIndex = ( iY * iImageWidth + iX ) * 3;

				(*aImage)[iIndex] = aBuffer[iBufferStart+iImageIndex+2];
				(*aImage)[iIndex+1] = aBuffer[iBufferStart+iImageIndex+1];
				(*aImage)[iIndex+2] = aBuffer[iBufferStart+iImageIndex];
				(*aImage)[iIndex+3] = 255;
			
			}	// for x = 0 to width

		}	// for y = 0 to height
	}
	else if( cBitsPerPixel == 32 )
	{
		int iBufferStart = 18;
		int iScanLineSize = iImageWidth * 4;
		int iIndex = 0;
		//for( int i = iImageHeight - 1; i >= 0; i-- )
		for( int i = 0; i < iImageHeight; i++ )
		{
			iIndex = i * iImageWidth * 4;
			WTFASSERT2( iIndex < iImageHeight * iImageWidth * 4, "array out of bounds" );
			memcpy( &(*aImage)[iIndex], &aBuffer[iBufferStart], iScanLineSize );
			
			int iCharIndex = iIndex;
			for( int j = 0; j < iImageWidth; j++ )
			{
				unsigned char cTemp = (*aImage)[iCharIndex];
				(*aImage)[iCharIndex] = (*aImage)[iCharIndex+2];
				(*aImage)[iCharIndex+2] = cTemp;
				
				iCharIndex += 4;
			}
			
			iBufferStart += iScanLineSize;
		}
	}
		
	FREE( aBuffer );
}

/*
**
*/
static void readPVR( const char* szFileName,
                     unsigned int* piWidth,
                     unsigned int* piHeight,
                     unsigned char** aData,
                     unsigned int* piBPP,
                     long* piSize )
{
    char szFullPath[512];
    getFullPath( szFullPath, szFileName );
    
    FILE* fp = fopen( szFullPath, "rb" );
    WTFASSERT2( fp, "No such file %s", szFullPath );
    
    fseek( fp, 0, SEEK_END );
    *piSize = ftell( fp );
    fseek( fp, 0, SEEK_SET );
    
    tPVRHeader header;
    fread( &header, sizeof( tPVRHeader ), 1, fp );
    
    *piSize = header.miDataLength;
    *aData = (unsigned char *)MALLOC( sizeof( char ) * *piSize );
    
    rewind( fp );
    fseek( fp, sizeof( tPVRHeader ), SEEK_SET );
    fread( *aData, sizeof( char ), *piSize, fp );
    
    *piWidth = header.miWidth;
    *piHeight = header.miHeight;
    
    *piBPP = header.miBPP;
    
    fclose( fp );
    
    OUTPUT( "REGISTER PVR %s\n", szFileName );
}

/*
**
*/
CTextureManager::CTextureManager( void )
{
	memset( maTextures, 0, sizeof( maTextures ) );
	memset( maiTextureIDs, 0, sizeof( maiTextureIDs ) );
	miNumTextures = 0;
	memset( maTextureSlots, 0, sizeof( maTextureSlots ) );

	glGenTextures( MAX_TEXTURE_SLOTS, maiTextureIDs );
    mbSemaphore = false;
    
    mbPurgeTextures = false;
}

/*
**
*/
CTextureManager::~CTextureManager( void )
{
    for( int i = 0; i < MAX_TEXTURE_SLOTS; i++ )
    {
        tTexture* pTexture = maTextureSlots[i].mpTexture;
        
        if( pTexture && pTexture->mbInMem )
        {
            glDeleteTextures( 1, maTextureSlots[i].mpiGLTexID );
        }
    }
}

/*
**
*/
void CTextureManager::registerTexture( const char* szTextureName, bool bCubeMap, GLint iEdgeTypeU, GLint iEdgeTypeV )
{
	tTexture* pTexture = getTexture( szTextureName );
	if( pTexture )
	{
		return;
	}

	WTFASSERT2( miNumTextures >= 0 && miNumTextures < sizeof( maTextures ) / sizeof( *maTextures ),
                "too many registered textures %d", miNumTextures );
    
	if( miNumTextures >= 0 && miNumTextures < sizeof( maTextures ) / sizeof( *maTextures ) )
	{
		pTexture = &maTextures[miNumTextures];
        if( strstr( szTextureName, ".tga" ) )
        {
            if( bCubeMap )
            {
                char szCubeTextureName[256];
                char szNoExtension[128];
                
                const char* szExtensionStart = strstr( szTextureName, ".tga" );
                size_t iSize = (size_t)szExtensionStart - (size_t)szTextureName;
                memset( szNoExtension, 0, sizeof( szNoExtension ) );
                memcpy( szNoExtension, szTextureName, iSize );
                
                snprintf( szCubeTextureName, sizeof( szCubeTextureName ), "%s_z_pos.tga", szNoExtension );
                readTargaDimensions( szCubeTextureName, &pTexture->miWidth, &pTexture->miHeight );
            }
            else
            {
                readTargaDimensions( szTextureName, &pTexture->miWidth, &pTexture->miHeight );
            }
            
            pTexture->miBPP = 8;
        }
        else if( strstr( szTextureName, ".png" ) )
        {
            char szFullPath[256];
            getFullPath( szFullPath, szTextureName );
            FILE* fp = fopen( szFullPath, "rb" );
            if( fp == NULL )
            {
                getWritePath( szFullPath, szTextureName );
                fp = fopen( szFullPath, "rb" );
                WTFASSERT2( fp, "can't load file: %s", szTextureName );
            }
        
            unsigned char* acImage = NULL;
            unsigned int iWidth, iHeight;
            LodePNG_decode32_file( &acImage, &iWidth, &iHeight, szFullPath );
            
            pTexture->miWidth = (int)iWidth;
            pTexture->miHeight = (int)iHeight;
            pTexture->miBPP = 8;
            FREE( acImage );
        }
        else if( strstr( szTextureName, ".pvr" ) )
        {
            unsigned char* acImage = NULL;
            unsigned int iWidth, iHeight;
            long iSize;
            unsigned int iBPP;
            readPVR( szTextureName, &iWidth, &iHeight, &acImage, &iBPP, &iSize );
            
            pTexture->miWidth = (int)iWidth;
            pTexture->miHeight = (int)iHeight;
            pTexture->miGLWidth = pTexture->miWidth;
            pTexture->miGLHeight = pTexture->miHeight;
            pTexture->miBPP = iBPP;
            
            pTexture->mbCompressed = true;
            
            FREE( acImage );
        }
        else 
        {
            char szFullName[128];
            sprintf( szFullName, "%s.tga", szTextureName );
            readTargaDimensions( szFullName, &pTexture->miWidth, &pTexture->miHeight );
        }

		strncpy( pTexture->mszName, szTextureName, sizeof( pTexture->mszName ) );
		
        pTexture->miHash = hash( pTexture->mszName );
        
        pTexture->miTextureEdgeTypeU = iEdgeTypeU;
        pTexture->miTextureEdgeTypeV = iEdgeTypeV;
        
        pTexture->mbCubeMap = bCubeMap;
        
		++miNumTextures;
	}
}

void CTextureManager::registerTextureWithData( const char* szTextureName, 
                                               unsigned char* acImage,
                                               int iWidth,
                                               int iHeight, 
                                               GLint iEdgeTypeU, 
                                               GLint iEdgeTypeV )
{
    WTFASSERT2( miNumTextures >= 0 && miNumTextures < sizeof( maTextures ) / sizeof( *maTextures ),
               "too many registered textures %d", miNumTextures );
    
	if( miNumTextures >= 0 && miNumTextures < sizeof( maTextures ) / sizeof( *maTextures ) )
	{
		tTexture* pTexture = &maTextures[miNumTextures];
        
        pTexture->miTextureEdgeTypeU = iEdgeTypeU;
        pTexture->miTextureEdgeTypeV = iEdgeTypeV;
        
        pTexture->miWidth = iWidth;
        pTexture->miHeight = iHeight;
        
        // texture is not in memory, check for FREE spot
        int iSlot = 0;
        for( iSlot = 0; iSlot < MAX_TEXTURE_SLOTS; iSlot++ )
        {
            if( maTextureSlots[iSlot].mpiGLTexID == NULL )
            {
                break;
            }
        }	// for slot = 0 to max slots
        
        //printf( "find FREE spot for texture %s\n", pTexture->mszName );
        
        // put in slot
        if( iSlot < MAX_TEXTURE_SLOTS )
        {
            // found a FREE spot, place the texture there
            //printf( "found FREE texture spot at %d\n", iSlot );
            
            maTextureSlots[iSlot].mpiGLTexID = &maiTextureIDs[iSlot];
            maTextureSlots[iSlot].mpTexture = pTexture;
            
            pTexture->mbInMem = true;
            pTexture->miID = iSlot;
        }
        
        glBindTexture( GL_TEXTURE_2D, maiTextureIDs[iSlot] );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, pTexture->miTextureEdgeTypeU );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, pTexture->miTextureEdgeTypeV );
        
        int iValidWidth = 1;
        int iValidHeight = 1;
        for( int i = 0; i < 32; i++ )
        {
            if( iValidWidth >= pTexture->miWidth )
            {
                break;
            }
            
            iValidWidth = ( iValidWidth << 1 );
        }
        
        for( int i = 0; i < 32; i++ )
        {
            if( iValidHeight >= pTexture->miHeight )
            {
                break;
            }
            
            iValidHeight = ( iValidHeight << 1 );
        }
        
        if( iValidWidth != pTexture->miWidth || iValidWidth != pTexture->miHeight )
        {
            // need to convert to power of 2
            
            unsigned char* acGLImageData = (unsigned char *)MALLOC( iValidWidth * iValidHeight * 4 );
            memset( acGLImageData, 0, sizeof( char ) * iValidWidth * iValidHeight * 4 );
            for( int i = 0; i < pTexture->miHeight; i++ )
            {
                int iOrigIndex = i * pTexture->miWidth * 4;
                int iIndex = i * iValidWidth * 4;
                memcpy( &acGLImageData[iIndex], &acImage[iOrigIndex], sizeof( char ) * pTexture->miWidth * 4 );
            }
            
            pTexture->miGLWidth = iValidWidth;
            pTexture->miGLHeight = iValidHeight;
            
            glTexImage2D( GL_TEXTURE_2D, 
                         0, 
                         GL_RGBA, 
                         pTexture->miGLWidth, pTexture->miGLHeight, 
                         0, 
                         GL_RGBA, 
                         GL_UNSIGNED_BYTE, 
                         acGLImageData );
            
            FREE( acGLImageData );
        }
        else
        {
            pTexture->miGLWidth = pTexture->miWidth;
            pTexture->miGLHeight = pTexture->miHeight;
            glTexImage2D( GL_TEXTURE_2D, 
                         0, 
                         GL_RGBA, 
                         pTexture->miGLWidth, pTexture->miGLHeight, 
                         0, 
                         GL_RGBA, 
                         GL_UNSIGNED_BYTE, 
                         acImage );
        }
        
        
        strncpy( pTexture->mszName, szTextureName, sizeof( pTexture->mszName ) );
        pTexture->miHash = hash( pTexture->mszName );
        
        ++miNumTextures;
    }
    
   
}

/*
**
*/
tTexture* CTextureManager::getTexture( const char* szTextureName, bool bLoadInMem )
{
    int iHash = hash( szTextureName );
	for( int i = 0; i < miNumTextures; i++ )
	{
		assert( i >= 0 && i < sizeof( maTextures ) / sizeof( *maTextures ) );
		
		//if( !strcmp( maTextures[i].mszName, szTextureName ) )
		if( maTextures[i].miHash == iHash )
        {
			if( strcmp( szTextureName, maTextures[i].mszName ) )
			{
				continue;
			}

            double fCurrTime = getCurrTime();
            if( bLoadInMem )
            {
                loadTextureInMem( &maTextures[i], fCurrTime );
			}
              
            return &maTextures[i];
		}
	}
	
	return NULL;
}

/*
**
*/
void CTextureManager::loadTextureInMem( tTexture* pTexture, double fCurrTime )
{
	assert( pTexture );
	pTexture->mfLastAccessed = fCurrTime;
	
	// check if the texture is already in memory
	if( pTexture->miID > 0 )
	{
        return;
	}
	
	// texture is not in memory, check for FREE spot
	int iSlot = 0;
	for( iSlot = 0; iSlot < MAX_TEXTURE_SLOTS; iSlot++ )
	{
		if( maTextureSlots[iSlot].mpiGLTexID == NULL )
		{
			break;
		}
	}	// for slot = 0 to max slots
	
	//printf( "find FREE spot for texture %s\n", pTexture->mszName );
	
	// put in slot
	if( iSlot < MAX_TEXTURE_SLOTS )
	{
		// found a FREE spot, place the texture there
		//printf( "found FREE texture spot at %d\n", iSlot );
		
		maTextureSlots[iSlot].mpiGLTexID = &maiTextureIDs[iSlot];
		maTextureSlots[iSlot].mpTexture = pTexture;
		
		// load the image
		swapInTexture( pTexture->mszName, pTexture, maiTextureIDs[iSlot] );	
		pTexture->mbInMem = true;
	}	// if has FREE slots
	else
	{
		// no FREE spot, find the most recent slot
		int iOldestTex = 0;
		double fAccessed = pTexture->mfLastAccessed;
		for( int i = 1; i < MAX_TEXTURE_SLOTS; i++ )
		{
			// save the older texture
			if( maTextureSlots[i].mpTexture->mfLastAccessed < fAccessed )
			{
				//printf( "texture %s last accessed=%f\n", pTextureManager->maTextureSlots[i].mpTexture->mszName, pTextureManager->maTextureSlots[i].mpTexture->mfLastAccessed );
				iOldestTex = i;
				fAccessed = maTextureSlots[i].mpTexture->mfLastAccessed;
			}
		}
		
		//printf( "no FREE spot, swap from slot %d texture=%s\n", iOldestTex, pTextureManager->maTextureSlots[iOldestTex].mpTexture->mszName );
		
		// release old texture
		GLuint aDelTexture[1];
		aDelTexture[0] = *maTextureSlots[iOldestTex].mpiGLTexID;
		glDeleteTextures( 1, aDelTexture );
		
		// save and swap out
		int iTexID = maTextureSlots[iOldestTex].mpTexture->miID;
		maTextureSlots[iOldestTex].mpTexture->miID = -1;
		maTextureSlots[iOldestTex].mpTexture->mbInMem = false;
		
		// set to new texture
		maTextureSlots[iOldestTex].mpTexture = pTexture;
		pTexture->mbInMem = false;
		
		// swap in
		swapInTexture( pTexture->mszName, pTexture, iTexID );
	}	// else no more slots
    
}

/*
**
*/
void CTextureManager::swapInTexture( const char* szTextureName, tTexture* pTexture, unsigned int iSwapTex )
{
	unsigned char* aImageData = NULL;
	long iFileSize = 0;
    
    if( pTexture->mbCubeMap )
    {
        char szCubeTextureName[256];
        char szNoExtension[128];
        
        const char* szExtensionStart = strstr( szTextureName, ".tga" );
        size_t iSize = (size_t)szExtensionStart - (size_t)szTextureName;
        memset( szNoExtension, 0, sizeof( szNoExtension ) );
        memcpy( szNoExtension, szTextureName, iSize );
        
        glBindTexture( GL_TEXTURE_CUBE_MAP, iSwapTex );
        glEnable( GL_TEXTURE_CUBE_MAP );
        glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        
        glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, pTexture->miTextureEdgeTypeU );
        glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, pTexture->miTextureEdgeTypeV );
        
        glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        
        if( strstr( szTextureName, ".tga" ) )
        {
            for( int i = 0; i < 6; i++ )
            {
                GLenum eFace = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
                switch( i )
                {
                    case 0:     // pos z
                        snprintf( szCubeTextureName, sizeof( szCubeTextureName ), "%s_z_pos.tga", szNoExtension );
                        eFace = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
                        break;
                        
                    case 1:     // neg z
                        snprintf( szCubeTextureName, sizeof( szCubeTextureName ), "%s_z_neg.tga", szNoExtension );
                        eFace = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
                        break;
                        
                    case 2:     // pos y
                        snprintf( szCubeTextureName, sizeof( szCubeTextureName ), "%s_y_pos.tga", szNoExtension );
                        eFace = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
                        break;
                        
                    case 3:     // neg y
                        snprintf( szCubeTextureName, sizeof( szCubeTextureName ), "%s_y_neg.tga", szNoExtension );
                        eFace = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
                        break;
                        
                    case 4:     // pos x
                        snprintf( szCubeTextureName, sizeof( szCubeTextureName ), "%s_x_pos.tga", szNoExtension );
                        eFace = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
                        break;
                        
                    case 5:     // neg x
                        snprintf( szCubeTextureName, sizeof( szCubeTextureName ), "%s_x_neg.tga", szNoExtension );
                        eFace = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
                        break;
                        
                }
                
                int iWidth, iHeight;
                unsigned char* acImage = NULL;
                readTarga( szCubeTextureName, &iWidth, &iHeight, &acImage );
                WTFASSERT2( acImage, "can't load texture: %s", szCubeTextureName );
                
                int iValidWidth = 1;
                int iValidHeight = 1;
                for( int i = 0; i < 32; i++ )
                {
                    if( iValidWidth >= pTexture->miWidth )
                    {
                        break;
                    }
                    
                    iValidWidth = ( iValidWidth << 1 );
                }
                
                for( int i = 0; i < 32; i++ )
                {
                    if( iValidHeight >= pTexture->miHeight )
                    {
                        break;
                    }
                    
                    iValidHeight = ( iValidHeight << 1 );
                }
                
                GLenum error = glGetError();
                if( iValidWidth != pTexture->miWidth || iValidWidth != pTexture->miHeight )
                {
                    // need to convert to power of 2
                    
                    unsigned char* acGLImageData = (unsigned char *)MALLOC( iValidWidth * iValidHeight * 4 );
                    memset( acGLImageData, 0, sizeof( char ) * iValidWidth * iValidHeight * 4 );
                    for( int i = 0; i < pTexture->miHeight; i++ )
                    {
                        int iOrigIndex = i * pTexture->miWidth * 4;
                        int iIndex = i * iValidWidth * 4;
                        memcpy( &acGLImageData[iIndex], &acImage[iOrigIndex], sizeof( char ) * pTexture->miWidth * 4 );
                    }
                    
                    pTexture->miGLWidth = iValidWidth;
                    pTexture->miGLHeight = iValidHeight;
                    
                    glTexImage2D( eFace,
                                 0,
                                 GL_RGBA,
                                 pTexture->miGLWidth, pTexture->miGLHeight,
                                 0,
                                 GL_RGBA, 
                                 GL_UNSIGNED_BYTE, 
                                 acGLImageData );
                    
                    //glGenerateMipmap( GL_TEXTURE_2D );
                    
                    FREE( acGLImageData );
                }
                else
                {
                    pTexture->miGLWidth = pTexture->miWidth;
                    pTexture->miGLHeight = pTexture->miHeight;
                    glTexImage2D( eFace,
                                  0,
                                  GL_RGBA,
                                  pTexture->miWidth, pTexture->miHeight,
                                  0,
                                  GL_RGBA,
                                  GL_UNSIGNED_BYTE,
                                  acImage );
                    
                    //glGenerateMipmap( GL_TEXTURE_2D );
                    
                    error = glGetError();
                    assert( error == GL_NONE );
                }
                
                FREE( acImage );
                
            }   // for i = 0 to 6
        }
        
    }   // if cube map
    else
    {
        if( strstr( szTextureName, ".tga" ) )
        {
            readTarga( szTextureName, &pTexture->miWidth, &pTexture->miHeight, &
                      aImageData );
            
            pTexture->miBPP = 8;
        }
        else if( strstr( szTextureName, ".png" ) )
        {
            unsigned int iWidth, iHeight;
            
            char szFullPath[512];
            getFullPath( szFullPath, szTextureName );
            unsigned int iError = LodePNG_decode32_file( &aImageData, &iWidth, &iHeight, szFullPath );
            if( iError != 0 )
            {
                getWritePath( szFullPath, szTextureName );
                iError = LodePNG_decode32_file( &aImageData, &iWidth, &iHeight, szFullPath );
            }
            
            WTFASSERT2( iError == 0, "error loading %s", szTextureName );
            
            pTexture->miWidth = (int)iWidth;
            pTexture->miHeight = (int)iHeight;
            pTexture->miBPP = 8;
        }
        else if( strstr( szTextureName, ".pvr" ) )
        {
            unsigned int iWidth, iHeight, iBPP;
            readPVR( szTextureName, &iWidth, &iHeight, &aImageData, &iBPP, &iFileSize );
            
            pTexture->miWidth = (int)iWidth;
            pTexture->miHeight = (int)iHeight;
            pTexture->miBPP = (int)iBPP;
            
            pTexture->mbCompressed = true;
        }
        else
        {
            char szFullName[128];
            sprintf( szFullName, "%s.tga", szTextureName );
            readTarga( szFullName, &pTexture->miWidth, &pTexture->miHeight, &aImageData );
        }
        
        assert( aImageData );
        
    #if defined( MULTI_THREAD )
        while( mbSemaphore );
    #endif // MULTI_THREAD
        mbSemaphore = true;
        
        strcpy( pTexture->mszName, szTextureName );
        glBindTexture(GL_TEXTURE_2D, iSwapTex );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, pTexture->miTextureEdgeTypeU );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, pTexture->miTextureEdgeTypeV );
        
        if( pTexture->mbCompressed )
        {
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            
            GLenum error = glGetError();
            
            pTexture->miGLWidth = pTexture->miWidth;
            pTexture->miGLHeight = pTexture->miHeight;
            
            if( pTexture->miBPP == 2 )
            {
#if !defined( WINDOWS ) && !defined( MACOS )
                glCompressedTexImage2D( GL_TEXTURE_2D, 
                                        0, 
                                        GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG, 
                                        pTexture->miWidth, 
                                        pTexture->miHeight, 
                                        0, 
                                        (GLsizei)iFileSize, 
                                        aImageData );
#endif // WINDOWS
                //glGenerateMipmap( GL_TEXTURE_2D );
            }
            else if( pTexture->miBPP == 4 )
            {
#if !defined( WINDOWS ) && !defined( MACOS )
                glCompressedTexImage2D( GL_TEXTURE_2D, 
                                        0, 
                                        GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, 
                                        pTexture->miWidth, 
                                        pTexture->miHeight, 
                                        0, 
                                        (GLsizei)iFileSize, 
                                        aImageData );
#endif // WINDOWS
                //glGenerateMipmap( GL_TEXTURE_2D );
            }
            else
            {
                assert( 0 );
            }
            
            //glGenerateMipmap( GL_TEXTURE_2D );
            
            error = glGetError();
        }
        else
        {
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
         
            int iValidWidth = 1;
            int iValidHeight = 1;
            for( int i = 0; i < 32; i++ )
            {
                if( iValidWidth >= pTexture->miWidth )
                {
                    break;
                }
                
                iValidWidth = ( iValidWidth << 1 );
            }
            
            for( int i = 0; i < 32; i++ )
            {
                if( iValidHeight >= pTexture->miHeight )
                {
                    break;
                }
                
                iValidHeight = ( iValidHeight << 1 );
            }
            
            if( iValidWidth != pTexture->miWidth || iValidWidth != pTexture->miHeight )
            {
                // need to convert to power of 2
                
                unsigned char* acGLImageData = (unsigned char *)MALLOC( iValidWidth * iValidHeight * 4 );
                memset( acGLImageData, 0, sizeof( char ) * iValidWidth * iValidHeight * 4 );
                for( int i = 0; i < pTexture->miHeight; i++ )
                {
                    int iOrigIndex = i * pTexture->miWidth * 4;
                    int iIndex = i * iValidWidth * 4;
                    memcpy( &acGLImageData[iIndex], &aImageData[iOrigIndex], sizeof( char ) * pTexture->miWidth * 4 );
                }
                
                pTexture->miGLWidth = iValidWidth;
                pTexture->miGLHeight = iValidHeight;
                
                
                glTexImage2D( GL_TEXTURE_2D, 
                              0, 
                              GL_RGBA, 
                              pTexture->miGLWidth, pTexture->miGLHeight, 
                              0, 
                              GL_RGBA, 
                              GL_UNSIGNED_BYTE, 
                              acGLImageData );
                
                //glGenerateMipmap( GL_TEXTURE_2D );
                
                FREE( acGLImageData );
            }
            else
            {
                // don't need to convert to power of 2
                
                pTexture->miGLWidth = pTexture->miWidth;
                pTexture->miGLHeight = pTexture->miHeight;
                
                
                glTexImage2D( GL_TEXTURE_2D, 
                             0, 
                             GL_RGBA, 
                             pTexture->miGLWidth, pTexture->miGLHeight, 
                             0, 
                             GL_RGBA, 
                             GL_UNSIGNED_BYTE, 
                             aImageData );
                
                //glGenerateMipmap( GL_TEXTURE_2D );
            }
        }

        FREE( aImageData );
    }   // else !cube map
	
    pTexture->miID = iSwapTex;
    
    mbSemaphore = false;
}

/*
**
*/
void CTextureManager::reloadTexture( tTexture* pTexture )
{
    for( int i = 0; i < MAX_TEXTURE_SLOTS; i++ )
    {
		if( maTextureSlots[i].mpiGLTexID &&
			*maTextureSlots[i].mpiGLTexID == pTexture->miID )
        {
            GLuint aDelTexture[1];
            aDelTexture[0] = *maTextureSlots[i].mpiGLTexID;
            glDeleteTextures( 1, aDelTexture );
            maTextureSlots[i].mpiGLTexID = NULL;
            
            maTextureSlots[i].mpTexture->miID = -1;
            maTextureSlots[i].mpTexture->mbInMem = false;
            
            break;
        }
    }
}

/*
**
*/
void CTextureManager::purgeTextures( void )
{
    for( int i = 0; i < MAX_TEXTURE_SLOTS; i++ )
    {
        GLuint aDelTexture[1];
        if( maTextureSlots[i].mpiGLTexID )
        {
            aDelTexture[0] = *maTextureSlots[i].mpiGLTexID;
            glDeleteTextures( 1, aDelTexture );
            maTextureSlots[i].mpiGLTexID = NULL;
            
            maTextureSlots[i].mpTexture->miID = -1;
            maTextureSlots[i].mpTexture->mbInMem = false;
        }
    }
}

/*
**
*/
void CTextureManager::reloadTextureData( const char* szTextureName, unsigned char const* acData )
{
	tTexture* pTexture = getTexture( szTextureName );
	WTFASSERT2( pTexture, "can't find texture: %s", szTextureName );
	
	glBindTexture(GL_TEXTURE_2D, pTexture->miID );
	glTexImage2D( GL_TEXTURE_2D, 
                  0, 
                  GL_RGBA, 
                  pTexture->miGLWidth, pTexture->miGLHeight, 
                  0, 
                  GL_RGBA, 
                  GL_UNSIGNED_BYTE, 
                  acData );
}

/*
**
*/
void CTextureManager::releaseTexture( tTexture* pTexture )
{
	for( int i = 1; i < MAX_TEXTURE_SLOTS; i++ )
	{
		// save the older texture
		if( maTextureSlots[i].mpTexture == pTexture )
		{
			GLuint aiDelTexture[] = { *maTextureSlots[i].mpiGLTexID };
            glDeleteTextures( 1, aiDelTexture );
            maTextureSlots[i].mpiGLTexID = NULL;
            
            maTextureSlots[i].mpTexture->miID = -1;
            maTextureSlots[i].mpTexture->mbInMem = false;
			
			break;
		}

	}	// for i = 1 to max texture slots
}