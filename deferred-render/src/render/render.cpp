//
//  render.cpp
//  ProjectG
//
//  Created by Dingwings on 11/6/13.
//  Copyright (c) 2013 Dingwings. All rights reserved.
//

#include "render.h"
#include "vector.h"
#include "batchsprite.h"
#include "texturemanager.h"

static int siScreenWidth = SCREEN_WIDTH;
static int siScreenHeight = SCREEN_HEIGHT;
static float sfScreenScale = 1.0f;

/*
**
*/
int renderGetScreenWidth( void )
{
    return siScreenWidth;
}

/*
**
*/
int renderGetScreenHeight( void )
{
    return siScreenHeight;
}

/*
**
*/
float renderGetScreenScale( void )
{
	return sfScreenScale;
}

/*
**
*/
void renderQuad( const char* szTextureName,
                tVector4 const* aTransformedV,
                tVector4 const* aColor,
                tVector2 const* aUV,
                int iShader,
				GLuint iSrcBlend,
				GLuint iDestBlend )
{
    CBatchManager* pBatchManager = CBatchManager::instance();
	GLuint iTextureID = CTextureManager::instance()->getTexture( szTextureName )->miID;

	GLuint aiTexID[] = { iTextureID, 0, 0, 0 };
    pBatchManager->addSprite( aiTexID,
                              aTransformedV,
                              aColor,
                              aUV,
                              iShader,
							  iSrcBlend,
							  iDestBlend );
}

/*
**
*/
void renderSetScreenWidth( int iScreenWidth )
{
	siScreenWidth = iScreenWidth;
}

/*
**
*/
void renderSetScreenHeight( int iScreenHeight )
{
	siScreenHeight = iScreenHeight;
}

/*
**
*/
void renderSetScreenScale( float fScreenScale )
{
	sfScreenScale = fScreenScale;
}

/*
**
*/
void renderSetToUseScissorBatch( bool bScissor,
                                 int iX,
                                 int iY,
                                 int iWidth,
                                 int iHeight )
{
    CBatchManager::instance()->setScissorState( bScissor, iX, iY, iWidth, iHeight );
}