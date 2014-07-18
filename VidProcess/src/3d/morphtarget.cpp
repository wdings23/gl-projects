//
//  morphtarget.cpp
//  Game7
//
//  Created by Dingwings on 5/1/14.
//  Copyright (c) 2014 Dingwings. All rights reserved.
//

#include "morphtarget.h"
#include "tinyxml.h"
#include "filepathutil.h"
#include "parseutil.h"
#include "jobmanager.h"

struct MorphJobData
{
    int                               miNumVerts;
    
    tVector4*                         maResultVertPos;
    tVector4*                         maResultVertNorm;
    
    tVector4 const*                   maPrevFrameVertPos;
    tVector4 const*                   maPrevFrameVertNorm;
    
    tVector4 const*                   maNextFrameVertPos;
    tVector4 const*                   maNextFrameVertNorm;
    
    float                       mfPct;
};

typedef struct MorphJobData tMorphJobData;

static void loadFrames( tMorphTarget* pMorphTarget, TiXmlNode* pNode );
static void computeMorph( void* pData, void* pJobDebugData );

/*
**
*/
void morphTargetInit( tMorphTarget* pMorphTarget )
{
    memset( pMorphTarget, 0, sizeof( tMorphTarget ) );
    
    Matrix44Identity( &pMorphTarget->mXFormMatrix );
    Matrix44Identity( &pMorphTarget->mRotationMatrix );
}

/*
**
*/
void morphTargetRelease( tMorphTarget* pMorphTarget )
{
    for( int i = 0; i < pMorphTarget->miNumFrames; i++ )
    {
        FREE( pMorphTarget->maaAnimVertPos[i] );
		FREE( pMorphTarget->maaAnimVertNorm[i] );
    }
    
	FREE( pMorphTarget->maaAnimVertPos );
	FREE( pMorphTarget->maaAnimVertNorm );

    modelRelease( pMorphTarget->mpModel );
    FREE( pMorphTarget->mpModel );
}

/*
**
*/
void morphTargetLoad( tMorphTarget* pMorphTarget, const char* szFileName )
{
    char szFullPath[256];
    getFullPath( szFullPath, szFileName );
    
    TiXmlDocument doc( szFullPath );
    bool bLoaded = doc.LoadFile();
    if( bLoaded )
    {
        morphTargetInit( pMorphTarget );
        
        TiXmlNode* pNode = doc.FirstChild()->FirstChild( "mesh_file" );
        
        // count number of frames
        int iNumFrames = 0;
        TiXmlNode* pFrameNode = doc.FirstChild()->FirstChild( "frame" );
        while( pFrameNode )
        {
            ++iNumFrames;
            pFrameNode = pFrameNode->NextSibling( "frame" );
        }
        
        pMorphTarget->miNumFrames = iNumFrames;
        
        // load model
        const char* szMeshFile = pNode->FirstChild()->Value();
        
        pMorphTarget->mpModel = (tModel *)MALLOC( sizeof( tModel ) );
        modelInit( pMorphTarget->mpModel );
        modelLoad( pMorphTarget->mpModel, szMeshFile );
        
        // initialize animating vertex positions
        pMorphTarget->maaAnimVertPos = (tVector4 **)MALLOC( sizeof( tVector4* ) * pMorphTarget->miNumFrames );
		pMorphTarget->maaAnimVertNorm = (tVector4 **)MALLOC( sizeof( tVector4* ) * pMorphTarget->miNumFrames );
        for( int i = 0; i < pMorphTarget->miNumFrames; i++ )
        {
            pMorphTarget->maaAnimVertPos[i] = (tVector4 *)MALLOC( sizeof( tVector4 ) * pMorphTarget->mpModel->miNumVerts );
			pMorphTarget->maaAnimVertNorm[i] = (tVector4 *)MALLOC( sizeof( tVector4 ) * pMorphTarget->mpModel->miNumNormals );
        }
        
        pFrameNode = doc.FirstChild()->FirstChild( "frame" );
        loadFrames( pMorphTarget, pFrameNode );
    }
    else
    {
        WTFASSERT2( 0, "can't load %s : %s", szFileName, doc.ErrorDesc() );
    }
}

/*
**
*/
void morphTargetAnimFrameUpdate( tMorphTargetAnimation* pAnimation, float fDT )
{
    pAnimation->mfTime += fDT;
    if( pAnimation->mfTime >= pAnimation->mfDuration )
    {
        pAnimation->mfTime = 0.0f;
    }

// test test test
float fTimePerKey = pAnimation->mfDuration / (float)( pAnimation->mpMorphTarget->miNumFrames - 1 );
    

    int iNextFrameIndex = -1;
    tVector4 const* aNextFrameVertPos = NULL;
    tVector4 const* aPrevFrameVertPos = NULL;

	tVector4 const* aNextFrameVertNorm = NULL;
    tVector4 const* aPrevFrameVertNorm = NULL;
	
	 // get the next frame
    for( int i = 0; i < pAnimation->mpMorphTarget->miNumFrames; i++ )
    {
        if( (float)i * fTimePerKey > pAnimation->mfTime )
        {
            aNextFrameVertPos = pAnimation->mpMorphTarget->maaAnimVertPos[i];
			aNextFrameVertNorm = pAnimation->mpMorphTarget->maaAnimVertNorm[i];

            WTFASSERT2( i - 1 >= 0, "invalid key index" );
            
			aPrevFrameVertPos = pAnimation->mpMorphTarget->maaAnimVertPos[i-1];
			aPrevFrameVertNorm = pAnimation->mpMorphTarget->maaAnimVertNorm[i-1];

            iNextFrameIndex = i;
			pAnimation->miAnimFrame = iNextFrameIndex - 1;

            break;
        }
    }

    // use last frame if not found
    if( aNextFrameVertPos == NULL )
    {
        iNextFrameIndex = pAnimation->mpMorphTarget->miNumFrames - 1;
		pAnimation->miAnimFrame = iNextFrameIndex;
        
		aNextFrameVertPos = pAnimation->mpMorphTarget->maaAnimVertPos[iNextFrameIndex];
        aNextFrameVertNorm = pAnimation->mpMorphTarget->maaAnimVertNorm[iNextFrameIndex];

        WTFASSERT2( iNextFrameIndex - 1 >= 0, "invalid key index" );
        
		aPrevFrameVertPos = pAnimation->mpMorphTarget->maaAnimVertPos[iNextFrameIndex-1];
		aPrevFrameVertNorm = pAnimation->mpMorphTarget->maaAnimVertNorm[iNextFrameIndex-1];
    }
    
    // percentage between the 2 key frames
    float fPct = ( pAnimation->mfTime - (float)pAnimation->miAnimFrame * fTimePerKey ) / fTimePerKey;
    tModel* pModel = pAnimation->mpMorphTarget->mpModel;
    
    tJob job;
    tMorphJobData jobData;
    jobData.maNextFrameVertNorm = aNextFrameVertNorm;
    jobData.maNextFrameVertPos = aNextFrameVertPos;
    jobData.maPrevFrameVertNorm = aPrevFrameVertNorm;
    jobData.maPrevFrameVertPos = aPrevFrameVertPos;
    jobData.maResultVertNorm = pModel->maNorm;
    jobData.maResultVertPos = pModel->maPos;
    jobData.mfPct = fPct;
    jobData.miNumVerts = pModel->miNumVerts;
    
    job.mpfnFunc = &computeMorph;
    job.mpData = &jobData;
    job.miDataSize = sizeof( jobData );
    
    jobManagerAddJob( gpJobManager, &job );
    
#if 0
    tVector4* pVertPos = pModel->maPos;
    tVector4 const* pPrevPos = aPrevFrameVertPos;
    tVector4 const* pNextPos = aNextFrameVertPos;
    
	tVector4* pVertNorm = pModel->maNorm;
	tVector4 const* pPrevNorm = aPrevFrameVertNorm;
	tVector4 const* pNextNorm = aNextFrameVertNorm;
    
    // interpolate between the 2 key frames
    for( int i = 0; i < pModel->miNumVerts; i++ )
    {
        pVertPos->fX = pPrevPos->fX + ( pNextPos->fX - pPrevPos->fX ) * fPct;
        pVertPos->fY = pPrevPos->fY + ( pNextPos->fY - pPrevPos->fY ) * fPct;
        pVertPos->fZ = pPrevPos->fZ + ( pNextPos->fZ - pPrevPos->fZ ) * fPct;

		pVertNorm->fX = pPrevNorm->fX + ( pNextNorm->fX - pPrevNorm->fX ) * fPct;
        pVertNorm->fY = pPrevNorm->fY + ( pNextNorm->fY - pPrevNorm->fY ) * fPct;
        pVertNorm->fZ = pPrevNorm->fZ + ( pNextNorm->fZ - pPrevNorm->fZ ) * fPct;
        
        ++pVertPos;
        ++pPrevPos;
        ++pNextPos;

		++pVertNorm;
		++pPrevNorm;
		++pNextNorm;
        
    }   // for i = 0 to num vertices
#endif // #if 0
}

/*
**
*/
static void loadFrames( tMorphTarget* pMorphTarget, TiXmlNode* pNode )
{
    TiXmlNode* pFrameNode = pNode;
    while( pFrameNode )
    {
        // frame index
        const char* szValue = pFrameNode->FirstChild( "index" )->FirstChild()->Value();
        int iFrameIndex = atoi( szValue );
        
        WTFASSERT2( iFrameIndex >= 0 && iFrameIndex < pMorphTarget->miNumFrames, "animation frame out of bounds" );
        tVector4* aVertPos = pMorphTarget->maaAnimVertPos[iFrameIndex];
		tVector4* aNorm = pMorphTarget->maaAnimVertNorm[iFrameIndex];

        int iVertex = 0;
        TiXmlNode* pVertexNode = pFrameNode->FirstChild( "vertex" );
        
        // vertex position
        tVector4 pos = { 0.0f, 0.0f, 0.0f, 1.0f };
        while( pVertexNode )
        {
            WTFASSERT2( iVertex >= 0 && iVertex < pMorphTarget->mpModel->miNumVerts, "animation vertex out of bounds" );
            
            const char* szPos = pVertexNode->FirstChild()->Value();
            parseVector( &pos, szPos );
            memcpy( &aVertPos[iVertex], &pos, sizeof( tVector4 ) );
            
            pVertexNode = pVertexNode->NextSibling( "vertex" );
            ++iVertex;
        }
        
		// normal position
		int iNormal = 0;
		TiXmlNode* pNormalNode = pFrameNode->FirstChild( "normal" );
		while( pNormalNode )
        {
            WTFASSERT2( iNormal >= 0 && iNormal < pMorphTarget->mpModel->miNumNormals, "animation normal out of bounds" );
            
            const char* szPos = pNormalNode->FirstChild()->Value();
            parseVector( &pos, szPos );
            memcpy( &aNorm[iNormal], &pos, sizeof( tVector4 ) );
            
            pNormalNode = pNormalNode->NextSibling( "normal" );
            ++iNormal;
        }

        pFrameNode = pFrameNode->NextSibling( "frame" );
    }
}

/*
**
*/
static void computeMorph( void* pData, void* pJobDebugData )
{
	//int iJobQueueIndex = ((tJobDebugData *)pJobDebugData)->miQueueIndex;
	//int iNumInQueue = ((tJobDebugData *)pJobDebugData)->miNumInQueue;

    tMorphJobData* pJobData = (tMorphJobData *)pData;
    
    float fPct = pJobData->mfPct;
    int iNumVerts = pJobData->miNumVerts;
    
    tVector4* pVertPos = pJobData->maResultVertPos;
    tVector4 const* pPrevPos = pJobData->maPrevFrameVertPos;
    tVector4 const* pNextPos = pJobData->maNextFrameVertPos;
    
    tVector4* pVertNorm = pJobData->maResultVertNorm;
    tVector4 const* pPrevNorm = pJobData->maPrevFrameVertNorm;
    tVector4 const* pNextNorm = pJobData->maNextFrameVertNorm;
    
    // interpolate between the 2 key frames
    for( int i = 0; i < iNumVerts; i++ )
    {
        pVertPos->fX = pPrevPos->fX + ( pNextPos->fX - pPrevPos->fX ) * fPct;
        pVertPos->fY = pPrevPos->fY + ( pNextPos->fY - pPrevPos->fY ) * fPct;
        pVertPos->fZ = pPrevPos->fZ + ( pNextPos->fZ - pPrevPos->fZ ) * fPct;
        
        pVertNorm->fX = pPrevNorm->fX + ( pNextNorm->fX - pPrevNorm->fX ) * fPct;
        pVertNorm->fY = pPrevNorm->fY + ( pNextNorm->fY - pPrevNorm->fY ) * fPct;
        pVertNorm->fZ = pPrevNorm->fZ + ( pNextNorm->fZ - pPrevNorm->fZ ) * fPct;
        
        ++pVertPos;
        ++pPrevPos;
        ++pNextPos;
        
        ++pVertNorm;
        ++pPrevNorm;
        ++pNextNorm;
        
    }   // for i = 0 to num vertices
}