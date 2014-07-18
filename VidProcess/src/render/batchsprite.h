//
//  batchsprite.h
//  CityBenchmark
//
//  Created by Dingwings on 7/30/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//
#ifndef __BATCHSPRITE_H__
#define __BATCHSPRITE_H__

#include "vector.h"

//#define USE_VBO 1

struct SpriteInterleaveVert
{
    tVector4        mV;
    tVector2        mTexCoord;
    tVector4        mColor;
};

typedef struct SpriteInterleaveVert tSpriteInterleaveVert;

struct SpriteVerts 
{
    tSpriteInterleaveVert maVerts[6];
};

typedef struct SpriteVerts tSpriteVerts;

struct BatchBucket
{
    unsigned int        maiTexID[4];
    
    //unsigned int        miTexID;
    unsigned int        miShader;
    tSpriteVerts*       maSpriteVerts;
    int                 miNumSprites;
    int                 miNumSpritesAlloc;
    int                 miNumVerts;
    bool                mbBlending;
    
#if defined( USE_VBO )
    GLuint              miVBO;
#endif // USE_VBO
    
    GLuint              miVertexArray;
    
    int                 miSrcBlend;
    int                 miDestBlend;
    
    bool                mbScissor;
    tVector4            mScissorRect;
    
    // shader attributes
    int                 miShaderPosition;
    int                 miShaderTexCoord;
    int                 miShaderColor;
    int                 miShaderViewProj;
};

typedef struct BatchBucket tBatchBucket;

class CBatchManager
{
public:
    CBatchManager( void );
    ~CBatchManager( void );
    
    void draw( void );
    void reset( void );
    
    void addSprite( GLuint const* aiTexID,
                    tVector4 const* aPos,
                    tVector4 const* aColors,
                    tVector2 const* aUV,
                    unsigned int iShader,
                    int iSrcBlend = GL_SRC_ALPHA,
                    int iDestBlend = GL_ONE_MINUS_SRC_ALPHA,
                    bool bBlending = true );
    
    void purgeAll( void );
    
    void setScissorState( bool bScissor, int iX, int iY, int iWidth, int iHeight );
    
public:
    static CBatchManager* instance( void );
    
protected:
    static CBatchManager*  mpInstance;
    
protected:
    int             miNumBucketsAlloc;
    int             miNumBuckets;
    tBatchBucket*   maBuckets;
    
    bool            mbInScissor;
    tVector4        mScissorRect;

protected:
    void addToBucket( tBatchBucket* pBucket, 
					  tVector4 const* pPos, 
					  float fWidth, 
					  float fHeight, 
					  tVector4 const* pColor );

    void addToBucket( tBatchBucket* pBucket, 
                      tVector4 const* aPos, 
                      tVector4 const* aColors, 
                      tVector2 const* aUV,
                      bool bBlending );

};

#endif // __BATCHSPRITE_H__