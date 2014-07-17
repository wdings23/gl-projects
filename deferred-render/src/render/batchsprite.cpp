//
//  batchsprite.cpp
//  CityBenchmark
//
//  Created by Dingwings on 7/30/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "batchsprite.h"
#include "camera.h"
#include "ShaderManager.h"

#define NUM_BUCKETS_PER_ALLOC 10
#define NUM_SPRITES_PER_ALLOC 100

CBatchManager* CBatchManager::mpInstance = NULL;
/*
**
*/
CBatchManager* CBatchManager::instance( void )
{
    if( mpInstance == NULL )
    {
        mpInstance = new CBatchManager();
    }
    
    return mpInstance;
}

/*
**
*/
CBatchManager::CBatchManager( void )
{
    miNumBucketsAlloc = NUM_BUCKETS_PER_ALLOC;
    miNumBuckets = 0;
    maBuckets = (tBatchBucket *)MALLOC( sizeof( tBatchBucket ) * miNumBucketsAlloc );
    memset( maBuckets, 0, sizeof( tBatchBucket ) * miNumBucketsAlloc );
    
    for( int i = 0; i < miNumBucketsAlloc; i++ )
    {
        tBatchBucket* pBucket = &maBuckets[i];
        pBucket->miNumSpritesAlloc = NUM_SPRITES_PER_ALLOC;
        pBucket->miNumSprites = 0;
        pBucket->maSpriteVerts = (tSpriteVerts *)MALLOC( sizeof( tSpriteVerts ) * NUM_SPRITES_PER_ALLOC );
        pBucket->miNumVerts = 0;
        //pBucket->miTexID = -1;
        for( int i = 0; i < sizeof( pBucket->maiTexID ) / sizeof( *pBucket->maiTexID ); i++ )
        {
            pBucket->maiTexID[i] = -1;
        }
        
#if defined( USE_VBO )
        glGenBuffers( 1, &pBucket->miVBO );
        glBindBuffer( GL_ARRAY_BUFFER, pBucket->miVBO );
        glBufferData( GL_ARRAY_BUFFER, pBucket->miNumSpritesAlloc * sizeof( tSpriteVerts ), pBucket->maSpriteVerts, GL_STREAM_DRAW );

        glBindBuffer( GL_ARRAY_BUFFER, 0 );
#endif // USE_VBO
        
        mbInScissor = false;
        memset( &mScissorRect, 0, sizeof( tVector4 ) );
        
    }   // for i = 0 to num buckets
}

/*
**
*/
CBatchManager::~CBatchManager( void )
{
    purgeAll();
}

/*
**
*/
void CBatchManager::draw( void )
{
    //for( int i = 0; i < miNumBuckets; i++ )
    for( int i = 0; i < miNumBuckets; i++ )
    {
        tBatchBucket* pBucket = &maBuckets[i];
        
        if( !pBucket->mbBlending )
        {
            glDisable( GL_BLEND );
        }
        
        // shader
        glUseProgram( pBucket->miShader );
        
        for( int i = 0; i < sizeof( pBucket->maiTexID ) / sizeof( *pBucket->maiTexID ); i++ )
        {
            if( pBucket->maiTexID[i] < 999999 &&
                pBucket->maiTexID[i] > 0 )
            {
                int iSamplerLocation = glGetUniformLocation( pBucket->miShader, "texture" );
                glActiveTexture( GL_TEXTURE0 );
                glBindTexture( GL_TEXTURE_2D, pBucket->maiTexID[i] );
                glUniform1i( iSamplerLocation, i );
            }
        }
        
#if defined( USE_VBO )
        WTFASSERT2( pBucket->miVBO > 0, "invalid bucket VBO" );
        glBindBuffer( GL_ARRAY_BUFFER, pBucket->miVBO );
        
        glBufferData( GL_ARRAY_BUFFER, pBucket->miNumSprites * sizeof( tSpriteVerts ), pBucket->maSpriteVerts, GL_DYNAMIC_DRAW );
        glVertexAttribPointer( pBucket->miShaderPosition, 4, GL_FLOAT, GL_FALSE, sizeof( tSpriteInterleaveVert ), 0 );
        glVertexAttribPointer( pBucket->miShaderTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof( tSpriteInterleaveVert ), (GLvoid *)( sizeof( tVector4 ) ) );
        glVertexAttribPointer( pBucket->miShaderColor, 4, GL_FLOAT, GL_FALSE, sizeof( tSpriteInterleaveVert ), (GLvoid *)( sizeof( tVector4 ) + sizeof( tVector2 ) ) );
#else
        glVertexAttribPointer( pBucket->miShaderPosition, 4, GL_FLOAT, GL_FALSE, sizeof( tSpriteInterleaveVert ), &pBucket->maSpriteVerts[0].maVerts[0].mV );
        glVertexAttribPointer( pBucket->miShaderTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof( tSpriteInterleaveVert ), &pBucket->maSpriteVerts[0].maVerts[0].mTexCoord );
        glVertexAttribPointer( pBucket->miShaderColor, 4, GL_FLOAT, GL_FALSE, sizeof( tSpriteInterleaveVert ), &pBucket->maSpriteVerts[0].maVerts[0].mColor );
#endif // USE_VBO
        
        // view projection matrix
        if( pBucket->miShaderViewProj >= 0 )
        {
            CCamera* pCamera = CCamera::instance();
            const tMatrix44* pViewProjMatrix = pCamera->getViewProjMatrix();
            glUniformMatrix4fv( pBucket->miShaderViewProj, 1, GL_FALSE, pViewProjMatrix->afEntries );
        }
        
        // scissor batch
        if( pBucket->mbScissor )
        {
            glEnable( GL_SCISSOR_TEST );
            glScissor( (int)pBucket->mScissorRect.fX,
                       (int)pBucket->mScissorRect.fY,
                       (int)pBucket->mScissorRect.fZ,
                       (int)pBucket->mScissorRect.fW );
        }
        
        glBlendFunc( pBucket->miSrcBlend, pBucket->miDestBlend );
        glDrawArrays( GL_TRIANGLES, 0, 6 * pBucket->miNumSprites );
        
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        
        if( !pBucket->mbBlending )
        {
            glEnable( GL_BLEND );
        }
        
        if( pBucket->mbScissor )
        {
            glDisable( GL_SCISSOR_TEST );
        }
        
#if defined( USE_VBO )
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
#endif // USE_VBO
    }
}

/*
**
*/
void CBatchManager::addToBucket( tBatchBucket* pBucket, 
                                 tVector4 const* aPos, 
                                 tVector4 const* aColors, 
                                 tVector2 const* aUV,
                                 bool bBlending )
{
    // make room for more
    if( pBucket->miNumSprites + 1 > pBucket->miNumSpritesAlloc )
    {
        pBucket->miNumSpritesAlloc += NUM_SPRITES_PER_ALLOC;
        pBucket->maSpriteVerts = (tSpriteVerts *)REALLOC( pBucket->maSpriteVerts, sizeof( tSpriteVerts ) * pBucket->miNumSpritesAlloc );
        
        memset( pBucket->maSpriteVerts, 0, sizeof( tSpriteVerts ) * pBucket->miNumSpritesAlloc );
    
#if defined( USE_VBO )
        glDeleteBuffers( 1, &pBucket->miVBO );
        glGenBuffers( 1, &pBucket->miVBO );
        glBindBuffer( GL_ARRAY_BUFFER, pBucket->miVBO );
        glBufferData( GL_ARRAY_BUFFER, pBucket->miNumSpritesAlloc * sizeof( tSpriteVerts ), pBucket->maSpriteVerts, GL_STREAM_DRAW );
        
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        
#endif // VBO
    }
    
    if( pBucket->miVBO <= 0 )
    {
        glGenBuffers( 1, &pBucket->miVBO );
        glBindBuffer( GL_ARRAY_BUFFER, pBucket->miVBO );
        glBufferData( GL_ARRAY_BUFFER,
                      pBucket->miNumSpritesAlloc * sizeof( tSpriteVerts ),
                      pBucket->maSpriteVerts,
                      GL_DYNAMIC_DRAW );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
    }
    
    pBucket->mbBlending = bBlending;
    
    // scissor
    pBucket->mbScissor = mbInScissor;
    memcpy( &pBucket->mScissorRect, &mScissorRect, sizeof( tVector4 ) );
    
    assert( pBucket->miNumSprites < pBucket->miNumSpritesAlloc );
    tSpriteVerts* pInsert = &pBucket->maSpriteVerts[pBucket->miNumSprites];
    
    // tri 0
    // top-left 
    memcpy( &pInsert->maVerts[0].mV, &aPos[0], sizeof( tVector4 ) );
    memcpy( &pInsert->maVerts[0].mTexCoord, &aUV[0], sizeof( tVector2 ) );
    memcpy( &pInsert->maVerts[0].mColor, &aColors[0], sizeof( tVector4 ) );
    
    // bottom-left 
    memcpy( &pInsert->maVerts[1].mV, &aPos[1], sizeof( tVector4 ) );
    memcpy( &pInsert->maVerts[1].mTexCoord, &aUV[1], sizeof( tVector2 ) );
    memcpy( &pInsert->maVerts[1].mColor, &aColors[1], sizeof( tVector4 ) );
    
    // bottom-right 
    memcpy( &pInsert->maVerts[2].mV, &aPos[3], sizeof( tVector4 ) );
    memcpy( &pInsert->maVerts[2].mTexCoord, &aUV[3], sizeof( tVector2 ) );
    memcpy( &pInsert->maVerts[2].mColor, &aColors[3], sizeof( tVector4 ) );
    
    // tri 1
    // top-left 
    memcpy( &pInsert->maVerts[3].mV, &aPos[0], sizeof( tVector4 ) );
    memcpy( &pInsert->maVerts[3].mTexCoord, &aUV[0], sizeof( tVector2 ) );
    memcpy( &pInsert->maVerts[3].mColor, &aColors[0], sizeof( tVector4 ) );
    
    // bottom-right
    memcpy( &pInsert->maVerts[4].mV, &aPos[3], sizeof( tVector4 ) );
    memcpy( &pInsert->maVerts[4].mTexCoord, &aUV[3], sizeof( tVector2 ) );
    memcpy( &pInsert->maVerts[4].mColor, &aColors[3], sizeof( tVector4 ) );
    
    // top-right
    memcpy( &pInsert->maVerts[5].mV, &aPos[2], sizeof( tVector4 ) );
    memcpy( &pInsert->maVerts[5].mTexCoord, &aUV[2], sizeof( tVector2 ) );
    memcpy( &pInsert->maVerts[5].mColor, &aColors[2], sizeof( tVector4 ) );
    
    pBucket->miNumVerts += 6;
    ++pBucket->miNumSprites;
}

/*
**
*/
void CBatchManager::addSprite( GLuint const* aiTexID,
                               tVector4 const* aPos,
                               tVector4 const* aColors,
                               tVector2 const* aUV,
                               unsigned int iShader,
                               int iSrcBlend,
                               int iDestBlend,
                               bool bBlending )
{
    bool bFound = false;
	int iLastBucket = miNumBuckets - 1;
	if( iLastBucket >= 0 )
	{
		tBatchBucket* pLastBucket = &maBuckets[iLastBucket];
		if( pLastBucket->maiTexID[0] == aiTexID[0] && 
			pLastBucket->maiTexID[1] == aiTexID[1] &&
			pLastBucket->maiTexID[2] == aiTexID[2] &&
			pLastBucket->maiTexID[3] == aiTexID[3] && 
			pLastBucket->miShader == iShader &&
			pLastBucket->miSrcBlend == iSrcBlend &&
			pLastBucket->miDestBlend == iDestBlend &&
			pLastBucket->mbScissor == mbInScissor &&
			fabs( pLastBucket->mScissorRect.fX - mScissorRect.fX ) < 0.001f &&
			fabs( pLastBucket->mScissorRect.fY - mScissorRect.fY ) < 0.001f &&
			fabs( pLastBucket->mScissorRect.fZ - mScissorRect.fZ ) < 0.001f &&
			fabs( pLastBucket->mScissorRect.fW - mScissorRect.fW ) < 0.001f &&
			( pLastBucket->miNumSprites >> 1 ) < ( ( 1 << 16 ) - 4 ) )
		{
			addToBucket( pLastBucket, aPos, aColors, aUV, bBlending );
			bFound = true;			   
		}
	}

    if( !bFound )
    {
        // make room for more
        if( miNumBuckets >= miNumBucketsAlloc )
        {
            miNumBucketsAlloc += NUM_BUCKETS_PER_ALLOC;
            
            if( maBuckets == NULL )
            {
                // just purged all the buckets
                maBuckets = (tBatchBucket *)MALLOC( sizeof( tBatchBucket ) * miNumBucketsAlloc );
            }
            else
            {
                maBuckets = (tBatchBucket *)REALLOC( maBuckets, sizeof( tBatchBucket ) * miNumBucketsAlloc );
            }
            
            for( int i = miNumBuckets; i < miNumBucketsAlloc; i++ )
            {
                tBatchBucket* pBucket = &maBuckets[i];
                pBucket->miNumSpritesAlloc = NUM_SPRITES_PER_ALLOC;
                pBucket->miNumSprites = 0;
                pBucket->maSpriteVerts = (tSpriteVerts *)MALLOC( sizeof( tSpriteVerts ) * NUM_SPRITES_PER_ALLOC );
                pBucket->miNumVerts = 0;
                pBucket->miSrcBlend = GL_SRC_ALPHA;
                pBucket->miDestBlend = GL_ONE_MINUS_SRC_ALPHA;
                pBucket->mbBlending = true;
                pBucket->mbScissor = false;
                memcpy( &pBucket->mScissorRect, &mScissorRect, sizeof( tVector4 ) );
                
                for( int i = 0; i < sizeof( pBucket->maiTexID ) / sizeof( *pBucket->maiTexID ); i++ )
                {
                    pBucket->maiTexID[i] = -1;
                }
                
            }   // for i = 0 to num buckets
        }
        
        // set bucket info
        assert( miNumBuckets < miNumBucketsAlloc );
        tBatchBucket* pBucket = &maBuckets[miNumBuckets++];
        pBucket->miShader = iShader;
        pBucket->miSrcBlend = iSrcBlend;
        pBucket->miDestBlend = iDestBlend;
        pBucket->mbBlending = bBlending;
        pBucket->mbScissor = mbInScissor;
        memcpy( &pBucket->mScissorRect, &mScissorRect, sizeof( tVector4 ) );
        
        memcpy( pBucket->maiTexID, aiTexID, sizeof( pBucket->maiTexID ) );
        
        // shader attributes
        pBucket->miShaderPosition = glGetAttribLocation( pBucket->miShader, "position" );
        pBucket->miShaderTexCoord = glGetAttribLocation( pBucket->miShader, "textureUV" );
        pBucket->miShaderColor = glGetAttribLocation( pBucket->miShader, "color" );
        pBucket->miShaderViewProj = glGetUniformLocation( pBucket->miShader, "viewProjMat" );
        
        addToBucket( pBucket, aPos, aColors, aUV, bBlending );
    }
}

/*
**
*/
void CBatchManager::addToBucket( tBatchBucket* pBucket,
                                 tVector4 const* pPos, 
                                 float fWidth, 
                                 float fHeight,
                                 tVector4 const* pColor )
{
    // make room for more
    if( pBucket->miNumSprites + 1 > pBucket->miNumSpritesAlloc )
    {
        pBucket->miNumSpritesAlloc += NUM_SPRITES_PER_ALLOC;
        pBucket->maSpriteVerts = (tSpriteVerts *)REALLOC( pBucket->maSpriteVerts, sizeof( tSpriteVerts ) * pBucket->miNumSpritesAlloc );
        
        memset( pBucket->maSpriteVerts, 0, sizeof( tSpriteVerts ) * pBucket->miNumSpritesAlloc );
    }
        
    // not inserted, put in the back
    {
        float fHalfWidth = fWidth * 0.5f;
        float fHalfHeight = fHeight * 0.5f;
        
        assert( pBucket->miNumSprites < pBucket->miNumSpritesAlloc );
        tSpriteVerts* pInsert = &pBucket->maSpriteVerts[pBucket->miNumSprites];
        pInsert->maVerts[0].mV.fX = pPos->fX - fHalfWidth;
        pInsert->maVerts[0].mV.fY = pPos->fY + fHalfHeight;
        pInsert->maVerts[0].mV.fZ = pPos->fZ;
        pInsert->maVerts[0].mV.fW = 1.0f;
        pInsert->maVerts[0].mTexCoord.fX = 0.0f;
        pInsert->maVerts[0].mTexCoord.fY = 0.0f;
        pInsert->maVerts[0].mColor.fX = pColor->fX;
        pInsert->maVerts[0].mColor.fY = pColor->fY;
        pInsert->maVerts[0].mColor.fZ = pColor->fZ;
        pInsert->maVerts[0].mColor.fW = pColor->fW;
        
        pInsert->maVerts[1].mV.fX = pPos->fX - fHalfWidth;
        pInsert->maVerts[1].mV.fY = pPos->fY - fHalfHeight;
        pInsert->maVerts[1].mV.fZ = pPos->fZ;
        pInsert->maVerts[1].mV.fW = 1.0f;
        pInsert->maVerts[1].mTexCoord.fX = 0.0f;
        pInsert->maVerts[1].mTexCoord.fY = 1.0f;
        pInsert->maVerts[1].mColor.fX = pColor->fX;
        pInsert->maVerts[1].mColor.fY = pColor->fY;
        pInsert->maVerts[1].mColor.fZ = pColor->fZ;
        pInsert->maVerts[1].mColor.fW = pColor->fW;
        
        pInsert->maVerts[2].mV.fX = pPos->fX + fHalfWidth;
        pInsert->maVerts[2].mV.fY = pPos->fY - fHalfHeight;
        pInsert->maVerts[2].mV.fZ = pPos->fZ;
        pInsert->maVerts[2].mV.fW = 1.0f;
        pInsert->maVerts[2].mTexCoord.fX = 1.0f;
        pInsert->maVerts[2].mTexCoord.fY = 1.0f;
        pInsert->maVerts[2].mColor.fX = pColor->fX;
        pInsert->maVerts[2].mColor.fY = pColor->fY;
        pInsert->maVerts[2].mColor.fZ = pColor->fZ;
        pInsert->maVerts[2].mColor.fW = pColor->fW;
        
        pInsert->maVerts[3].mV.fX = pPos->fX + fHalfWidth;
        pInsert->maVerts[3].mV.fY = pPos->fY - fHalfHeight;
        pInsert->maVerts[3].mV.fZ = pPos->fZ;
        pInsert->maVerts[3].mV.fW = 1.0f;
        pInsert->maVerts[3].mTexCoord.fX = 1.0f;
        pInsert->maVerts[3].mTexCoord.fY = 1.0f;
        pInsert->maVerts[3].mColor.fX = pColor->fX;
        pInsert->maVerts[3].mColor.fY = pColor->fY;
        pInsert->maVerts[3].mColor.fZ = pColor->fZ;
        pInsert->maVerts[3].mColor.fW = pColor->fW;
        
        pInsert->maVerts[4].mV.fX = pPos->fX + fHalfWidth;
        pInsert->maVerts[4].mV.fY = pPos->fY + fHalfHeight;
        pInsert->maVerts[4].mV.fZ = pPos->fZ;
        pInsert->maVerts[4].mV.fW = 1.0f;
        pInsert->maVerts[4].mTexCoord.fX = 1.0f;
        pInsert->maVerts[4].mTexCoord.fY = 0.0f;
        pInsert->maVerts[4].mColor.fX = pColor->fX;
        pInsert->maVerts[4].mColor.fY = pColor->fY;
        pInsert->maVerts[4].mColor.fZ = pColor->fZ;
        pInsert->maVerts[4].mColor.fW = pColor->fW;
        
        pInsert->maVerts[5].mV.fX = pPos->fX - fHalfWidth;
        pInsert->maVerts[5].mV.fY = pPos->fY + fHalfHeight;
        pInsert->maVerts[5].mV.fZ = pPos->fZ;
        pInsert->maVerts[5].mV.fW = 1.0f;
        pInsert->maVerts[5].mTexCoord.fX = 0.0f;
        pInsert->maVerts[5].mTexCoord.fY = 0.0f;
        pInsert->maVerts[5].mColor.fX = pColor->fX;
        pInsert->maVerts[5].mColor.fY = pColor->fY;
        pInsert->maVerts[5].mColor.fZ = pColor->fZ;
        pInsert->maVerts[5].mColor.fW = pColor->fW;
  
    
    }   // if not inserted
    
    pBucket->miNumVerts += 6;
    ++pBucket->miNumSprites;
}

/*
**
*/
void CBatchManager::reset( void )
{
    for( int i = 0; i < miNumBucketsAlloc; i++ )
    {
        tBatchBucket* pBucket = &maBuckets[i];
        pBucket->miNumSprites = 0;
        pBucket->miNumVerts = 0;
        pBucket->mbScissor = false;
        memset( &pBucket->mScissorRect, 0, sizeof( tVector4 ) );
    }
    
    miNumBuckets = 0;
}

/*
**
*/
void CBatchManager::purgeAll( void )
{
    for( int i = 0; i < miNumBucketsAlloc; i++ )
    {
        tBatchBucket* pBucket = &maBuckets[i];
        FREE( pBucket->maSpriteVerts );
    }
    
    FREE( maBuckets );
    maBuckets = NULL;
    
    miNumBucketsAlloc = 0;
    miNumBuckets = 0;
}

/*
**
*/
void CBatchManager::setScissorState( bool bScissor, int iX, int iY, int iWidth, int iHeight )
{
    mbInScissor = bScissor;
    
    if( bScissor )
    {
        mScissorRect.fX = (float)iX;
        mScissorRect.fY = (float)iY;
        mScissorRect.fZ = (float)iWidth;
        mScissorRect.fW = (float)iHeight;
    }
    else
    {
        memset( &mScissorRect, 0, sizeof( tVector4 ) );
    }
}