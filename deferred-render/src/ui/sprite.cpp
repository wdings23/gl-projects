#include "sprite.h"
#include "render.h"

/*
**
*/
CSprite::CSprite( void )
{
	memset( mszTextureName, 0, sizeof( mszTextureName ) );
	mpTexture = NULL;
    
    miType = CControl::TYPE_SPRITE;
   
    mfClipU = 1.0f;
    mfClipV = 1.0f;
    mbClip = true;
    
    mpAtlasInfo = NULL;
    
    memset( &mTopLeftUV, 0, sizeof( mTopLeftUV ) );
    memset( &mBottomRightUV, 0, sizeof( mBottomRightUV ) );

}

/*
**
*/
CSprite::~CSprite( void )
{
	memset( mszTextureName, 0, sizeof( mszTextureName ) );
}

/*
**
*/
void CSprite::draw( double fTime, int iScreenWidth, int iScreenHeight )
{
    if( miState == STATE_DISABLED )
	{
		return;
	}
    
    updateMatrix( fTime );
    
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

		tVector4 aColor[] =
        {
            { mColor.fX, mColor.fY, mColor.fZ, mColor.fW },
            { mColor.fX, mColor.fY, mColor.fZ, mColor.fW },
            { mColor.fX, mColor.fY, mColor.fZ, mColor.fW },
            { mColor.fX, mColor.fY, mColor.fZ, mColor.fW }
        };

        CTextureManager* pTextureManager = CTextureManager::instance();
        assert( mpTexture );
        tTexture* pTexture = pTextureManager->getTexture( mpTexture->mszName );
        if( pTexture == NULL )
        {
            pTextureManager->registerTexture( mpTexture->mszName );
            pTexture = pTextureManager->getTexture( mpTexture->mszName );
        }

        glBindTexture( GL_TEXTURE_2D, pTexture->miID );
        
        float fU = mDimension.fX / (float)pTexture->miWidth;
        float fV = mDimension.fY / (float)pTexture->miHeight;
        
        if( !mbClip )
        {
            fU = mfClipU;
            fV = mfClipV;
        }
        
        maUV[1].fX = fU;
        maUV[2].fY = fV;
        maUV[3].fX = fU;
        maUV[3].fY = fV;

		if( !mbClip )
		{
			maUV[0].fX = mTopLeftUV.fX;
            maUV[0].fY = mTopLeftUV.fY;
            
            maUV[1].fX = mBottomRightUV.fX;
            maUV[1].fY = mTopLeftUV.fY;
            
            maUV[2].fX = mTopLeftUV.fX;
            maUV[2].fY = mBottomRightUV.fY;
            
            maUV[3].fX = mBottomRightUV.fX;
            maUV[3].fY = mBottomRightUV.fY;
		}
        
        if( mpAtlasInfo && mpAtlasInfo->mpTexture )
        {
            if( mbClip )
            {
                float fU0 = mpAtlasInfo->mTopLeft.fX / (float)mpAtlasInfo->mpTexture->miWidth;
                float fU1 = mpAtlasInfo->mBottomRight.fX / (float)mpAtlasInfo->mpTexture->miWidth;
                
                float fV0 = mpAtlasInfo->mTopLeft.fY / (float)mpAtlasInfo->mpTexture->miHeight;
                float fV1 = mpAtlasInfo->mBottomRight.fY / (float)mpAtlasInfo->mpTexture->miHeight; 
                
                maUV[0].fX = fU0;
                maUV[0].fY = fV0;
                
                maUV[1].fX = fU1;
                maUV[1].fY = fV0;
                
                maUV[2].fX = fU0;
                maUV[2].fY = fV1;
                
                maUV[3].fX = fU1;
                maUV[3].fY = fV1;
            }
            else
            {
                maUV[0].fX = mTopLeftUV.fX;
                maUV[0].fY = mTopLeftUV.fY;
                
                maUV[1].fX = mBottomRightUV.fX;
                maUV[1].fY = mTopLeftUV.fY;
                
                maUV[2].fX = mTopLeftUV.fX;
                maUV[2].fY = mBottomRightUV.fY;
                
                maUV[3].fX = mBottomRightUV.fX;
                maUV[3].fY = mBottomRightUV.fY;
            }
        }
        
        assert( miShaderProgram > 0 );
        
#if defined( UI_USE_BATCH )
        renderQuad( mpTexture->mszName, aScreenVerts, aColor, maUV, miShaderProgram );
#else
		glBindTexture( GL_TEXTURE_2D, pTexture->miID );

        assert( miShaderProgram > 0 );
        
        glVertexAttribPointer( miShaderPos, 4, GL_FLOAT, GL_FALSE, 0, aScreenVerts );
        glVertexAttribPointer( miShaderUV, 2, GL_FLOAT, GL_FALSE, 0, maUV );
        glVertexAttribPointer( miShaderColor, 4, GL_FLOAT, GL_FALSE, 0, aColor );
        
        glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
        
#endif // #if UI_USE_BATCH

    }   // if alpha > 0.0

	for( unsigned int i = 0; i < mapChildren.size(); i++ )
	{
		mapChildren[i]->draw( fTime, iScreenWidth, iScreenHeight );
	}
    
    // draw emitter
    if( mpAnimPlayer )
    {
        CEmitter* pEmitter = mpAnimPlayer->getEmitter();
        if( pEmitter )
        {
            pEmitter->render();
        }
    }
}

/*
**
*/
CControl* CSprite::copy( void )
{
    CSprite* pSprite = new CSprite();
    copyControl( pSprite );
    
    memcpy( pSprite->mszTextureName, mszTextureName, sizeof( pSprite->mszTextureName ) );
    pSprite->mpTexture = mpTexture;
    pSprite->miTexOrigWidth = miTexOrigWidth;
    pSprite->miTexOrigHeight = miTexOrigHeight;
    pSprite->mpAtlasInfo = mpAtlasInfo;
    
    
    return pSprite;
    
}