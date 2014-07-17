//
//  gamerender.cpp
//  animtest
//
//  Created by Tony Peng on 7/30/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "gamerender.h"
#include "factory.h"
#include "jobmanager.h"
#include "timeutil.h"

static CFactory<tGameRenderObject>      sGameRenderObjectFactory;
static CFactory<tAnimModelInstance>     sAnimModelInstanceFactory;
static CFactory<tVector4>               sVectorFactory;
static CFactory<tAnimSequence>          sAnimSequenceFactory;
static CFactory<tAnimHierarchy>         sAnimHierarchyFactory;

static tGameRenderObject* saRenderObjects;
static tAnimModelInstance* saAnimModelInstances;
static tVector4* saObjectInfo;
static tAnimSequence* spAnimSequence;
static tAnimHierarchy* saAnimHierarchies;

static int siNumRenderObjects;

CFactory<tVector4>      gVectorFactory;
CFactory<tQuaternion>   gQuaternionFactory;
CFactory<tMatrix44>     gMatrixFactory;
CFactory<tJoint>        gJointFactory;

tMatrix44 gIdentityMat;

static void updateJointValuesAndMatrices( void* pData );

/*
**
*/
void gamerRenderInitAllObjects( tAnimModel const* pAnimModel,
                                tAnimSequence* pAnimSequence,
                                tAnimHierarchy const* pAnimHierarchy )
{
    const float fStartingPosRadius = 5.0f;
    
    Matrix44Identity( &gIdentityMat );
    
    siNumRenderObjects = 1;
    
    saRenderObjects = sGameRenderObjectFactory.alloc( siNumRenderObjects );
    saAnimModelInstances = sAnimModelInstanceFactory.alloc( siNumRenderObjects );
    saObjectInfo = sVectorFactory.alloc( siNumRenderObjects * 3 );
    saAnimHierarchies = sAnimHierarchyFactory.alloc( siNumRenderObjects );
    
    memset( saObjectInfo, 0, sizeof( tVector4 ) * siNumRenderObjects * 3 );
    
    float fAnglePart = ( 2.0f * 3.14159f ) / (float)siNumRenderObjects;
    
    int iCount = 0;
    for( int i = 0; i < siNumRenderObjects; i++ )
    {
        // animation model instance
        tAnimModelInstance* pAnimModelInstance = &saAnimModelInstances[i];
        animModelInstanceInit( pAnimModelInstance );
        animModelInstanceSet( pAnimModelInstance, pAnimModel );
        animModelInstanceSetupGL( pAnimModelInstance );
        
        tGameRenderObject* pRenderObject = &saRenderObjects[i];
        pRenderObject->miType = RENDEROBJECT_ANIMMODEL;
        
        pRenderObject->mpAnimModelInstance = &saAnimModelInstances[i];
        
        // position, heading and speed
        pRenderObject->mpPosition = &saObjectInfo[iCount++];
        pRenderObject->mpSize = &saObjectInfo[iCount++];
        pRenderObject->mpHeading = &saObjectInfo[iCount++];
        pRenderObject->mfSpeed = 0.0f;
    
        pRenderObject->mpPosition->fW = 1.0f;
        pRenderObject->mpSize->fW = 1.0f;
        pRenderObject->mpHeading->fW = 1.0f;
        
        // heading
        tVector4 xVec = { 1.0f, 0.0f, 0.0f, 1.0f };
        tMatrix44 rotMatY;
        Matrix44RotateY( &rotMatY, fAnglePart * (float)i );
        Matrix44Transform( pRenderObject->mpHeading, &xVec, &rotMatY );
        Vector4Normalize( pRenderObject->mpHeading, pRenderObject->mpHeading );
        
        // position
        pRenderObject->mpPosition->fX = pRenderObject->mpHeading->fX * fStartingPosRadius - 15.0f;
        pRenderObject->mpPosition->fZ = pRenderObject->mpHeading->fZ * fStartingPosRadius;
        
        pRenderObject->mfAnimMult = 0.5f + (float)( rand() % 500 ) * 0.001f;
        
        pRenderObject->mfSpeed = pRenderObject->mfAnimMult;
        pRenderObject->mfSpeed *= 0.01f;
        
        pRenderObject->mpSize->fX = 1.5f;
        pRenderObject->mpSize->fY = 1.5f;
        pRenderObject->mpSize->fZ = 1.5f;
        
        // duplicate of the hierarchy for storing skin matrices
        animHierarchyCopy( &saAnimHierarchies[i],
                           pAnimHierarchy,
                           &gJointFactory,
                           &gMatrixFactory );
        
        // use this hierarchy for this model instance
        pAnimModelInstance->mpAnimHierarchy = &saAnimHierarchies[i];
        pAnimModelInstance->mpAnimModel = pAnimModel;
        
        pAnimModelInstance->maRotations = (tVector4 *)malloc( sizeof( tVector4 ) );
        pAnimModelInstance->maPositions = (tVector4 *)malloc( sizeof( tVector4 ) );
        pAnimModelInstance->maScalings = (tVector4 *)malloc( sizeof( tVector4 ) );
        pAnimModelInstance->maRotatePivot = (tVector4 *)malloc( sizeof( tVector4 ) );
        pAnimModelInstance->maScalePivot = (tVector4 *)malloc( sizeof( tVector4 ) );
        
        //memcpy( pAnimModelInstance->maRotatePivot, &pAnimModel->mCenter, sizeof( tVector4 ) );
        //memcpy( pAnimModelInstance->maScalePivot, &pAnimModel->mCenter, sizeof( tVector4 ) );
        
        memset( pAnimModelInstance->maRotatePivot, 0, sizeof( tVector4 ) );
        memset( pAnimModelInstance->maScalePivot, 0, sizeof( tVector4 ) );
        
        pAnimModelInstance->miNumXForms = 1;
    }

    spAnimSequence = pAnimSequence;
}

/*
**
*/
void gameRenderReleaseAllObjects( void )
{
   
}

/*
**
*/
void gameRenderAnimModelJob( tAnimSequence const* pAnimSequence,
                             tAnimHierarchy const* pAnimHierarchy,
                             tAnimHierarchy* pResultHierarchy,
                             tVector4* aScalings,
                             tVector4* aPositions,
                             tQuaternion* aRotations,
                             tVector4* aRotationVecs,
                             tMatrix44* aAnimMatrices,
                             float fTime )
{
    tJob job;
    tJointJobData jobData;
    jobData.mpSequence = pAnimSequence;
    jobData.mpHierarchy = pAnimHierarchy;
    jobData.maPositions = aPositions;
    jobData.maScalings = aScalings;
    jobData.maRotations = aRotations;
    jobData.maRotationVecs = aRotationVecs;
    jobData.mfTime = fTime;
    jobData.maAnimMatrices = aAnimMatrices;
    
    jobData.mpResultAnimHiearchy = pResultHierarchy;
    
    job.mpfnFunc = updateJointValuesAndMatrices;
    job.miDataSize = sizeof( tJointJobData );
    job.mpData = &jobData;
    
    jobManagerAddJob( gpJobManager, &job );
}

/*
**
*/
void gamerRenderUpdateAllObjects( float fTime )
{
    double fElapsed4 = 0.0f;
    double fElapsed6 = 0.0f;
    
    double fElapsed7 = 0.0f;
    double fElapsed8 = 0.0f;
    double fElapsed9 = 0.0f;
    double fElapsed10 = 0.0f;
    
    double fStart, fEnd;
    
    double fPartStart, fPartEnd;
    
    int iNumJoints = saAnimHierarchies[0].miNumJoints;
    
    /*tVector4* aPositions = (tVector4 *)malloc( sizeof( tVector4 ) * iNumJoints );
    tVector4* aScalings = (tVector4 *)malloc( sizeof( tVector4 ) * iNumJoints );
    tQuaternion* aRotations = (tQuaternion *)malloc( sizeof( tQuaternion ) * iNumJoints );
    tVector4* aRotationVecs = (tVector4 *)malloc( sizeof( tVector4 ) * iNumJoints );
    tMatrix44* aAnimMatrices = (tMatrix44 *)malloc( sizeof( tMatrix44 ) * iNumJoints );
    */
    
    // allocate for all the objects and their joints
    tVector4** aaPositions = (tVector4 **)malloc( sizeof( tVector4* ) * siNumRenderObjects );
    tVector4** aaScalings = (tVector4 **)malloc( sizeof( tVector4* ) * siNumRenderObjects );
    tQuaternion** aaRotations = (tQuaternion **)malloc( sizeof( tQuaternion* ) * siNumRenderObjects );
    tVector4** aaRotationVecs = (tVector4 **)malloc( sizeof( tVector4* ) * siNumRenderObjects );
    tMatrix44** aaAnimMatrices = (tMatrix44 **)malloc( sizeof( tMatrix44* ) * siNumRenderObjects );
    
    for( int i = 0; i < siNumRenderObjects; i++ )
    {
        aaPositions[i] = (tVector4 *)malloc( sizeof( tVector4 ) * iNumJoints );
        aaScalings[i] = (tVector4 *)malloc( sizeof( tVector4 ) * iNumJoints );
        aaRotations[i] = (tQuaternion *)malloc( sizeof( tQuaternion ) * iNumJoints );
        aaRotationVecs[i] = (tVector4 *)malloc( sizeof( tVector4 ) * iNumJoints );
        aaAnimMatrices[i] = (tMatrix44 *)malloc( sizeof( tMatrix44 ) * iNumJoints );
    
        for( int j = 0; j < iNumJoints; j++ )
        {
            Matrix44Identity( &aaAnimMatrices[i][j] );
        }
    }
    
    /*for( int i = 0; i < iNumJoints; i++ )
    {
        Matrix44Identity( &aAnimMatrices[i] );
    }*/
    
    fPartStart = getTime();
    
#if 0
    // update anim model instances' transform
    for( int i = 0; i < siNumRenderObjects; i++ )
    {
        fStart = getTime();
        
        //animSequencePlay( spAnimSequence, aPositions, aScalings, aRotations, aRotationVecs, aAnimMatrices, sfTime );
        
        tGameRenderObject* pGameRenderObject = &saRenderObjects[i];
        tAnimModelInstance* pAnimModelInstance = pGameRenderObject->mpAnimModelInstance;
        
        // y rotation angle
        float fAngle = atan2f( pGameRenderObject->mpHeading->fZ, pGameRenderObject->mpHeading->fX ) - 3.14159f * 0.5f;
        pAnimModelInstance->mRotation.fY = fAngle;
        
        // update position
        tVector4 dPos;
        Vector4MultScalar( &dPos, pGameRenderObject->mpHeading, pGameRenderObject->mfSpeed );
        
        tVector4* pPos = pGameRenderObject->mpPosition;
        tVector4 newPos;
        Vector4Add( &newPos, pPos, &dPos );
        
        memcpy( pGameRenderObject->mpPosition, &newPos, sizeof( tVector4 ) );
        
        memcpy( &pAnimModelInstance->mPosition, &newPos, sizeof( tVector4 ) );
        memcpy( &pAnimModelInstance->mScaling, pGameRenderObject->mpSize, sizeof( tVector4 ) );
    
        fEnd = getTime();
        fElapsed0 += ( fEnd - fStart );
    }
#endif // #if 0
    
    fPartEnd = getTime();
    fElapsed7 = fPartEnd - fPartEnd;
    fPartStart = fPartEnd;
    
    tVector4** paPositions = aaPositions;
    tVector4** paScalings = aaScalings;
    tVector4** paRotationVecs = aaRotationVecs;
    tQuaternion** paRotations = aaRotations;
    tMatrix44** paAnimMatrices = aaAnimMatrices;
    
    // update joint in hierarchy matrices
    for( int i = 0; i < siNumRenderObjects; i++ )
    {
        tJob job;
        tJointJobData jobData;
        jobData.mpSequence = spAnimSequence;
        jobData.mpHierarchy = &saAnimHierarchies[0];
        jobData.maPositions = *paPositions; //aPositions;
        jobData.maScalings = *paScalings; //aScalings;
        jobData.maRotations = *paRotations; //aRotations;
        jobData.maRotationVecs = *paRotationVecs; //aRotationVecs;
        jobData.mfTime = fTime * saRenderObjects[i].mfAnimMult;
        jobData.maAnimMatrices = *paAnimMatrices; //aAnimMatrices;
        
        jobData.mpResultAnimHiearchy = &saAnimHierarchies[i];
        
        job.mpfnFunc = updateJointValuesAndMatrices;
        job.miDataSize = sizeof( tJointJobData );
        job.mpData = &jobData;
        
        jobManagerAddJob( gpJobManager, &job );
        
        ++paPositions;
        ++paScalings;
        ++paRotations;
        ++paRotationVecs;
        ++paAnimMatrices;
        
    }   // for i = 0 to num render objects
    
    jobManagerWait( gpJobManager );
    
    
    // update all the joint's matrices
    /*for( int i = 0; i < siNumRenderObjects; i++ )
    {
        fStart = getTime();
        animSequenceUpdateJointAnimValues( spAnimSequence,
                                          &saAnimHierarchies[0],
                                          aaPositions[i],
                                          aaScalings[i],
                                          aaRotations[i],
                                          aaRotationVecs[i],
                                          fTime * saRenderObjects[i].mfAnimMult );
        
        jobManagerWait( gpJobManager );
        
        fEnd = getTime();
        fElapsed1 += ( fEnd - fStart );
        fStart = fEnd;
        
        animSequenceUpdateAnimMatrices( spAnimSequence,
                                       &saAnimHierarchies[0],
                                       aaPositions[i],
                                       aaScalings[i],
                                       aaRotations[i],
                                       aaRotationVecs[i],
                                       aaAnimMatrices[i] );
        
        jobManagerWait( gpJobManager );
        
        fElapsed2 += ( fEnd - fStart );
        fStart = fEnd;
        
        updateJointMatrices( &saAnimHierarchies[i], aaAnimMatrices[i] );
        jobManagerWait( gpJobManager );
        
        fElapsed3 += ( fEnd - fStart );
        fStart = fEnd;
    }
    
    jobManagerWait( gpJobManager );
    */
    
    fPartEnd = getTime();
    fElapsed8 = fPartEnd - fPartStart;
    
    // update the xform of the total model
    for( int i = 0; i < siNumRenderObjects; i++ )
    {
        animModelInstanceUpdateXForm( &saAnimModelInstances[i] );
    }
    
    // transform position and normal
    fPartStart = getTime();
    
    fStart = getTime();
    for( int i = 0; i < siNumRenderObjects; i++ )
    {
        animModelInstanceUpdate( &saAnimModelInstances[i] );
    }
    
    jobManagerWait( gpJobManager );
    
    fEnd = getTime();
    fElapsed4 += ( fEnd - fStart );
    fStart = fEnd;
    
    fPartEnd = getTime();
    fElapsed9 = fPartEnd - fPartStart;
    
    WTFASSERT2( fPartEnd >= fPartStart, "WTF" );
    
    fStart = getTime();
    
    // TODO: OPTIMIZE COPY VB DATA
    for( int i = 0; i < siNumRenderObjects; i++ )
    {
        animModelInstanceCopyVBData( &saAnimModelInstances[i] );
    }
    
    fEnd = getTime();
    fElapsed6 = fEnd - fStart;
    
    fPartEnd = getTime();
    fElapsed10 = fPartEnd - fPartStart;
    fPartStart = fPartEnd;
    
#if 0
    OUTPUT( "********************\n" );
    OUTPUT( "elapsed 0 = %.4f\n", fElapsed0 );
    OUTPUT( "elapsed 1 = %.4f\n", fElapsed1 );
    OUTPUT( "elapsed 2 = %.4f\n", fElapsed2 );
    OUTPUT( "elapsed 3 = %.4f\n", fElapsed3 );
    OUTPUT( "elapsed 4 = %.4f\n", fElapsed4 );
    OUTPUT( "elapsed 5 = %.4f\n", fElapsed5 );
    OUTPUT( "elapsed 6 = %.4f\n", fElapsed6 );
    OUTPUT( "elapsed 7 = %.4f\n", fElapsed7 );
    OUTPUT( "elapsed 8 = %.4f\n", fElapsed8 );
    OUTPUT( "elapsed 9 = %.4f\n", fElapsed9 );
    OUTPUT( "elapsed 10 = %.4f\n", fElapsed10 );
    OUTPUT( "total elapsed = %.4f\n", fTotalElapsed );
    OUTPUT( "********************\n" );
#endif // #if 0
    
    /*free( aPositions );
    free( aScalings );
    free( aRotations );
    free( aRotationVecs );
    free( aAnimMatrices );*/
    
    for( int i = 0; i < siNumRenderObjects; i++ )
    {
        free( aaPositions[i] );
        free( aaScalings[i] );
        free( aaRotations[i] );
        free( aaRotationVecs[i] );
        free( aaAnimMatrices[i] );
    }
    
    free( aaPositions );
    free( aaScalings );
    free( aaRotations );
    free( aaRotationVecs );
    free( aaAnimMatrices );
}

/*
**
*/
void gameRenderDraw( tMatrix44 const* pViewMatrix,
                     tMatrix44 const* pProjMatrix,
                     int iShader )
{
    for( int i = 0; i < siNumRenderObjects; i++ )
    {
        animModelInstanceDraw( &saAnimModelInstances[i],
                               pViewMatrix,
                               pProjMatrix,
                               iShader );
    }
}

/*
**
*/
void gameRenderSetXForm( int iObject,
                         tVector4 const* pPosition,
                         tVector4 const* pScaling,
                         tVector4 const* pRotation )
{
    tGameRenderObject* pGameRenderObject = &saRenderObjects[iObject];
    tAnimModelInstance* pAnimModelInstance = pGameRenderObject->mpAnimModelInstance;
    
    memcpy( pGameRenderObject->mpPosition, pPosition, sizeof( tVector4 ) );
    memcpy( pGameRenderObject->mpSize, pScaling, sizeof( tVector4 ) );
    memcpy( pGameRenderObject->mpHeading, pRotation, sizeof( tVector4 ) );
    
    memcpy( &pAnimModelInstance->maPositions[0], pGameRenderObject->mpPosition, sizeof( tVector4 ) );
    memcpy( &pAnimModelInstance->maScalings[0], pGameRenderObject->mpSize, sizeof( tVector4 ) );
    memcpy( &pAnimModelInstance->maRotations[0], pGameRenderObject->mpHeading, sizeof( tVector4 ) );
}

/*
**
*/
tVector4 const* gameRenderGetObjectPosition( int iObject )
{
    return saRenderObjects[iObject].mpPosition;
}

/*
**
*/
static void updateJointMatricesNoJob( tAnimHierarchy* pAnimHierarchy,
                                      tMatrix44* aAnimMatrices )
{
    // update joint matrices in hierarchy
    // use worker threads
    int iJoint = 0;
    int iLevel = 0;
    for( ;; )
    {
        if( iJoint >= pAnimHierarchy->miNumJoints )
        {
            break;
        }
        
        tJoint* pJoint = &pAnimHierarchy->maJoints[iJoint];
        if( pJoint->miLevel > iLevel )
        {
            // update next level
            ++iLevel;
        }
        
        int iJointIndex = 0;
        int iNumHierarchyJoints = pAnimHierarchy->miNumJoints;
        for( iJointIndex = 0; iJointIndex < iNumHierarchyJoints; iJointIndex++ )
        {
            if( !strcmp( pAnimHierarchy->maJoints[iJointIndex].mszName, pJoint->mszName ) )
            {
                break;
            }
        }
        
        WTFASSERT2( iJointIndex < iNumHierarchyJoints, "can't find joint in hierarchy" );
        
        // transform
        tMatrix44 const* pParentMatrix = &gIdentityMat;
        if( pJoint->mpParent )
        {
            pParentMatrix = pJoint->mpParent->mpTotalMatrix;
        }
        
        // M(total) = M(parent) * M(local) * M(anim)
        tMatrix44 parentLocalMatrix;
        tMatrix44* pTotalMatrix = pJoint->mpTotalMatrix;
        tMatrix44* pSkinMatrix = pJoint->mpSkinMatrix;
        
        Matrix44Multiply( &parentLocalMatrix, pParentMatrix, pJoint->mpLocalMatrix );
        //Matrix44Multiply( pTotalMatrix, &parentLocalMatrix, pJoint->mpAnimMatrix );
        Matrix44Multiply( pTotalMatrix, &parentLocalMatrix, &aAnimMatrices[iJointIndex]);
        
        // M(skin) = M(total) * M(invPose)
        Matrix44Multiply( pJoint->mpSkinMatrix, pTotalMatrix, pJoint->mpInvPoseMatrix );
        
        // M(invTransposeSkin)
        tMatrix44 invSkinMatrix;
        Matrix44Inverse( &invSkinMatrix, pSkinMatrix );
        Matrix44Transpose( pJoint->mpInvTransSkinningMatrix, &invSkinMatrix );
        
        ++iJoint;
    }
}

/*
**
*/
static void updateJointValuesAndMatrices( void* pData )
{
    tJointJobData* pJointJobData = (tJointJobData *)pData;
    
    tAnimSequence const* pAnimSequence = pJointJobData->mpSequence;
    tAnimHierarchy const* pAnimHierarchy = pJointJobData->mpHierarchy;
    tAnimHierarchy* pResultAnimHierarchy = pJointJobData->mpResultAnimHiearchy;
    tVector4* aPositions = pJointJobData->maPositions;
    tVector4* aScalings = pJointJobData->maScalings;
    tQuaternion* aRotations = pJointJobData->maRotations;
    tVector4* aRotationVecs = pJointJobData->maRotationVecs;
    tMatrix44* aAnimMatrices = pJointJobData->maAnimMatrices;
    
    animSequenceUpdateJointAnimValues( pAnimSequence,
                                       pAnimHierarchy,
                                       aPositions,
                                       aScalings,
                                       aRotations,
                                       aRotationVecs,
                                       pJointJobData->mfTime );
    
    animSequenceUpdateAnimMatrices( pAnimSequence,
                                    pAnimHierarchy,
                                    aPositions,
                                    aScalings,
                                    aRotations,
                                    aRotationVecs,
                                    aAnimMatrices );
    
    updateJointMatricesNoJob( pResultAnimHierarchy, aAnimMatrices );
}