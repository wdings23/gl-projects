//
//  animmodelinstance.cpp
//  animtest
//
//  Created by Tony Peng on 7/23/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "animmodelinstance.h"
#include "jobmanager.h"

struct XFormVertJobData
{
    tVector4*               mpXFormPos;
    tVector4 const*         mpPos;
    
    tVector4*               mpXFormNorm;
    tVector4 const*         mpNorm;
    
    float                   mfPct0;
    float                   mfPct1;
    
    tJoint*                 mpJoint0;
    tJoint*                 mpJoint1;
};

typedef struct XFormVertJobData tXFormVertJobData;

struct InstanceJobData
{
    tAnimModelInstance*     mpAnimModelInstance;
};

typedef struct InstanceJobData tInstanceJobData;

static void xformAllVertsAndNorms( void* pData );

/*
**
*/
void animModelInstanceInit( tAnimModelInstance* pAnimModelInstance )
{
    memset( pAnimModelInstance, 0, sizeof( tAnimModelInstance ) );
}

/*
**
*/
void animModelInstanceRelease( tAnimModelInstance* pAnimModelInstance )
{
    free( pAnimModelInstance->maiVBOIndices );
    free( pAnimModelInstance->maUV );
    free( pAnimModelInstance->maVBOVerts );
    free( pAnimModelInstance->maXFormNorm );
    free( pAnimModelInstance->maXFormPos );
    free( pAnimModelInstance->maAnimMatrices );
    free( pAnimModelInstance->maXFormMaps );
    
    free( pAnimModelInstance->maPositions );
    free( pAnimModelInstance->maRotations );
    free( pAnimModelInstance->maScalings );
    free( pAnimModelInstance->maScalePivot );
    free( pAnimModelInstance->maRotatePivot );
}

/*
**
*/
void animModelInstanceSet( tAnimModelInstance* pAnimModelInstance, tAnimModel const* pAnimModel )
{
    pAnimModelInstance->mpAnimModel = pAnimModel;
    
    pAnimModelInstance->miNumNormals = pAnimModel->miNumNormals;
    pAnimModelInstance->miNumPos = pAnimModel->miNumVerts;
    pAnimModelInstance->miNumUV = pAnimModel->miNumUV;
    pAnimModelInstance->miNumVBOIndices = pAnimModel->miNumFaces * 3;
    pAnimModelInstance->miNumVBOVerts = pAnimModel->miNumVBOVerts;
    
    pAnimModelInstance->maVBOVerts = (tInterleaveVert *)malloc( sizeof( tInterleaveVert ) * pAnimModelInstance->miNumVBOVerts );
    pAnimModelInstance->maiVBOIndices = (unsigned int *)malloc( sizeof( int ) * pAnimModelInstance->miNumVBOIndices );
    pAnimModelInstance->maXFormNorm = (tVector4 *)malloc( sizeof( tVector4 ) * pAnimModelInstance->miNumNormals );
    pAnimModelInstance->maXFormPos = (tVector4 *)malloc( sizeof( tVector4 ) * pAnimModelInstance->miNumPos );
    pAnimModelInstance->maUV = (tVector2 *)malloc( sizeof( tVector2 ) * pAnimModelInstance->miNumUV );
    
    pAnimModelInstance->maXFormMaps = (tXFormMap *)malloc( sizeof( tXFormMap ) * pAnimModelInstance->miNumVBOVerts );
    
    pAnimModelInstance->maAnimMatrices = (tMatrix44 *)malloc( sizeof( tMatrix44 ) * pAnimModel->mpAnimHierarchy->miNumJoints );
    
    memcpy( pAnimModelInstance->maiVBOIndices, pAnimModel->maiVBOIndices, sizeof( int ) * pAnimModelInstance->miNumVBOIndices );
    memcpy( pAnimModelInstance->maVBOVerts, pAnimModel->maVBOVerts, sizeof( tInterleaveVert ) * pAnimModelInstance->miNumVBOVerts );
    
    // set the correct ptr to xform pos and norm
    int iNumVBOVerts = pAnimModel->miNumVBOVerts;
    for( int i = 0; i < iNumVBOVerts; i++ )
    {
        tXFormMap* pPtr = &pAnimModelInstance->maXFormMaps[i];
        
        // indices in the list
        pPtr->miXFormPos = pAnimModel->maVBOVertPtrs[i].miPos;
        pPtr->miXFormNorm = pAnimModel->maVBOVertPtrs[i].miNorm;
        pPtr->miUV = pAnimModel->maVBOVertPtrs[i].miUV;
        
    }   // for i = 0 to num vbo verts
}

/*
**
*/
void animModelInstanceUpdate( tAnimModelInstance* pAnimModelInstance )
{
    tAnimModel const* pAnimModel = pAnimModelInstance->mpAnimModel;
    WTFASSERT2( pAnimModel, "invalid animation model" );
    
    // transform all the position and normals in jobs
    tJob job;
    tInstanceJobData jobData = { pAnimModelInstance };
    job.mpData = &jobData;
    job.miDataSize = sizeof( tInstanceJobData );
    job.mpfnFunc = &xformAllVertsAndNorms;
    
    jobManagerAddJob( gpJobManager, &job );
    
#if 0
    int iNumVerts = pAnimModel->miNumVerts;
    int iNumNorms = pAnimModel->miNumNormals;
    
    tVector4* pXFormPos = pAnimModelInstance->maXFormPos;
    tVector4* pXFormNorm = pAnimModelInstance->maXFormNorm;
    
    tVector4* pPos = pAnimModel->maPos;
    tVector4* pNorm = pAnimModel->maNorm;
    
    int* aiJointIndices = pAnimModel->maiJointIndices;
    float* afBlendVals = pAnimModel->mafBlendVals;
    
    tAnimHierarchy const* pAnimHierarchy = pAnimModelInstance->mpAnimHierarchy;
    tJoint* aJoints = pAnimHierarchy->maJoints;
    
    //OUTPUT( "*** REGULAR ***\n" );
    // transform position
    for( int i = 0; i < iNumVerts; i++ )
    {
        tJoint* pJoint0 = &aJoints[*aiJointIndices++];
        tJoint* pJoint1 = &aJoints[*aiJointIndices++];
        
        tVector4 xform0, xform1;
        Matrix44Transform( &xform0, pPos, pJoint0->mpSkinMatrix );
        Matrix44Transform( &xform1, pPos, pJoint1->mpSkinMatrix );
        
        Vector4MultScalar( &xform0, &xform0, *afBlendVals++ );
        Vector4MultScalar( &xform1, &xform1, *afBlendVals++ );
        
        Vector4Add( pXFormPos, &xform0, &xform1 );
        
        /*OUTPUT( "xform ( %f, %f, %f ) orig ( %f, %f, %f )\n",
               pXFormPos->fX,
               pXFormPos->fY,
               pXFormPos->fZ,
               pPos->fX,
               pPos->fY,
               pPos->fZ );*/
        
        ++pPos;
        ++pXFormPos;
        
    }   // for i = 0 to num vertices

    aiJointIndices = pAnimModel->maiNormalJointIndices;
    afBlendVals = pAnimModel->mafNormalBlendVals;
    for( int i = 0; i < iNumNorms; i++ )
    {
        tJoint* pJoint0 = &aJoints[*aiJointIndices++];
        tJoint* pJoint1 = &aJoints[*aiJointIndices++];
        
        tVector4 xform0, xform1;
        Matrix44Transform( &xform0, pNorm, pJoint0->mpInvTransSkinningMatrix );
        Matrix44Transform( &xform1, pNorm, pJoint1->mpInvTransSkinningMatrix );
        
        Vector4MultScalar( &xform0, &xform0, *afBlendVals++ );
        Vector4MultScalar( &xform1, &xform1, *afBlendVals++ );
        
        Vector4Add( pXFormNorm, &xform0, &xform1 );
        
        ++pNorm;
        ++pXFormNorm;
        
    }   // for i = 0 to num normals

#endif // #if 0
  
#if 0
    tJob job;
    tXFormVertJobData jobData;
    job.mpData = &jobData;
    job.miDataSize = sizeof( tXFormVertJobData );
    job.mpfnFunc = &xformVert;
    
    for( int i = 0; i < iNumVerts; i++ )
    {
        jobData.mpJoint0 = &aJoints[*aiJointIndices++];
        jobData.mpJoint1 = &aJoints[*aiJointIndices++];
        
        jobData.mfPct0 = *afBlendVals++;
        jobData.mfPct1 = *afBlendVals++;
        
        jobData.mpPos = pPos;
        jobData.mpXFormPos = pXFormPos;
        
        jobManagerAddJob( gpJobManager, &job );
        
        ++pPos;
        ++pXFormPos;
    }
    
    aiJointIndices = pAnimModel->maiNormalJointIndices;
    afBlendVals = pAnimModel->mafBlendVals;
    
    job.mpfnFunc = &xformNorm;
    for( int i = 0; i < iNumNorms; i++ )
    {
        WTFASSERT2( *aiJointIndices >= 0 && *aiJointIndices < pAnimHierarchy->miNumJoints, "invalid joint for this noraml" );
        jobData.mpJoint0 = &aJoints[*aiJointIndices++];
        
        WTFASSERT2( *aiJointIndices >= 0 && *aiJointIndices < pAnimHierarchy->miNumJoints, "invalid joint for this noraml" );
        jobData.mpJoint1 = &aJoints[*aiJointIndices++];
    
        jobData.mfPct0 = *afBlendVals++;
        jobData.mfPct1 = *afBlendVals++;
        
        jobData.mpNorm = pNorm;
        jobData.mpXFormNorm = pXFormNorm;
        
        jobManagerAddJob( gpJobManager, &job );
        
        ++pNorm;
        ++pXFormNorm;
        
    }
#endif // #if 0
    
#if 0
    // transform for this model
    // TODO: move this to job manager
    tMatrix44 rotMat, scaleMat, posMat, posRotMat;
    
    tQuaternion xRot, yRot, zRot, xyRot, totalRot;
    quaternionFromAxisAngle( &xRot, &gXAxis, pAnimModelInstance->mRotation.fX );
    quaternionFromAxisAngle( &yRot, &gYAxis, pAnimModelInstance->mRotation.fY );
    quaternionFromAxisAngle( &zRot, &gZAxis, pAnimModelInstance->mRotation.fZ );
    
    quaternionMultiply( &xyRot, &xRot, &yRot );
    quaternionMultiply( &totalRot, &xyRot, &zRot );
    
    quaternionToMatrix( &rotMat, &totalRot );
    
    Matrix44Scale( &scaleMat, pAnimModelInstance->mScaling.fX, pAnimModelInstance->mScaling.fY, pAnimModelInstance->mScaling.fZ );
    Matrix44Translate( &posMat, &pAnimModelInstance->mPosition );
    
    Matrix44Multiply( &posRotMat, &posMat, &rotMat );
    Matrix44Multiply( &pAnimModelInstance->mXFormMat, &posRotMat, &scaleMat );
#endif // #if 0
    
//#endif // #if 0
}

/*
**
*/
void animModelInstanceCopyVBData( tAnimModelInstance* pAnimModelInstance )
{
    int iNumVBOVerts = pAnimModelInstance->miNumVBOVerts;
    
    tXFormMap* aXFormMap = pAnimModelInstance->maXFormMaps;
    tInterleaveVert* aV = pAnimModelInstance->maVBOVerts;
    
    tVector4* aXFormPos = pAnimModelInstance->maXFormPos;
    tVector4* aXFormNorm = pAnimModelInstance->maXFormNorm;
    
    // copy the xformed position and normal over
    for( int i = 0; i < iNumVBOVerts; i++ )
    {
        tXFormMap* pXFormMap = &aXFormMap[i];
        tInterleaveVert* pVert = aV;
        
        memcpy( &pVert->mPos, &aXFormPos[pXFormMap->miXFormPos], sizeof( tVector4 ) );
        memcpy( &pVert->mNorm, &aXFormNorm[pXFormMap->miXFormNorm], sizeof( tVector4 ) );
    
        ++aV;
        
    }   // for i = 0 to num vbo verts

}

/*
**
*/
void animModelInstanceSetAnimSequence( tAnimModelInstance* pAnimModelInstance, tAnimSequence const* pSequence )
{
    pAnimModelInstance->mpAnimSequence = pSequence;
    pAnimModelInstance->mfAnimTime = 0.0f;
}

/*
**
*/
void animModelInstanceSetupGL( tAnimModelInstance* pAnimModelInstance )
{
    // data array
    glGenBuffers( 1, &pAnimModelInstance->miVBO );
    glBindBuffer( GL_ARRAY_BUFFER, pAnimModelInstance->miVBO );
    glBufferData( GL_ARRAY_BUFFER, sizeof( tInterleaveVert ) * pAnimModelInstance->miNumVBOVerts, pAnimModelInstance->maVBOVerts, GL_DYNAMIC_DRAW );
    
    // index array
    glGenBuffers( 1, &pAnimModelInstance->miIBO );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pAnimModelInstance->miIBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( int ) * pAnimModelInstance->miNumVBOIndices, pAnimModelInstance->maiVBOIndices, GL_DYNAMIC_DRAW );
    
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

/*
**
*/
void animModelInstanceDraw( tAnimModelInstance* pAnimModelInstance,
                            tMatrix44 const* pViewMatrix,
                            tMatrix44 const* pProjMatrix,
                            GLint iShader )
{
    glUseProgram( iShader );
    
    GLint iViewProj = glGetUniformLocation( iShader, "viewProjMat" );
    WTFASSERT2( iViewProj >= 0, "invalid view proj matrix" );
    
    GLint iNormMat = glGetUniformLocation( iShader, "normMat" );
    WTFASSERT2( iNormMat >= 0, "invalid normal matrix" );
    
    tMatrix44 normalMat;
    Matrix44Transpose( &normalMat, &pAnimModelInstance->mRotMat );
    
#if 0
    tMatrix44 xformMatrix, transpose, viewProjMatrix;
    
#endif // #if 0
    tMatrix44 viewProjMatrix, transpose;
    

    if( pAnimModelInstance->mbPreXform )
    {
        Matrix44Transpose( &transpose, pViewMatrix );
    }
    else
    {
        tMatrix44 xformMatrix;
        Matrix44Multiply( &xformMatrix, pViewMatrix, &pAnimModelInstance->mXFormMat );
        Matrix44Transpose( &transpose, &xformMatrix );
    }
    
    Matrix44Multiply( &viewProjMatrix, &transpose, pProjMatrix );
    
    glUniformMatrix4fv( iViewProj, 1, GL_FALSE, viewProjMatrix.afEntries );
    glUniformMatrix4fv( iNormMat, 1, GL_FALSE, normalMat.afEntries );
    
    GLint iPos = glGetAttribLocation( iShader, "position" );
    GLint iNorm = glGetAttribLocation( iShader, "normal" );
    
    WTFASSERT2( iPos >= 0, "invalid position shader semantic" );
    WTFASSERT2( iNorm >= 0, "invalid normal shader semantic" );

    glBindBuffer( GL_ARRAY_BUFFER, pAnimModelInstance->miVBO );
    
    glBufferData( GL_ARRAY_BUFFER, sizeof( tInterleaveVert ) * pAnimModelInstance->miNumVBOVerts, pAnimModelInstance->maVBOVerts, GL_DYNAMIC_DRAW );
    
    glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, sizeof( tInterleaveVert ), (void *)0 );
    glEnableVertexAttribArray( iPos );
    
    glVertexAttribPointer( iNorm, 4, GL_FLOAT, GL_FALSE, sizeof( tInterleaveVert ), (void *)( sizeof( tVector4 ) + sizeof( tVector2 ) ) );
    glEnableVertexAttribArray( iNorm );
    
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pAnimModelInstance->miIBO );
    glDrawElements( GL_TRIANGLES, pAnimModelInstance->miNumVBOIndices, GL_UNSIGNED_INT, (void *)0 );
     
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

/*
**
*/
static void xformAllVertsAndNorms( void* pData )
{
    tAnimModelInstance* pAnimModelInstance = ( (tInstanceJobData *)pData )->mpAnimModelInstance;
    
    tAnimModel const* pAnimModel = pAnimModelInstance->mpAnimModel;
    WTFASSERT2( pAnimModel, "invalid animation model" );
    
    int iNumVerts = pAnimModel->miNumVerts;
    int iNumNorms = pAnimModel->miNumNormals;
    
    tVector4* pXFormPos = pAnimModelInstance->maXFormPos;
    tVector4* pXFormNorm = pAnimModelInstance->maXFormNorm;
    
    tVector4* pPos = pAnimModel->maPos;
    tVector4* pNorm = pAnimModel->maNorm;
    
    int* aiJointIndices = pAnimModel->maiJointIndices;
    float* afBlendVals = pAnimModel->mafBlendVals;
    
    tAnimHierarchy const* pAnimHierarchy = pAnimModelInstance->mpAnimHierarchy;
    tJoint* aJoints = pAnimHierarchy->maJoints;
    
    // transform position
    for( int i = 0; i < iNumVerts; i++ )
    {
        tJoint* pJoint0 = &aJoints[*aiJointIndices++];
        tJoint* pJoint1 = &aJoints[*aiJointIndices++];
        
        tVector4 xform;
        Matrix44Transform( &xform, pPos, &pAnimModelInstance->mXFormMat );
        
        tVector4 xform0, xform1;
        
        if( pAnimModelInstance->mbPreXform )
        {
            Matrix44Transform( &xform0, &xform, pJoint0->mpSkinMatrix );
            Matrix44Transform( &xform1, &xform, pJoint1->mpSkinMatrix );
        }
        else
        {
            Matrix44Transform( &xform0, pPos, pJoint0->mpSkinMatrix );
            Matrix44Transform( &xform1, pPos, pJoint1->mpSkinMatrix );
        }
        
        Vector4MultScalar( &xform0, &xform0, *afBlendVals++ );
        Vector4MultScalar( &xform1, &xform1, *afBlendVals++ );
        
        Vector4Add( pXFormPos, &xform0, &xform1 );
    
        ++pPos;
        ++pXFormPos;
        
    }   // for i = 0 to num vertices
    
    aiJointIndices = pAnimModel->maiNormalJointIndices;
    afBlendVals = pAnimModel->mafNormalBlendVals;
    for( int i = 0; i < iNumNorms; i++ )
    {
        tJoint* pJoint0 = &aJoints[*aiJointIndices++];
        tJoint* pJoint1 = &aJoints[*aiJointIndices++];
        
        tVector4 xform0, xform1;
        Matrix44Transform( &xform0, pNorm, pJoint0->mpInvTransSkinningMatrix );
        Matrix44Transform( &xform1, pNorm, pJoint1->mpInvTransSkinningMatrix );
        
        Vector4MultScalar( &xform0, &xform0, *afBlendVals++ );
        Vector4MultScalar( &xform1, &xform1, *afBlendVals++ );
        
        Vector4Add( pXFormNorm, &xform0, &xform1 );
        
        ++pNorm;
        ++pXFormNorm;
        
    }   // for i = 0 to num normals
}

/*
**
*/
void animModelInstanceUpdateXForm( tAnimModelInstance* pModelInstance )
{
    int iNumXForms = pModelInstance->miNumXForms;
    Matrix44Identity( &pModelInstance->mXFormMat );
    Matrix44Identity( &pModelInstance->mRotMat );
    
    for( int i = 0; i < iNumXForms; i++ )
    {
        tMatrix44 rotMat, scaleMat, posMat, posRotMat, posRotScaleMat;
        tMatrix44 prevMatrix, prevRotMatrix;
        memcpy( &prevMatrix, &pModelInstance->mXFormMat, sizeof( tMatrix44 ) );
        memcpy( &prevRotMatrix, &pModelInstance->mRotMat, sizeof( tMatrix44 ) );
        
        tVector4 const* pRotation = &pModelInstance->maRotations[i];
        tVector4 const* pScale = &pModelInstance->maScalings[i];
        tVector4 const* pTranslation = &pModelInstance->maPositions[i];
        tVector4 const* pScalePivot = &pModelInstance->maScalePivot[i];
        tVector4 const* pRotatePivot = &pModelInstance->maRotatePivot[i];
        
#if 0
        tQuaternion xRot, yRot,zRot, xyRot, totalRot;
        quaternionFromAxisAngle( &xRot, &gXAxis, pRotation->fX );
        quaternionFromAxisAngle( &yRot, &gYAxis, pRotation->fY );
        quaternionFromAxisAngle( &zRot, &gZAxis, pRotation->fZ );
        
        quaternionMultiply( &xRot, &xRot, &yRot );
        quaternionMultiply( &totalRot, &xyRot, &zRot );
        
        quaternionToMatrix( &rotMat, &totalRot );
#endif // #if 0
        
        tMatrix44 rotMatX, rotMatY, rotMatZ, rotMatZY;
        Matrix44RotateX( &rotMatX, pRotation->fX );
        Matrix44RotateY( &rotMatY, pRotation->fY );
        Matrix44RotateZ( &rotMatZ, pRotation->fZ );
        Matrix44Multiply( &rotMatZY, &rotMatZ, &rotMatY );
        Matrix44Multiply( &rotMat, &rotMatZY, &rotMatX );
        
        Matrix44Multiply( &pModelInstance->mRotMat, &rotMat, &prevRotMatrix );
        
        Matrix44Scale( &scaleMat, pScale->fX, pScale->fY, pScale->fZ );
        Matrix44Translate( &posMat, pTranslation );
        
        // pivot-1
        tVector4 negScalePivot, negRotatePivot;
        Vector4MultScalar( &negScalePivot, pScalePivot, -1.0f );
        Vector4MultScalar( &negRotatePivot, pRotatePivot, -1.0f );
        
        // pivot and pivot-1 matrices
        tMatrix44 scalePivotMat, negScalePivotMat, rotatePivotMat, negRotatePivotMat;
        Matrix44Translate( &scalePivotMat, pScalePivot );
        Matrix44Translate( &negScalePivotMat, &negScalePivot );
        
        Matrix44Translate( &rotatePivotMat, pRotatePivot );
        Matrix44Translate( &negRotatePivotMat, &negRotatePivot );
        
        // pivot * rotation * pivot-1
        tMatrix44 pivotRotMat, totalPivotRot;
        Matrix44Multiply( &pivotRotMat, &rotatePivotMat, &rotMat );
        Matrix44Multiply( &totalPivotRot, &pivotRotMat, &negRotatePivotMat );
        
        // pivot * scale * pivot-1
        tMatrix44 pivotScaleMat, totalPivotScale;
        Matrix44Multiply( &pivotScaleMat, &scalePivotMat, &scaleMat );
        Matrix44Multiply( &totalPivotScale, &pivotScaleMat, &negScalePivotMat );
        
        
        // translate * pivot * rotation * pivot-1 * pivot * scale * pivot-1
        Matrix44Multiply( &posRotMat, &posMat, &totalPivotRot );
        Matrix44Multiply( &posRotScaleMat, &posRotMat, &totalPivotScale );
        Matrix44Multiply( &pModelInstance->mXFormMat, &posRotScaleMat, &prevMatrix );
        
    }   // for i = 0 to num xforms

    tAnimModel const* pModel = pModelInstance->mpAnimModel;
    tVector4 newDimension;
    memcpy( &newDimension, &pModel->mDimension, sizeof( tVector4 ) );
    
    newDimension.fX *= pModelInstance->mXFormMat.M( 0, 0 );
    newDimension.fY *= pModelInstance->mXFormMat.M( 1, 1 );
    newDimension.fZ *= pModelInstance->mXFormMat.M( 2, 2 );
    
    if( newDimension.fX > newDimension.fY &&
       newDimension.fX > newDimension.fZ )
    {
        pModelInstance->mfRadius = newDimension.fX * 0.5f;
    }
    else if( newDimension.fY > newDimension.fX &&
            newDimension.fY > newDimension.fZ )
    {
        pModelInstance->mfRadius = newDimension.fY * 0.5f;
    }
    else
    {
        pModelInstance->mfRadius = newDimension.fZ * 0.5f;
    }
}
