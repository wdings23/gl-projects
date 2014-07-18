#include "fbotextureatlas.h"
#include "hashutil.h"
#include "render.h"
#include "tga.h"
#include "shadermanager.h"
#include "timeutil.h"

#define MAX_ATLAS_TEXTURES 4		// maximum number of gl textures for atlas

static tFBOTextureAtlasBucket*			spFBOAtlasBucket = NULL;
static tTextureAtlasInfoBucket*			spAtlasBucket = NULL;

static tTextureAtlasInfo	sSwappedOutInfo = 
{ 
	NULL, "swapped_out", 0, { 0.0f, 0.0f }, { 0.0f, 0.0f }, 0, 0, 0, 0.0, false
};

static tTextureAtlasInfo* swapInTextureAtlasInfo( const char* szTextureName );
static int getAtlasIndex( tTextureAtlasInfo const* pAtlas );

/*
**
*/
void fboTextureAtlasInitGL( GLuint* piFBO,
							GLuint* piTextureID,
							int iWidth,
							int iHeight )
{
	// texture
	glGenTextures( 1, piTextureID );
	glBindTexture( GL_TEXTURE_2D, *piTextureID );
	glTexImage2D( GL_TEXTURE_2D,
				  0,
				  GL_RGBA,
				  iWidth,
				  iHeight,
				  0,
				  GL_RGBA,
				  GL_UNSIGNED_BYTE,
				  NULL );
    
	// attribute
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    
	// bind fbo to texture
	glGenFramebuffers( 1, piFBO );
	glBindFramebuffer( GL_FRAMEBUFFER, *piFBO );
	glFramebufferTexture2D( GL_FRAMEBUFFER,
							GL_COLOR_ATTACHMENT0,
							GL_TEXTURE_2D,
							*piTextureID,
							0 );

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

/*
**
*/
tTextureAtlasInfo const* fboTextureAtlasAdd( const char* szName, int iWidth, int iHeight )
{
	const int iAtlasSize = 1024;
	
	// initialize bucket
	if( spAtlasBucket == NULL )
	{
		spAtlasBucket = (tTextureAtlasInfoBucket *)malloc( sizeof( tTextureAtlasInfoBucket ) );
		
		spAtlasBucket->miNumBuckets = 0;
		spAtlasBucket->miCurrTexture = 0;
		spAtlasBucket->miNumBucketAlloc = ( iAtlasSize / 128 ) * ( iAtlasSize / 128 );

		spAtlasBucket->maBuckets = (tTextureAtlasInfo *)MALLOC( sizeof( tTextureAtlasInfo ) * spAtlasBucket->miNumBucketAlloc );
		spAtlasBucket->mapFBO = (tFBOTextureAtlas **)MALLOC( sizeof( tFBOTextureAtlas* ) * spAtlasBucket->miNumBucketAlloc );
		spAtlasBucket->mapModelInstances = (tModelInstance **)MALLOC( sizeof( tModelInstance* ) * spAtlasBucket->miNumBucketAlloc );
		
		memset( spAtlasBucket->mapModelInstances, 0, sizeof( tModelInstance* ) * spAtlasBucket->miNumBucketAlloc );

		spFBOAtlasBucket = (tFBOTextureAtlasBucket *)MALLOC( sizeof( tFBOTextureAtlasBucket ) );

		spFBOAtlasBucket->miNumFBOAtlas = 0;
		spFBOAtlasBucket->miNumFBOAtlasAlloc = 100;
		spFBOAtlasBucket->maFBOAtlas = (tFBOTextureAtlas *)MALLOC( sizeof( tFBOTextureAtlas ) * spFBOAtlasBucket->miNumFBOAtlasAlloc );
		
		for( int i = 0; i < spFBOAtlasBucket->miNumFBOAtlasAlloc; i++ )
		{
			spFBOAtlasBucket->maFBOAtlas[i].miFBO = 0;
			spFBOAtlasBucket->maFBOAtlas[i].miTextureID = 0;

		}	// for i = 0 to num fbo atlas alloc
	}
	
	// need to allocate more spaces
	if( spAtlasBucket->miNumBuckets >= spAtlasBucket->miNumBucketAlloc )
	{
		int iNumAtlas = iAtlasSize / 128;
		int iNumPrevBucketAlloc = spAtlasBucket->miNumBucketAlloc;

		spAtlasBucket->miNumBucketAlloc += ( iNumAtlas * iNumAtlas );
		spAtlasBucket->maBuckets = (tTextureAtlasInfo *)REALLOC( spAtlasBucket->maBuckets, sizeof( tTextureAtlasInfo ) * spAtlasBucket->miNumBucketAlloc );
		spAtlasBucket->mapFBO = (tFBOTextureAtlas **)REALLOC( spAtlasBucket->mapFBO, sizeof( tFBOTextureAtlas* ) * spAtlasBucket->miNumBucketAlloc );
		
		// set modelinstance's drawtexture info to new ones
		for( int i = 0; i < spAtlasBucket->miNumBuckets; i++ )
		{
			spAtlasBucket->mapModelInstances[i]->mpDrawTextureAtlasInfo = &spAtlasBucket->maBuckets[i];
		}

		spAtlasBucket->mapModelInstances = (tModelInstance **)REALLOC( spAtlasBucket->mapModelInstances, sizeof( tModelInstance* ) * spAtlasBucket->miNumBucketAlloc );
		for( int i = iNumPrevBucketAlloc; i < spAtlasBucket->miNumBucketAlloc; i++ )
		{
			spAtlasBucket->mapFBO[i]->miFBO = 0;
			spAtlasBucket->mapFBO[i]->miTextureID = 0;

			spAtlasBucket->mapModelInstances[i] = NULL;
		}
	}

	tTextureAtlasInfo* pLastAtlas = NULL;
	
	// see if it's already registered
	tTextureAtlasInfo* pAtlas = NULL;
	tFBOTextureAtlas* pFBOAtlas = NULL;

	int iHash = hash( szName );
	for( int i = 0; i < spAtlasBucket->miNumBuckets; i++ )
	{
		// found the bucket
		if( spAtlasBucket->maBuckets[i].miHashID == iHash )
		{
			pAtlas = &spAtlasBucket->maBuckets[i];
			if( i - 1 >= 0 )
			{
				pLastAtlas = &spAtlasBucket->maBuckets[i-1];
			}

			pFBOAtlas = spAtlasBucket->mapFBO[i];
			pAtlas->mfTimeAccessed = getTime();
			pAtlas->mbInMemory = true;
			break;
		}
	}
	
	// add new atlas
	if( pAtlas == NULL )
	{
		pAtlas = swapInTextureAtlasInfo( szName );
	}

	int iNumQuadsInAtlas = ( iAtlasSize / 128 ) * ( iAtlasSize / 128 ); 
	int iIndex = getAtlasIndex( pAtlas );
	int iFBOIndex = iIndex / iNumQuadsInAtlas; 

	WTFASSERT2( iIndex >= 0, "invalid atlas index" );
	pFBOAtlas = &spFBOAtlasBucket->maFBOAtlas[iFBOIndex];

	// render the atlas texture
	fboTextureAtlasRender( pFBOAtlas );
	pAtlas->miTextureID = spFBOAtlasBucket->maFBOAtlas[spAtlasBucket->miCurrTexture].miTextureID;
	pAtlas->miTextureAtlasWidth = iAtlasSize;
	pAtlas->miTextureAtlasHeight = iAtlasSize;
    
	return pAtlas;
}

/*
**
*/
GLuint siTexID = 0;
void fboTextureAtlasRender( tFBOTextureAtlas const* pFBOAtlas )
{
#if 0
	const int iTextureSize = 128;

	glBindFramebuffer( GL_FRAMEBUFFER, pFBOAtlas->miFBO );
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	glViewport( 0, 0, 1024, 1024 );

	glDisable( GL_CULL_FACE );
	
	// create new texture with data
	if( siTexID == 0 )
	{
		glGenTextures( 1, &siTexID );
	}

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, siTexID );
	
	// attribute
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

	glDisable( GL_BLEND );

	// draw texture as quads
	for( int i = 0; i < spAtlasBucket->miNumBuckets; i++ )
	{
		tTextureAtlasInfo* pInfo = &spAtlasBucket->maBuckets[i];
		
		// need the same texture atlas fbo
		if( spAtlasBucket->mapFBO[i] != pFBOAtlas )
		{
			continue;
		}

		// update new data
		glTexImage2D( GL_TEXTURE_2D,
					  0,
					  GL_RGBA,
					  iTextureSize,
					  iTextureSize,
					  0,
					  GL_RGBA,
					  GL_UNSIGNED_BYTE,
					  acImage );

		float fTop = pInfo->mTopLeft.fY * 2.0f - 1.0f;
		float fBottom = pInfo->mBottomRight.fY * 2.0f - 1.0f;
		float fLeft = pInfo->mTopLeft.fX * 2.0f - 1.0f;
		float fRight = pInfo->mBottomRight.fX * 2.0f - 1.0f;

		tVector4 aScreenVerts[] =
		{
			{ fLeft, fTop, 0.0f, 1.0f },
			{ fLeft, fBottom, 0.0f, 1.0f },
			{ fRight, fTop, 0.0f, 1.0f },
			{ fRight, fBottom, 0.0f, 1.0f }
		};

		float afColor[] =
		{
			1.0f, 1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f, 1.0f,
		};
	    
		tVector2 aUV[] =
		{
			{ 0.0f, 0.0f },
			{ 0.0f, 1.0f },
			{ 1.0f, 0.0f },
			{ 1.0f, 1.0f },
		};
	    
		int iShader = CShaderManager::instance()->getShader( "ui" );
	    
		glUseProgram( iShader );
	    
		// position, color semantic
		GLint iPos = glGetAttribLocation( iShader, "position" );
		GLint iColor = glGetAttribLocation( iShader, "color" );
		GLint iUV = glGetAttribLocation( iShader, "textureUV" );
	    
		glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aScreenVerts );
		glVertexAttribPointer( iUV, 2, GL_FLOAT, GL_FALSE, 0, aUV );
		glVertexAttribPointer( iColor, 4, GL_FLOAT, GL_FALSE, 0, afColor );
		
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	}

	glEnable( GL_CULL_FACE );
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glViewport( 0, 0, renderGetScreenWidth() * renderGetScreenScale(), renderGetScreenHeight() * renderGetScreenScale() );
#endif // #if 0
}

/*
**
*/
GLint fboTextureAtlasGetTextureID( int iIndex )
{
	GLint iRet = -1;
	if( spFBOAtlasBucket && spFBOAtlasBucket->maFBOAtlas )
	{
		iRet = spFBOAtlasBucket->maFBOAtlas[iIndex].miTextureID;
	}
	
	return iRet;
}

/*
**
*/
void fboTextureAtlasSetModelInstance( tTextureAtlasInfo const* pInfo,
									  tModelInstance* pModelInstance )
{
	bool bFound = false;
	for( int i = 0; i < spAtlasBucket->miNumBuckets; i++ )
	{
		if( pInfo == &spAtlasBucket->maBuckets[i] )
		{
			spAtlasBucket->mapModelInstances[i] = pModelInstance;
			bFound = true;
			break;
		}
	}

	WTFASSERT2( bFound, "did not find %s", pInfo->mszTextureName );

	pModelInstance->mpDrawTextureAtlasInfo = pInfo;
}

/*
**
*/
tTextureAtlasInfo const* fboTextureAtlasGetAtlasInfo( const char* szName )
{
	// see if it's already registered
	tTextureAtlasInfo* pAtlas = NULL;
	int iHash = hash( szName );
	for( int i = 0; i < spAtlasBucket->miNumBuckets; i++ )
	{
		if( spAtlasBucket->maBuckets[i].miHashID == iHash )
		{
			pAtlas = &spAtlasBucket->maBuckets[i];
			break;
		}
	}

	return pAtlas;
}

/*
**
*/
void fboTextureAtlasClearAll( void )
{
	if( spAtlasBucket )
	{
		// clear model instance texture atlas
		for( int i = 0; i < spAtlasBucket->miNumBuckets; i++ )
		{
			if( spAtlasBucket->mapModelInstances[i] )
			{
				spAtlasBucket->mapModelInstances[i]->mpDrawTextureAtlasInfo = NULL;
				spAtlasBucket->mapModelInstances[i] = NULL;
			}

			spAtlasBucket->mapFBO[i] = NULL;
		}

		spAtlasBucket->miNumBuckets = 0;
		spAtlasBucket->miCurrTexture = 0;
	}
	
	if( spFBOAtlasBucket )
	{
		spFBOAtlasBucket->miNumFBOAtlas = 0;
	}

}

/*
**
*/
void fboTextureRenderAll( void )
{
	if( spAtlasBucket )
	{
		for( int i = 0; i <= spAtlasBucket->miNumBuckets; i++ )
		{
			if( spAtlasBucket->mapFBO[i] )
			{
				fboTextureAtlasRender( spAtlasBucket->mapFBO[i] );
			}
		}
	}
}

/*
**
*/
static tTextureAtlasInfo* swapInTextureAtlasInfo( const char* szTextureName )
{
	const int iTextureSize = 128;
	const int iAtlasSize = 1024;

	int iMaxNumBuckets = ( iAtlasSize / 128 ) * ( iAtlasSize / 128 ) * MAX_ATLAS_TEXTURES;

	tTextureAtlasInfo* pSelected = NULL;
	if( spAtlasBucket->miNumBuckets + 1 >= iMaxNumBuckets )
	{
		tTextureAtlasInfo* pOldest = &spAtlasBucket->maBuckets[0];
		tModelInstance* pOldestModelInstance = spAtlasBucket->mapModelInstances[0];

		// find oldest time accessed
		for( int i = 0; i < spAtlasBucket->miNumBuckets; i++ )
		{
			// swap out
			tTextureAtlasInfo* pInfo = &spAtlasBucket->maBuckets[i];
			if( pInfo->mfTimeAccessed < pOldest->mfTimeAccessed )
			{
				pOldest = pInfo;
				pOldestModelInstance = spAtlasBucket->mapModelInstances[i];
			}

		}	// for i = 0 to num buckets
		
		strncpy( pOldest->mszTextureName, szTextureName, sizeof( pOldest->mszTextureName ) ); 
		pOldestModelInstance->mpDrawTextureAtlasInfo = &sSwappedOutInfo;

		pSelected = pOldest;

	}	// if over allocation
	else
	{
		pSelected = &spAtlasBucket->maBuckets[spAtlasBucket->miNumBuckets];

		// scale to ( 0.0, 1.0 )
		float fWidthAtlasRatio = (float)iTextureSize / (float)iAtlasSize;
		float fHeightAtlasRatio = (float)iTextureSize / (float)iAtlasSize;
		float fOneOverAtlasSize = 1.0f / (float)iAtlasSize;
		
		// add new atlas
		{
			if( spAtlasBucket->miNumBuckets == 0 )
			{
				// first ever texture in the atlas

				pSelected->mTopLeft.fX = fOneOverAtlasSize;
				pSelected->mTopLeft.fY = fOneOverAtlasSize;

				pSelected->mBottomRight.fX = pSelected->mTopLeft.fX + fWidthAtlasRatio;
				pSelected->mBottomRight.fY = pSelected->mTopLeft.fY + fHeightAtlasRatio;
				
				// create the total atlas texture in gl
				if( spFBOAtlasBucket->maFBOAtlas[spAtlasBucket->miCurrTexture].miFBO == 0 &&
					spFBOAtlasBucket->maFBOAtlas[spAtlasBucket->miCurrTexture].miTextureID == 0 )
				{
					fboTextureAtlasInitGL( &spFBOAtlasBucket->maFBOAtlas[spAtlasBucket->miCurrTexture].miFBO,
										   &spFBOAtlasBucket->maFBOAtlas[spAtlasBucket->miCurrTexture].miTextureID,
										   iAtlasSize,
										   iAtlasSize );
				}
			}
			else
			{
				tTextureAtlasInfo* pLastAtlas = &spAtlasBucket->maBuckets[spAtlasBucket->miNumBuckets-1];

				pSelected->mTopLeft.fX = pLastAtlas->mBottomRight.fX + 2.0f * fOneOverAtlasSize;
				pSelected->mTopLeft.fY = pLastAtlas->mTopLeft.fY;

				pSelected->mBottomRight.fX = pSelected->mTopLeft.fX + fWidthAtlasRatio;
				pSelected->mBottomRight.fY = pLastAtlas->mBottomRight.fY;

				// next line
				if( pSelected->mTopLeft.fX > 1.0f )
				{
					pSelected->mTopLeft.fX = fOneOverAtlasSize;
					pSelected->mTopLeft.fY += fHeightAtlasRatio + 2.0f * fOneOverAtlasSize;
					
					pSelected->mBottomRight.fX = pSelected->mTopLeft.fX + fWidthAtlasRatio;
					
					// no more room on this current atlas, go to next one
					if( pSelected->mTopLeft.fY > 1.0f )
					{
						pSelected->mTopLeft.fY = fOneOverAtlasSize;
						
						++spAtlasBucket->miCurrTexture;
						
						// create the total atlas texture in gl
						if( spFBOAtlasBucket->maFBOAtlas[spAtlasBucket->miCurrTexture].miFBO == 0 &&
							spFBOAtlasBucket->maFBOAtlas[spAtlasBucket->miCurrTexture].miTextureID == 0 )
						{
							fboTextureAtlasInitGL( &spFBOAtlasBucket->maFBOAtlas[spAtlasBucket->miCurrTexture].miFBO,
												   &spFBOAtlasBucket->maFBOAtlas[spAtlasBucket->miCurrTexture].miTextureID,
												   iAtlasSize,
												   iAtlasSize );
						}
					}

					pSelected->mBottomRight.fY = pSelected->mTopLeft.fY + fHeightAtlasRatio;
				}

			}	// last atlas != null
			
			// use this for overall atlas texture
			spAtlasBucket->mapFBO[spAtlasBucket->miNumBuckets] = &spFBOAtlasBucket->maFBOAtlas[spAtlasBucket->miCurrTexture];
			++spAtlasBucket->miNumBuckets;

		}	// if add new atlas
	}
	
	strncpy( pSelected->mszTextureName, szTextureName, sizeof( pSelected->mszTextureName ) );
	pSelected->miHashID = hash( pSelected->mszTextureName );
	pSelected->mfTimeAccessed = getTime();

	return pSelected;
}

/*
**
*/
static int getAtlasIndex( tTextureAtlasInfo const* pAtlas )
{
	int iRet = -1;
	for( int i = 0; i < spAtlasBucket->miNumBucketAlloc; i++ )
	{
		if( &spAtlasBucket->maBuckets[i] == pAtlas )
		{
			iRet = i;
			break;
		}
	}

	return iRet;
}