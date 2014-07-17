//
//  box.cpp
//  Game3
//
//  Created by Dingwings on 8/14/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "box.h"
#include "texturemanager.h"
#include "textureatlasmanager.h"
#include "render.h"

/*
**
*/
CBox::CBox( void )
{
    mColor.fX = mColor.fY = mColor.fZ = mColor.fW = 1.0f;

	for( int i = 0; i < NUM_BOX_CORNERS; i++ )
	{
		memcpy( &maCornerColors[i], &mColor, sizeof( tVector4 ) );
	}
}

/*
**
*/
CBox::~CBox( void )
{

}

/*
**
*/
void CBox::draw( double fDT, int iScreenWidth, int iScreenHeight )
{
    if( miState == STATE_DISABLED )
	{
		return;
	}
    
    updateMatrix( fDT );
    
    tVector4 color;
    memcpy( &color, &mColor, sizeof( color ) );
    
    color.fX = mColor.fX * mOrigColor.fX;
    color.fY = mColor.fY * mOrigColor.fY;
    color.fZ = mColor.fZ * mOrigColor.fZ;
    color.fW = mColor.fW * mOrigColor.fW;
    
    if( mColor.fW > 0.0f )
    {
        // screen dimension to convert coordinate to ( -1, 1 )
        float fHalfWidth = (float)( iScreenWidth / 2 );
        float fHalfHeight = (float)( iScreenHeight / 2 );
        float fOneOverHalfWidth = 1.0f / fHalfWidth;
        float fOneOverHalfHeight = 1.0f / fHalfHeight;
        
        // sprite quad in local space
        tVector4 quad[] =
        {
            { -mDimension.fX * 0.5f, -mDimension.fY * 0.5f, 0.0f, 1.0f },
            { mDimension.fX * 0.5f, -mDimension.fY * 0.5f, 0.0f, 1.0f },
            { -mDimension.fX * 0.5f, mDimension.fY * 0.5f, 0.0f, 1.0f },
            { mDimension.fX * 0.5f, mDimension.fY * 0.5f, 0.0f, 1.0f },
        };
        
        // transform local space quad to world
        tVector4 xformQuads[4];
        for( int i = 0; i < 4; i++ )
        {
            Matrix44Transform( &xformQuads[i], &quad[i], &mTotalMatrix );
        }
        
        // convert to ( -1, 1 )
        tVector4 aScreenVerts[] =
        {
            { ( xformQuads[0].fX - fHalfWidth ) * fOneOverHalfWidth, ( fHalfHeight - xformQuads[0].fY ) * fOneOverHalfHeight, 0.0f, 1.0f },
            { ( xformQuads[1].fX - fHalfWidth ) * fOneOverHalfWidth, ( fHalfHeight - xformQuads[1].fY ) * fOneOverHalfHeight, 0.0f, 1.0f },
            { ( xformQuads[2].fX - fHalfWidth ) * fOneOverHalfWidth, ( fHalfHeight - xformQuads[2].fY ) * fOneOverHalfHeight, 0.0f, 1.0f },
            { ( xformQuads[3].fX - fHalfWidth ) * fOneOverHalfWidth, ( fHalfHeight - xformQuads[3].fY ) * fOneOverHalfHeight, 0.0f, 1.0f },
        };
        
		CTextureManager* pTextureManager = CTextureManager::instance();
		tTextureAtlasInfo* pAtlasInfo = CTextureAtlasManager::instance()->getTextureAtlasInfo( "default.png" );
		
		tVector2 aUV[] = 
		{
			{ 0.0f, 0.0f },
			{ 1.0, 0.0f },
			{ 0.0f, 1.0f },
			{ 1.0f, 1.0f }
		};
		
		tTexture* pTexture = pTextureManager->getTexture( "default.png" );
		if( pTexture == NULL )
		{
			pTextureManager->registerTexture( "default.png" );
		}

		if( pAtlasInfo )
		{
			pTexture = pTextureManager->getTexture( pAtlasInfo->mpTexture->mszName );
			if( pTexture == NULL )
			{
				pTextureManager->registerTexture( pAtlasInfo->mszTextureName );
				pTexture = pTextureManager->getTexture( pAtlasInfo->mszTextureName );
			}
	    
			aUV[0].fX = pAtlasInfo->mTopLeft.fX / (float)pAtlasInfo->mpTexture->miGLWidth; 
			aUV[0].fY = pAtlasInfo->mTopLeft.fY / (float)pAtlasInfo->mpTexture->miGLHeight;
			aUV[1].fX = pAtlasInfo->mBottomRight.fX / (float)pAtlasInfo->mpTexture->miGLWidth;
			aUV[1].fY = pAtlasInfo->mTopLeft.fY / (float)pAtlasInfo->mpTexture->miGLHeight;
			aUV[2].fX = pAtlasInfo->mTopLeft.fX / (float)pAtlasInfo->mpTexture->miGLWidth; 
			aUV[2].fY = pAtlasInfo->mBottomRight.fY / (float)pAtlasInfo->mpTexture->miGLHeight;
			aUV[3].fX = pAtlasInfo->mBottomRight.fX / (float)pAtlasInfo->mpTexture->miGLWidth;
			aUV[3].fY = pAtlasInfo->mBottomRight.fY / (float)pAtlasInfo->mpTexture->miGLHeight;
		}
		
#if defined( UI_USE_BATCH )
        renderQuad( pAtlasInfo->mpTexture->mszName,
                    aScreenVerts,
                    maCornerColors,
                    aUV,
                    miShaderProgram );
#else
		glBindTexture( GL_TEXTURE_2D, pTexture->miID );

        assert( miShaderProgram > 0 );
        
        glVertexAttribPointer( miShaderPos, 4, GL_FLOAT, GL_FALSE, 0, aScreenVerts );
        //glEnableVertexAttribArray( miShaderPos );
        
        glVertexAttribPointer( miShaderUV, 2, GL_FLOAT, GL_FALSE, 0, aUV );
        //glEnableVertexAttribArray( miShaderUV );
        
        glVertexAttribPointer( miShaderColor, 4, GL_FLOAT, GL_FALSE, 0, maCornerColors );
        //glEnableVertexAttribArray( miShaderColor );
        
        glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

#endif // #if UI_USE_BATCH
        
    }   // if alpha > 0.0
}

/*
**
*/
void CBox::setColor( float fRed, float fGreen, float fBlue, float fAlpha )
{
    mOrigColor.fX = fRed;
    mOrigColor.fY = fGreen;
    mOrigColor.fZ = fBlue;
    mOrigColor.fW = fAlpha;
    
    memcpy( &mColor, &mOrigColor, sizeof( mColor ) );
    mColor.fW = 1.0f;

	for( int i = 0; i < NUM_BOX_CORNERS; i++ )
	{
		memcpy( &maCornerColors[i], &mOrigColor, sizeof( tVector4 ) );
	}
}

/*
**
*/
void CBox::setCornerColor( int iCorner, tVector4 const* pColor )
{
	memcpy( &maCornerColors[iCorner], pColor, sizeof( tVector4 ) );
}

/*
**
*/
CControl* CBox::copy( void )
{
    CBox* pBox = new CBox();
    copyControl( pBox );
    
    memcpy( &pBox->mOrigColor, &mOrigColor, sizeof( mOrigColor ) );
    memcpy( &pBox->maCornerColors, &maCornerColors, sizeof( maCornerColors ) );
    
    return pBox;
}