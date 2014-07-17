//
//  animsequence.cpp
//  animtest
//
//  Created by Tony Peng on 6/25/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "animsequence.h"
#include "filepathutil.h"
#include "tinyxml.h"
#include "jobmanager.h"

struct AnimFrame
{
    float               mfTime;
    tVector4            mPosition;
    tQuaternion         mRotation;
    tVector4            mScaling;
    tVector4            mRotationVec;
    
    tJoint*             mpJoint;
};

typedef struct AnimFrame tAnimFrame;

struct KeyFrameJobData
{
    tVector4 const*       mpPos0;
    tVector4 const*       mpPos1;
    
    tVector4 const*       mpScale0;
    tVector4 const*       mpScale1;
    
    tQuaternion const*    mpRot0;
    tQuaternion const*    mpRot1;
    
    tVector4 const*       mpRotVec0;
    tVector4 const*       mpRotVec1;
    
    float           mfPct;
    
    tVector4*       mpResultPos;
    tVector4*       mpResultScale;
    tQuaternion*    mpResultRot;
    
    tVector4*       mpResultRotVec;
    
    tJoint const*   mpJoint;
};

typedef struct KeyFrameJobData tKeyFrameJobData;

struct MatrixJobData
{
    tVector4 const*         mpPosition;
    tVector4 const*         mpScale;
    tQuaternion const*      mpRotation;
    
    tVector4 const*         mpRotationVec;
    
    tMatrix44*              mpResultMat;
    
    tJoint*                 mpJoint;
};

typedef struct MatrixJobData tMatrixJobData;

static void sortOnJoints( tAnimSequence* pAnimSequence );
static void sortOnTime( tAnimSequence* pAnimSequence );


/*
**
*/
void animSequenceInit( tAnimSequence* pAnimSequence )
{
    pAnimSequence->mafTime = NULL;
    pAnimSequence->maPositions = NULL;
    pAnimSequence->maRotation = NULL;
    pAnimSequence->maScalings = NULL;
    
    pAnimSequence->miNumFrames = 0;
    pAnimSequence->miNumJoints = 0;
}

/*
**
*/
void animSequenceRelease( tAnimSequence* pAnimSequence )
{
    FREE( pAnimSequence->mafTime );
    FREE( pAnimSequence->mapJoints );
    FREE( pAnimSequence->mapUniqueJoints );
    FREE( pAnimSequence->maiStartFrames );
    FREE( pAnimSequence->maiEndFrames );
}

/*
**
*/
void animSequenceLoad( tAnimSequence* pAnimSequence,
                       const char* szFileName,
                       tAnimHierarchy const* pAnimHierarchy,
                       CFactory<tVector4>* pVectorFactory,
                       CFactory<tQuaternion>* pQuaternionFactory )
{
    const float fDegreeToRadian = 3.14159f / 180.0f;
    
    char szFullPath[256];
    getFullPath( szFullPath, szFileName );
    
    TiXmlDocument doc( szFullPath );
    bool bLoaded = doc.LoadFile();
    if( bLoaded )
    {
        pAnimSequence->miNumFrames = 0;
        
        TiXmlNode* pNode = doc.FirstChild( "jointKeyframe" )->FirstChild( "keyframe" );
        while( pNode )
        {
            ++pAnimSequence->miNumFrames;
            pNode = pNode->NextSibling( "keyframe" );
        }
        
        pAnimSequence->mafTime = (float *)MALLOC( sizeof( float ) * pAnimSequence->miNumFrames );
        pAnimSequence->maPositions = pVectorFactory->alloc( pAnimSequence->miNumFrames );
        pAnimSequence->maRotation = pQuaternionFactory->alloc( pAnimSequence->miNumFrames );
        pAnimSequence->maScalings = pVectorFactory->alloc( pAnimSequence->miNumFrames );
        pAnimSequence->mapJoints = (tJoint **)MALLOC( sizeof( tJoint* ) * pAnimSequence->miNumFrames );
        
        pAnimSequence->maRotationVec = pVectorFactory->alloc( pAnimSequence->miNumFrames );
        
        pAnimSequence->miNumJoints = 0;
        pAnimSequence->mapUniqueJoints = (tJoint **)MALLOC( sizeof( tJoint* ) * pAnimSequence->miNumFrames );
        memset( pAnimSequence->mapUniqueJoints, 0, sizeof( tJoint* ) * pAnimSequence->miNumFrames );
        
        pAnimSequence->mfLastTime = 0.0f;
        
        int iFrame = 0;
        pNode = doc.FirstChild( "jointKeyframe" )->FirstChild( "keyframe" );
        while( pNode )
        {
            TiXmlElement* pElement = pNode->ToElement();
            
            const char* szName = pElement->Attribute( "joint" );
            const char* szTime = pElement->Attribute( "time" );
            
            const char* szRotX = pElement->Attribute( "rotatex" );
            const char* szRotY = pElement->Attribute( "rotatey" );
            const char* szRotZ = pElement->Attribute( "rotatez" );
            
            const char* szTranslateX = pElement->Attribute( "translatex" );
            const char* szTranslateY = pElement->Attribute( "translatey" );
            const char* szTranslateZ = pElement->Attribute( "translatez" );
            
            // 24 frames / sec
            pAnimSequence->mafTime[iFrame] = ( (float)atof( szTime ) - 1.0f ) / 23.0f;
            if( pAnimSequence->mfLastTime < pAnimSequence->mafTime[iFrame] )
            {
                pAnimSequence->mfLastTime = pAnimSequence->mafTime[iFrame];
            }
            
            tVector4* pPos = &pAnimSequence->maPositions[iFrame];
            tVector4* pScaling = &pAnimSequence->maScalings[iFrame];
            tQuaternion* pRotation = &pAnimSequence->maRotation[iFrame];
            
            tVector4* pRotVec = &pAnimSequence->maRotationVec[iFrame];
            
            tVector4 xAxis = { 1.0f, 0.0f, 0.0f, 1.0f };
            tVector4 yAxis = { 0.0f, 1.0f, 0.0f, 1.0f };
            tVector4 zAxis = { 0.0f, 0.0f, 1.0f, 1.0f };
            
            // total rotation
            tQuaternion xRot, yRot, zRot;
            quaternionInit( &xRot );
            quaternionInit( &yRot );
            quaternionInit( &zRot );
            
            float fRotX = (float)atof( szRotX );
            float fRotY = (float)atof( szRotY );
            float fRotZ = (float)atof( szRotZ );
            
            fRotX *= fDegreeToRadian;
            fRotY *= fDegreeToRadian;
            fRotZ *= fDegreeToRadian;
            
            quaternionFromAxisAngle( &xRot, &xAxis, fRotX );
            quaternionFromAxisAngle( &yRot, &yAxis, fRotY );
            quaternionFromAxisAngle( &zRot, &zAxis, fRotZ );
            
            tQuaternion zyRot;
            quaternionMultiply( &zyRot, &zRot, &yRot );
            quaternionMultiply( pRotation, &zyRot, &xRot );
            
            // scale
            pScaling->fX = pScaling->fY = pScaling->fZ = pScaling->fW = 1.0f;
            
            // position
            pPos->fX = (float)atof( szTranslateX );
            pPos->fY = (float)atof( szTranslateY );
            pPos->fZ = (float)atof( szTranslateZ );
            pPos->fW = 1.0f;
            
            // rotation vector
            pRotVec->fX = fRotX;
            pRotVec->fY = fRotY;
            pRotVec->fZ = fRotZ;
            pRotVec->fW = 1.0f;
            
            // look for the joint in the hierarchy
            int iNumJoints = pAnimHierarchy->miNumJoints;
            for( int i = 0; i < iNumJoints; i++ )
            {
                if( !strcmp( szName, pAnimHierarchy->maJoints[i].mszName ) )
                {
                    pAnimSequence->mapJoints[iFrame] = &pAnimHierarchy->maJoints[i];
                    break;
                }
            }
            
            //OUTPUT( "%s time = %.1f rot ( %f, %f, %f )\n", szName, pAnimSequence->mafTime[iFrame], fRotX, fRotY, fRotZ );
            
            pNode = pNode->NextSibling( "keyframe" );
            
            ++iFrame;
        }
    }
    
    // unique joints in the hierarchy
    pAnimSequence->mapUniqueJoints = (tJoint **)MALLOC( sizeof( tJoint* ) * pAnimHierarchy->miNumJoints );
    pAnimSequence->miNumJoints = pAnimHierarchy->miNumJoints;
    for( int i = 0; i < pAnimHierarchy->miNumJoints; i++ )
    {
        pAnimSequence->mapUniqueJoints[i] = &pAnimHierarchy->maJoints[i];
    }
    
    // sort using joint as the index key
    sortOnJoints( pAnimSequence );
    pAnimSequence->maiStartFrames = (unsigned int *)MALLOC( sizeof( int ) * pAnimSequence->miNumJoints );
    pAnimSequence->maiEndFrames = (unsigned int *)MALLOC( sizeof( int ) * pAnimSequence->miNumJoints );

    int iNumFrames = pAnimSequence->miNumFrames;
    int iNumJoints = pAnimSequence->miNumJoints;
    
    for( int i = 0; i < iNumJoints; i++ )
    {
        pAnimSequence->maiStartFrames[i] = -1;
        pAnimSequence->maiEndFrames[i] = -1;
    }

    // get start and end indices of the joint's keyframes
    for( int i = 0; i < iNumJoints; i++ )
    {
        tJoint* pJoint = pAnimSequence->mapUniqueJoints[i];
        for( int j = 0; j < iNumFrames; j++ )
        {
            // find start
            if( pAnimSequence->mapJoints[j] == pJoint )
            {
                // find end
                pAnimSequence->maiStartFrames[i] = j;
                for( int k = j + 1; k < iNumFrames; k++ )
                {
                    if( pAnimSequence->mapJoints[k] != pJoint )
                    {
                        pAnimSequence->maiEndFrames[i] = k - 1;
                        break;
                    }
                }
                
                if( pAnimSequence->maiEndFrames[i] == -1 )
                {
                    pAnimSequence->maiEndFrames[i] = iNumFrames - 1;
                }
                
                break;
            }
            
        }   // for j = 0 to num frames
        
    }   // for i = 0 to num joints
    
    /*for( int i = 0; i < pAnimSequence->miNumFrames; i++ )
    {
        OUTPUT( "%s time = %.1f rot ( %f, %f, %f )\n",
               pAnimSequence->mapJoints[i]->mszName,
               pAnimSequence->mafTime[i],
               pAnimSequence->maRotationVec[i].fX,
               pAnimSequence->maRotationVec[i].fY,
               pAnimSequence->maRotationVec[i].fZ );
    }*/
    
    sortOnTime( pAnimSequence );
    
    /*for( int i = 0; i < pAnimSequence->miNumFrames; i++ )
    {
        OUTPUT( "%s time = %.1f rot ( %f, %f, %f )\n",
                pAnimSequence->mapJoints[i]->mszName,
                pAnimSequence->mafTime[i],
                pAnimSequence->maRotationVec[i].fX,
                pAnimSequence->maRotationVec[i].fY,
               pAnimSequence->maRotationVec[i].fZ );
    }
    
    for( int i = 0; i < pAnimSequence->miNumJoints; i++ )
    {
        OUTPUT( "%s\t( %d, %d )\n",
                pAnimSequence->mapUniqueJoints[i]->mszName,
                pAnimSequence->maiStartFrames[i],
                pAnimSequence->maiEndFrames[i] );
    }*/
}

/*
**
*/
void animSequenceUpdateJointAnimValues( tAnimSequence const* pAnimSequence,
                                        tAnimHierarchy const* pAnimHierarchy,
                                        tVector4* aPositions,
                                        tVector4* aScalings,
                                        tQuaternion* aRotations,
                                        tVector4* aRotationVecs,
                                        float fTime )
{
    float fNumLoops = 0.0f;
	
	//if( pAnimSequence->mfLastTime > 0.0f )
	//{
	//	fNumLoops = floorf( fTime / pAnimSequence->mfLastTime );
	//}

	float fFrameTime = fTime - fNumLoops * pAnimSequence->mfLastTime;
    
    // key frame for the joints
    int iNumJoints = pAnimSequence->miNumJoints;
    for( int i = 0; i < iNumJoints; i++ )
    {
        int iStart = pAnimSequence->maiStartFrames[i];
        int iEnd = pAnimSequence->maiEndFrames[i];
        
        // no valid animation frame for this joint
        if( iStart < 0 || iEnd < 0 )
        {
            continue;
        }
        
        tJoint* pJoint = pAnimSequence->mapJoints[iStart];
        
        // find the ending frame
        int iEndFrame = iStart;
        for( iEndFrame = iStart; iEndFrame <= iEnd; iEndFrame++ )
        {
            if( pAnimSequence->mafTime[iEndFrame] > fFrameTime )
            {
                break;
            }
            
        }   // for end frame = start to end
        
        // didn't find end frame
        if( iEndFrame > iEnd )
        {
            iEndFrame = iEnd;
        }
        
        int iStartFrame = iEndFrame - 1;
        if( iStartFrame < 0 )
        {
            iStartFrame = 0;
        }
        
        // just at the start
        if( iStartFrame < iStart )
        {
            iStartFrame = iStart;
        }
        
        WTFASSERT2( iStartFrame >= iStart && iStartFrame <= iEnd, "out of bounds looking for start animation frame" );
        WTFASSERT2( iEndFrame >= iStart && iEndFrame <= iEnd, "out of bounds looking for end animation frame" );
        
        float fTimeDuration = pAnimSequence->mafTime[iEndFrame] - pAnimSequence->mafTime[iStartFrame];
        float fTimeFromStart = fFrameTime - pAnimSequence->mafTime[iStartFrame];
        float fPct = 0.0f;
        
        if( fTimeDuration > 0.0f )
        {
            fPct = fTimeFromStart / fTimeDuration;
        }
        
        // clamp pct
        if( fPct > 1.0f )
        {
            fPct = 1.0f;
        }
        
        int iJointIndex = 0;
        int iNumHierarchyJoints = pAnimHierarchy->miNumJoints;
        for( iJointIndex = 0; iJointIndex < iNumHierarchyJoints; iJointIndex++ )
        {
            if( !strcmp( pJoint->mszName, pAnimHierarchy->maJoints[iJointIndex].mszName ) )
            {
                break;
            }
        }
        
        WTFASSERT2( iJointIndex == i, "TEST" );
        WTFASSERT2( iJointIndex < iNumHierarchyJoints, "can't find joint in hierarchy" );
        
        Vector4Lerp( &aPositions[i],
                     &pAnimSequence->maPositions[iStartFrame],
                     &pAnimSequence->maPositions[iEndFrame],
                     fPct );
        
        Vector4Lerp( &aScalings[i],
                     &pAnimSequence->maScalings[iStartFrame],
                     &pAnimSequence->maScalings[iEndFrame],
                     fPct );
        
        quaternionSlerp( &aRotations[i],
                         &pAnimSequence->maRotation[iStartFrame],
                         &pAnimSequence->maRotation[iEndFrame],
                         fPct );
        
        Vector4Lerp( &aRotationVecs[i],
                     &pAnimSequence->maRotationVec[iStartFrame],
                     &pAnimSequence->maRotationVec[iEndFrame],
                     fPct );
        
#if 0
        tJob job;
        tKeyFrameJobData interpJobData;
        
        interpJobData.mfPct = fPct;
        interpJobData.mpPos0 = &pAnimSequence->maPositions[iStartFrame];
        interpJobData.mpPos1 = &pAnimSequence->maPositions[iEndFrame];
        interpJobData.mpScale0 = &pAnimSequence->maScalings[iStartFrame];
        interpJobData.mpScale1 = &pAnimSequence->maScalings[iEndFrame];
        interpJobData.mpRot0 = &pAnimSequence->maRotation[iStartFrame];
        interpJobData.mpRot1 = &pAnimSequence->maRotation[iEndFrame];
        interpJobData.mpResultPos = &aPositions[iJointIndex];
        interpJobData.mpResultScale = &aScalings[iJointIndex];
        interpJobData.mpResultRot = &aRotations[iJointIndex];
        interpJobData.mpJoint = pJoint;
        
        interpJobData.mpRotVec0 = &pAnimSequence->maRotationVec[iStartFrame];
        interpJobData.mpRotVec1 = &pAnimSequence->maRotationVec[iEndFrame];
        interpJobData.mpResultRotVec = &aRotationVecs[iJointIndex];
        
        job.mpfnFunc = interpolateJob;
        job.mpData = &interpJobData;
        job.miDataSize = sizeof( tKeyFrameJobData );
        
        jobManagerAddJob( gpJobManager, &job );
#endif // #if 0
        
    }   // for i = 0 to num joints
}

/*
**
*/
void animSequenceUpdateAnimMatrices( tAnimSequence const* pAnimSequence,
                                     tAnimHierarchy const* pAnimHierarchy,
                                     tVector4* aPositions,
                                     tVector4* aScalings,
                                     tQuaternion* aRotations,
                                     tVector4* aRotationVecs,
                                     tMatrix44* aAnimMatrices )
{
    int iNumJoints = pAnimSequence->miNumJoints;
    for( int i = 0; i < iNumJoints; i++ )
    {
        int iStart = pAnimSequence->maiStartFrames[i];
        int iEnd = pAnimSequence->maiEndFrames[i];
        
        // no valid animation frame for this joint
        if( iStart < 0 || iEnd < 0 )
        {
            continue;
        }
        
        tJoint* pJoint = pAnimSequence->mapJoints[iStart];
        
        // get corresponding joint index from hierarchy
        int iJointIndex = 0;
        int iNumHierarchyJoints = pAnimHierarchy->miNumJoints;
        for( iJointIndex = 0; iJointIndex < iNumHierarchyJoints; iJointIndex++ )
        {
            if( !strcmp( pAnimHierarchy->maJoints[iJointIndex].mszName, pJoint->mszName ) )
            {
                break;
            }
        }
        
        WTFASSERT2( iJointIndex < iNumHierarchyJoints, "can't find corresponding joint" );
        
        tMatrix44 posMatrix, scaleMatrix, rotMatrix, posRotMatrix;
        //quaternionToMatrix( &rotMatrix, pMatrixData->mpRotation );
        
        tMatrix44 rotMatZY, rotMatX, rotMatY, rotMatZ;
        Matrix44RotateX( &rotMatX, aRotationVecs[i].fX );
        Matrix44RotateY( &rotMatY, aRotationVecs[i].fY );
        Matrix44RotateZ( &rotMatZ, aRotationVecs[i].fZ );
        
        Matrix44Multiply( &rotMatZY, &rotMatZ, &rotMatY );
        Matrix44Multiply( &rotMatrix, &rotMatZY, &rotMatX );
        
        
        Matrix44Scale( &scaleMatrix, aScalings[i].fX, aScalings[i].fY, aScalings[i].fZ );
        Matrix44Translate( &posMatrix, &aPositions[i] );
        Matrix44Multiply( &posRotMatrix, &posMatrix, &rotMatrix );
        
        Matrix44Multiply( &aAnimMatrices[iJointIndex], &posRotMatrix, &scaleMatrix );
        
#if 0
        tJob job;
        tMatrixJobData matrixJobData;
        matrixJobData.mpPosition = &aPositions[i];
        matrixJobData.mpRotation = &aRotations[i];
        matrixJobData.mpScale = &aScalings[i];
        matrixJobData.mpResultMat = &aAnimMatrices[iJointIndex];
        matrixJobData.mpJoint = pAnimSequence->mapUniqueJoints[i];
        
        matrixJobData.mpRotationVec = &aRotationVecs[i];
        
        job.mpfnFunc = &matrixUpdateJob;
        job.mpData = &matrixJobData;
        job.miDataSize = sizeof( tMatrixJobData );
        
        jobManagerAddJob( gpJobManager, &job );
#endif // #if 0
                
    }   // for i = 0 to num joints
}

/*
**
*/
void animSequencePlay( tAnimSequence const* pAnimSequence,
                       tAnimHierarchy const* pAnimHierarchy,
                       tVector4* aPositions,
                       tVector4* aScalings,
                       tQuaternion* aRotations,
                       tVector4* aRotationVecs,
                       tMatrix44* aAnimMatrices,
                       float fTime )
{
    float fNumLoops = floorf( fTime / pAnimSequence->mfLastTime );
    float fFrameTime = fTime - fNumLoops * pAnimSequence->mfLastTime;
    
    // key frame for the joints
    int iNumJoints = pAnimSequence->miNumJoints;
    for( int i = 0; i < iNumJoints; i++ )
    {
        int iStart = pAnimSequence->maiStartFrames[i];
        int iEnd = pAnimSequence->maiEndFrames[i];
        
        // no valid animation frame for this joint
        if( iStart < 0 || iEnd < 0 )
        {
            continue;
        }
     
        // find the ending frame
        int iEndFrame = iStart;
        for( iEndFrame = iStart; iEndFrame <= iEnd; iEndFrame++ )
        {
            if( pAnimSequence->mafTime[iEndFrame] > fFrameTime )
            {
                break;
            }
            
        }   // for end frame = start to end
        
        // didn't find end frame
        if( iEndFrame > iEnd )
        {
            iEndFrame = iEnd;
        }
        
        int iStartFrame = iEndFrame - 1;
        if( iStartFrame < 0 )
        {
            iStartFrame = 0;
        }
        
        // just at the start
        if( iStartFrame < iStart )
        {
            iStartFrame = iStart;
        }
        
        WTFASSERT2( iStartFrame >= iStart && iStartFrame <= iEnd, "out of bounds looking for start animation frame" );
        WTFASSERT2( iEndFrame >= iStart && iEndFrame <= iEnd, "out of bounds looking for end animation frame" );
        
        float fTimeDuration = pAnimSequence->mafTime[iEndFrame] - pAnimSequence->mafTime[iStartFrame];
        float fTimeFromStart = fFrameTime - pAnimSequence->mafTime[iStartFrame];
        float fPct = 0.0f;
        
        if( fTimeDuration > 0.0f )
        {
            fPct = fTimeFromStart / fTimeDuration;
        }
        
        // clamp pct
        if( fPct > 1.0f )
        {
            fPct = 1.0f;
        }
        
        Vector4Lerp( &aPositions[i],
                     &pAnimSequence->maPositions[iStartFrame],
                     &pAnimSequence->maPositions[iEndFrame],
                     fPct );
        
        Vector4Lerp( &aScalings[i],
                     &pAnimSequence->maScalings[iStartFrame],
                     &pAnimSequence->maScalings[iEndFrame],
                     fPct );
        
        quaternionSlerp( &aRotations[i],
                         &pAnimSequence->maRotation[iStartFrame],
                         &pAnimSequence->maRotation[iEndFrame],
                         fPct );
        
        Vector4Lerp( &aRotationVecs[i],
                     &pAnimSequence->maRotationVec[iStartFrame],
                     &pAnimSequence->maRotationVec[iEndFrame],
                     fPct );
        
#if 0
        tJoint* pJoint = pAnimSequence->mapJoints[iStart];
        
        tJob job;
        tKeyFrameJobData interpJobData;
        
        interpJobData.mfPct = fPct;
        interpJobData.mpPos0 = &pAnimSequence->maPositions[iStartFrame];
        interpJobData.mpPos1 = &pAnimSequence->maPositions[iEndFrame];
        interpJobData.mpScale0 = &pAnimSequence->maScalings[iStartFrame];
        interpJobData.mpScale1 = &pAnimSequence->maScalings[iEndFrame];
        interpJobData.mpRot0 = &pAnimSequence->maRotation[iStartFrame];
        interpJobData.mpRot1 = &pAnimSequence->maRotation[iEndFrame];
        interpJobData.mpResultPos = &aPositions[i];
        interpJobData.mpResultScale = &aScalings[i];
        interpJobData.mpResultRot = &aRotations[i];
        interpJobData.mpJoint = pJoint;
        
        interpJobData.mpRotVec0 = &pAnimSequence->maRotationVec[iStartFrame];
        interpJobData.mpRotVec1 = &pAnimSequence->maRotationVec[iEndFrame];
        interpJobData.mpResultRotVec = &aRotationVecs[i];
        
        job.mpfnFunc = interpolateJob;
        job.mpData = &interpJobData;
        job.miDataSize = sizeof( tKeyFrameJobData );
        
        jobManagerAddJob( gpJobManager, &job );
#endif // #if 0
    }

#if 0
        tVector4* pScale = &aScalings[i];
        tVector4* pPosition = &aPositions[i];
        
        // interpolate position, scale, and rotation
        Vector4Lerp( pPosition,
                     &pAnimSequence->maPositions[iEndFrame],
                     &pAnimSequence->maPositions[iStartFrame],
                     fPct );
        
        Vector4Lerp( pScale,
                     &pAnimSequence->maScalings[iEndFrame],
                     &pAnimSequence->maScalings[iStartFrame],
                     fPct );
        
        quaternionSlerp( &aRotations[i],
                         &pAnimSequence->maRotation[iEndFrame],
                         &pAnimSequence->maRotation[iStartFrame],
                         fPct );
    
    jobManagerWait( gpJobManager );
#endif // #if 0
    
    for( int i = 0; i < iNumJoints; i++ )
    {
        int iStart = pAnimSequence->maiStartFrames[i];
        int iEnd = pAnimSequence->maiEndFrames[i];
        
        // no valid animation frame for this joint
        if( iStart < 0 || iEnd < 0 )
        {
            continue;
        }
        
#if 0
        tJob job;
        tMatrixJobData matrixJobData;
        matrixJobData.mpPosition = &aPositions[i];
        matrixJobData.mpRotation = &aRotations[i];
        matrixJobData.mpScale = &aScalings[i];
        matrixJobData.mpResultMat = &aAnimMatrices[i];
        matrixJobData.mpJoint = pAnimSequence->mapUniqueJoints[i];
        
        matrixJobData.mpRotationVec = &aRotationVecs[i];
        
        job.mpfnFunc = &matrixUpdateJob;
        job.mpData = &matrixJobData;
        job.miDataSize = sizeof( tMatrixJobData );
        
        jobManagerAddJob( gpJobManager, &job );
#endif // #if 0
        
        tMatrix44 posMatrix, scaleMatrix, rotMatrix, posRotMatrix;
        //quaternionToMatrix( &rotMatrix, pMatrixData->mpRotation );
        
        tMatrix44 rotMatZY, rotMatX, rotMatY, rotMatZ;
        Matrix44RotateX( &rotMatX, aRotationVecs[i].fX );
        Matrix44RotateY( &rotMatY, aRotationVecs[i].fY );
        Matrix44RotateZ( &rotMatZ, aRotationVecs[i].fZ );
        
        Matrix44Multiply( &rotMatZY, &rotMatZ, &rotMatY );
        Matrix44Multiply( &rotMatrix, &rotMatZY, &rotMatX );
        
        
        Matrix44Scale( &scaleMatrix, aScalings[i].fX, aScalings[i].fY, aScalings[i].fZ );
        Matrix44Translate( &posMatrix, &aPositions[i] );
        Matrix44Multiply( &posRotMatrix, &posMatrix, &rotMatrix );
        
        Matrix44Multiply( &aAnimMatrices[i], &posRotMatrix, &scaleMatrix );
        
#if 0
        // concat for animation matrix
        tMatrix44 posMatrix, scaleMatrix, rotMatrix, posRotMatrix;
        quaternionToMatrix( &rotMatrix, &aRotations[i] );
        Matrix44Scale( &scaleMatrix, pScale->fX, pScale->fY, pScale->fZ );
        Matrix44Translate( &posMatrix, pPosition );
        Matrix44Multiply( &posRotMatrix, &posMatrix, &rotMatrix );
        
        tMatrix44* pAnimMatrix = &aAnimMatrices[i];
        Matrix44Multiply( pAnimMatrix, &posRotMatrix, &scaleMatrix );
#endif // #if 0
        
    }   // for i = 0 to num joints
    
    jobManagerWait( gpJobManager );
}

#if 0
/*
**
*/
static void interpolateJob( void* pData )
{
    tKeyFrameJobData* pJobData = (tKeyFrameJobData *)pData;
    Vector4Lerp( pJobData->mpResultPos,
                 pJobData->mpPos1,
                 pJobData->mpPos0,
                 pJobData->mfPct );
    
    Vector4Lerp( pJobData->mpResultScale,
                 pJobData->mpScale0,
                 pJobData->mpScale1,
                 pJobData->mfPct );
    
    quaternionSlerp( pJobData->mpResultRot,
                     pJobData->mpRot0,
                     pJobData->mpRot1,
                     pJobData->mfPct );
    
    Vector4Lerp( pJobData->mpResultRotVec,
                 pJobData->mpRotVec0,
                 pJobData->mpRotVec1,
                 pJobData->mfPct );
    
}

/*
**
*/
static void matrixUpdateJob( void* pData )
{
    tMatrixJobData* pMatrixData = (tMatrixJobData *)pData;
    tVector4 const* pScale = pMatrixData->mpScale;
    //tJoint* pJoint = pMatrixData->mpJoint;
    
    // concat for animation matrix
    tMatrix44 posMatrix, scaleMatrix, rotMatrix, posRotMatrix;
    //quaternionToMatrix( &rotMatrix, pMatrixData->mpRotation );
    
    tMatrix44 rotMatZY, rotMatX, rotMatY, rotMatZ;
    Matrix44RotateX( &rotMatX, pMatrixData->mpRotationVec->fX );
    Matrix44RotateY( &rotMatY, pMatrixData->mpRotationVec->fY );
    Matrix44RotateZ( &rotMatZ, pMatrixData->mpRotationVec->fZ );
    
    Matrix44Multiply( &rotMatZY, &rotMatZ, &rotMatY );
    Matrix44Multiply( &rotMatrix, &rotMatZY, &rotMatX );
    
    
    Matrix44Scale( &scaleMatrix, pScale->fX, pScale->fY, pScale->fZ );
    Matrix44Translate( &posMatrix, pMatrixData->mpPosition );
    Matrix44Multiply( &posRotMatrix, &posMatrix, &rotMatrix );
    
    // result
    Matrix44Multiply( pMatrixData->mpResultMat, &posRotMatrix, &scaleMatrix );
    //Matrix44Multiply( pJoint->mpAnimMatrix, &posRotMatrix, &scaleMatrix );
}

/*
**
*/
static void sortAnimSequence( tAnimSequence* pAnimSequence )
{
    // copy animation frames
    int iNumFrames = pAnimSequence->miNumFrames;
    tAnimFrame* aAnimFrames = (tAnimFrame *)malloc( sizeof( tAnimFrame ) * iNumFrames );
    for( int i = 0; i < iNumFrames; i++ )
    {
        tAnimFrame* pFrame = &aAnimFrames[i];
        pFrame->mfTime = pAnimSequence->mafTime[i];
        memcpy( &pFrame->mPosition, &pAnimSequence->maPositions[i], sizeof( tVector4 ) );
        memcpy( &pFrame->mRotation, &pAnimSequence->maRotation[i], sizeof( tQuaternion ) );
        memcpy( &pFrame->mScaling, &pAnimSequence->maScalings[i], sizeof( tVector4 ) );
        
        pFrame->mpJoint = pAnimSequence->mapJoints[i];
    }
    
    // sort by time
    for( int i = 0; i < iNumFrames; i++ )
    {
        tAnimFrame* pCurrFrame = &aAnimFrames[i];
        
        for( int j = i + 1; j < iNumFrames; j++ )
        {
            tAnimFrame* pIterFrame = &aAnimFrames[j];
            if( pCurrFrame->mfTime > pIterFrame->mfTime )
            {
                // swap
                tAnimFrame temp;
                memcpy( &temp, pCurrFrame, sizeof( tAnimFrame ) );
                memcpy( pCurrFrame, pIterFrame, sizeof( tAnimFrame ) );
                memcpy( pIterFrame, &temp, sizeof( tAnimFrame ) );
                
                pCurrFrame = &aAnimFrames[i];
            }
            
        }   // for j = i to num frames
        
    }   // for i = 0 to num frames
    
    for( int i = 0; i < iNumFrames; i++ )
    {
        tAnimFrame* pCurrFrame = &aAnimFrames[i];
        for( int j = i + 1; j < iNumFrames; j++ )
        {
            tAnimFrame* pIterFrame = &aAnimFrames[j];
            
            // check if it's same time
            if( fabs( pIterFrame->mfTime - pCurrFrame->mfTime ) < 0.001f )
            {
                // sort based on address in the array
                if( (unsigned int)pIterFrame->mpJoint < (unsigned int)pCurrFrame->mpJoint )
                {
                    // swap
                    tAnimFrame temp;
                    memcpy( &temp, pCurrFrame, sizeof( tAnimFrame ) );
                    memcpy( pCurrFrame, pIterFrame, sizeof( tAnimFrame ) );
                    memcpy( pIterFrame, &temp, sizeof( tAnimFrame ) );
                    
                    pCurrFrame = &aAnimFrames[i];
                }
            }   // if same keyframe time
            
        }   // for j = i + 1 to num frames
        
    }   // for i = 0 to num frames
    
    // reset the arrays
    for( int i = 0; i < iNumFrames; i++ )
    {
        tAnimFrame* pFrame = &aAnimFrames[i];
        pAnimSequence->mafTime[i] = pFrame->mfTime;
        pAnimSequence->mapJoints[i] = pFrame->mpJoint;
        memcpy( &pAnimSequence->maPositions[i], &pFrame->mPosition, sizeof( tVector4 ) );
        memcpy( &pAnimSequence->maRotation[i], &pFrame->mRotation, sizeof( tQuaternion ) );
        memcpy( &pAnimSequence->maScalings[i], &pFrame->mScaling, sizeof( tVector4 ) );
    }
    
    FREE( aAnimFrames );
}
#endif // #if 0

/*
**
*/
static void sortOnJoints( tAnimSequence* pAnimSequence )
{
    // copy animation frames
    int iNumFrames = pAnimSequence->miNumFrames;
    tAnimFrame* aAnimFrames = (tAnimFrame *)MALLOC( sizeof( tAnimFrame ) * iNumFrames );
    for( int i = 0; i < iNumFrames; i++ )
    {
        tAnimFrame* pFrame = &aAnimFrames[i];
        pFrame->mfTime = pAnimSequence->mafTime[i];
        memcpy( &pFrame->mPosition, &pAnimSequence->maPositions[i], sizeof( tVector4 ) );
        memcpy( &pFrame->mRotation, &pAnimSequence->maRotation[i], sizeof( tQuaternion ) );
        memcpy( &pFrame->mScaling, &pAnimSequence->maScalings[i], sizeof( tVector4 ) );
        memcpy( &pFrame->mRotationVec, &pAnimSequence->maRotationVec[i], sizeof( tVector4 ) );
        
        pFrame->mpJoint = pAnimSequence->mapJoints[i];
    }
    
    
    // sort by joints
    int iNumJoints = pAnimSequence->miNumJoints;
    int iInsert = 0;
    for( int iJoint = 0; iJoint < iNumJoints; iJoint++ )
    {
        tJoint const* pJoint = pAnimSequence->mapUniqueJoints[iJoint];
        tAnimFrame* pCurrFrame = &aAnimFrames[iInsert];
        
        for( int i = 0; i < iNumFrames; i++ )
        {
            tAnimFrame* pFrame = &aAnimFrames[i];
            if( pFrame->mpJoint == pJoint )
            {
                // swap
                tAnimFrame temp;
                memcpy( &temp, pCurrFrame, sizeof( tAnimFrame ) );
                memcpy( pCurrFrame, pFrame, sizeof( tAnimFrame ) );
                memcpy( pFrame, &temp, sizeof( tAnimFrame ) );
                
                // next anim frame slot
                ++iInsert;
                pCurrFrame = &aAnimFrames[iInsert];
            }
            
        }   // for i = 0 to num frames
        
    }   // for joint = 0 to num joints
    
    /*for( int i = 0; i < pAnimSequence->miNumJoints; i++ )
    {
        OUTPUT( "%d\t%s\n", i, pAnimSequence->mapUniqueJoints[i]->mszName );
    }
    
    for( int i = 0; i < iNumFrames; i++ )
    {
        OUTPUT( "%d\t%s\t%f\n", i , aAnimFrames[i].mpJoint->mszName, aAnimFrames[i].mfTime );
    }*/
    
#if 0
    // sort by joint address
    for( int i = 0; i < iNumFrames; i++ )
    {
        tAnimFrame* pCurrFrame = &aAnimFrames[i];
        
        for( int j = i + 1; j < iNumFrames; j++ )
        {
            tAnimFrame* pIterFrame = &aAnimFrames[j];
            if( (unsigned int)pCurrFrame->mpJoint > (unsigned int)pIterFrame->mpJoint )
            {
                // swap
                tAnimFrame temp;
                memcpy( &temp, pCurrFrame, sizeof( tAnimFrame ) );
                memcpy( pCurrFrame, pIterFrame, sizeof( tAnimFrame ) );
                memcpy( pIterFrame, &temp, sizeof( tAnimFrame ) );
                
                pCurrFrame = &aAnimFrames[i];
            }
            
        }   // for j = i to num frames
        
    }   // for i = 0 to num frames
#endif // #if 0
    
    //for( int i = 0; i < iNumFrames; i++ )
    //{
    //    OUTPUT( "%f\t%s\n", aAnimFrames[i].mfTime, aAnimFrames[i].mpJoint->mszName );
    //}
    
    // reset the arrays
    for( int i = 0; i < iNumFrames; i++ )
    {
        tAnimFrame* pFrame = &aAnimFrames[i];
        pAnimSequence->mafTime[i] = pFrame->mfTime;
        pAnimSequence->mapJoints[i] = pFrame->mpJoint;
        memcpy( &pAnimSequence->maPositions[i], &pFrame->mPosition, sizeof( tVector4 ) );
        memcpy( &pAnimSequence->maRotation[i], &pFrame->mRotation, sizeof( tQuaternion ) );
        memcpy( &pAnimSequence->maScalings[i], &pFrame->mScaling, sizeof( tVector4 ) );
        memcpy( &pAnimSequence->maRotationVec[i], &pFrame->mRotationVec, sizeof( tVector4 ) );
    }
    
    /*for( int i = 0; i < iNumFrames; i++ )
    {
        OUTPUT( "%d %.1f\trot( %f, %f, %f )\t%s\n",
                i,
                pAnimSequence->mafTime[i],
                pAnimSequence->maRotationVec[i].fX,
                pAnimSequence->maRotationVec[i].fY,
                pAnimSequence->maRotationVec[i].fZ,
                pAnimSequence->mapJoints[i]->mszName );
    }*/
    
    FREE( aAnimFrames );
}

/*
**
*/
static void sortOnTime( tAnimSequence* pAnimSequence )
{
    unsigned int iNumJoints = pAnimSequence->miNumJoints;
    for( unsigned int i = 0; i < iNumJoints; i++ )
    {
        int iStart = pAnimSequence->maiStartFrames[i];
        int iEnd = pAnimSequence->maiEndFrames[i];
        for( int j = iStart; j <= iEnd; j++ )
        {
            for( int k = j + 1; k <= iEnd; k++ )
            {
                if( pAnimSequence->mafTime[j] > pAnimSequence->mafTime[k] )
                {
                    tVector4 tempPos, tempScale, tempRotVec;
                    tQuaternion tempRot;
                    float fTemp = pAnimSequence->mafTime[j];
                    
                    memcpy( &tempPos, &pAnimSequence->maPositions[j], sizeof( tVector4 ) );
                    memcpy( &tempScale, &pAnimSequence->maScalings[j], sizeof( tVector4 ) );
                    memcpy( &tempRot, &pAnimSequence->maRotation[j], sizeof( tQuaternion ) );
                    memcpy( &tempRotVec, &pAnimSequence->maRotationVec[j], sizeof( tVector4 ) );
                    
                    pAnimSequence->mafTime[j] = pAnimSequence->mafTime[k];
                    memcpy( &pAnimSequence->maPositions[j], &pAnimSequence->maPositions[k], sizeof( tVector4 ) );
                    memcpy( &pAnimSequence->maScalings[j], &pAnimSequence->maScalings[k], sizeof( tVector4 ) );
                    memcpy( &pAnimSequence->maRotation[j], &pAnimSequence->maRotation[k], sizeof( tQuaternion ) );
                    memcpy( &pAnimSequence->maRotationVec[j], &pAnimSequence->maRotationVec[k], sizeof( tQuaternion ) );
                    
                    pAnimSequence->mafTime[k] = fTemp;
                    memcpy( &pAnimSequence->maPositions[k], &tempPos, sizeof( tVector4 ) );
                    memcpy( &pAnimSequence->maScalings[k], &tempScale, sizeof( tVector4 ) );
                    memcpy( &pAnimSequence->maRotation[k], &tempRot, sizeof( tQuaternion ) );
                    memcpy( &pAnimSequence->maRotationVec[k], &tempRotVec, sizeof( tVector4 ) );
                }
            }
            
        }   // for j = start to end
        
    }   // for i = 0 to num joints
    
    /*int iNumFrames = pAnimSequence->miNumFrames;
    for( int i = 0; i < iNumFrames; i++ )
    {
        OUTPUT( "%d %f\t%s\n", i, pAnimSequence->mafTime[i], pAnimSequence->mapJoints[i]->mszName );
    }*/
    
}