#ifndef __FBOTEXTUREATLAS_H__
#define __FBOTEXTUREATLAS_H__

#include "modelinstance.h"

struct FBOTextureAtlas
{
	GLuint				miFBO;
	GLuint				miTextureID;
};

typedef struct FBOTextureAtlas tFBOTextureAtlas;

struct TextureAtlasInfoBucket
{
	tTextureAtlasInfo*		maBuckets;
	tFBOTextureAtlas**		mapFBO;
	int						miNumBuckets;
	int						miNumBucketAlloc;
	tModelInstance**		mapModelInstances;

	int						miCurrTexture;
};

typedef TextureAtlasInfoBucket tTextureAtlasInfoBucket;

struct FBOTextureAtlasBucket
{
	tFBOTextureAtlas*		maFBOAtlas;
	int						miNumFBOAtlas;
	int						miNumFBOAtlasAlloc;
};

typedef struct FBOTextureAtlasBucket tFBOTextureAtlasBucket;

void fboTextureAtlasInitGL( GLuint* piFBO,
							GLuint* piTextureID,
							int iWidth,
							int iHeight );

tTextureAtlasInfo const* fboTextureAtlasAdd( const char* szName, int iWidth, int iHeight );

void fboTextureAtlasRender( tFBOTextureAtlas const* pFBOAtlas );

GLint fboTextureAtlasGetTextureID( int iIndex );
void fboTextureAtlasSetModelInstance( tTextureAtlasInfo const* pInfo,
									  tModelInstance* pModelInstance );

tTextureAtlasInfo const* fboTextureAtlasGetAtlasInfo( const char* szName );

void fboTextureAtlasClearAll( void );
void fboTextureRenderAll( void );

#endif // __FBOTEXTUREATLAS_H__