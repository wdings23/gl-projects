//
//  modelinstance.cpp
//  animtest
//
//  Created by Dingwings on 8/8/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "modelinstance.h"
#include "quaternion.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define OUTPUT printf

/*
**
*/
void modelInstanceInit( tModelInstance* pModelInstance )
{
    memset( pModelInstance, 0, sizeof( tModelInstance ) );
    Matrix44Identity( &pModelInstance->mXFormMat );
}

/*
**
*/
void modelInstanceRelease( tModelInstance* pModelInstance )
{
    free( pModelInstance->maiVBOIndices );
    free( pModelInstance->maUV );
    free( pModelInstance->maVBOVerts );
    free( pModelInstance->maXFormNorm );
    free( pModelInstance->maXFormPos );
    free( pModelInstance->maXFormMaps );
    
    free( pModelInstance->maPositions );
    free( pModelInstance->maRotations );
    free( pModelInstance->maScalings );
    free( pModelInstance->maScalePivot );
    free( pModelInstance->maRotatePivot );
}

/*
**
*/
void modelInstanceSet( tModelInstance* pModelInstance, tModel const* pModel )
{
    pModelInstance->mpModel = pModel;
    
    pModelInstance->miNumNormals = pModel->miNumNormals;
    pModelInstance->miNumPos = pModel->miNumVerts;
    pModelInstance->miNumUV = pModel->miNumUV;
    pModelInstance->miNumVBOIndices = pModel->miNumFaces * 3;
    pModelInstance->miNumVBOVerts = pModel->miNumVBOVerts;
    
    pModelInstance->maVBOVerts = (tInterleaveVert *)malloc( sizeof( tInterleaveVert ) * pModelInstance->miNumVBOVerts );
    pModelInstance->maiVBOIndices = (unsigned int *)malloc( sizeof( int ) * pModelInstance->miNumVBOIndices );
    
	pModelInstance->maXFormNorm = (tVector4 *)malloc( sizeof( tVector4 ) * pModelInstance->miNumNormals );
    pModelInstance->maXFormPos = (tVector4 *)malloc( sizeof( tVector4 ) * pModelInstance->miNumPos );
    pModelInstance->maUV = (tVector2 *)malloc( sizeof( tVector2 ) * pModelInstance->miNumUV );
    
#if 0
	pModelInstance->maXFormMaps = (tXFormMap *)malloc( sizeof( tXFormMap ) * pModelInstance->miNumVBOVerts );
    
    memcpy( pModelInstance->maiVBOIndices, pModel->maiVBOIndices, sizeof( int ) * pModelInstance->miNumVBOIndices );
    memcpy( pModelInstance->maVBOVerts, pModel->maVBOVerts, sizeof( tInterleaveVert ) * pModelInstance->miNumVBOVerts );
    
    // set the correct ptr to xform pos and norm
    int iNumVBOVerts = pModel->miNumVBOVerts;
    for( int i = 0; i < iNumVBOVerts; i++ )
    {
        tXFormMap* pPtr = &pModelInstance->maXFormMaps[i];
        
        // indices in the list
        pPtr->miXFormPos = pModel->maVBOVertPtrs[i].miPos;
        pPtr->miXFormNorm = pModel->maVBOVertPtrs[i].miNorm;
        pPtr->miUV = pModel->maVBOVertPtrs[i].miUV;
        
    }   // for i = 0 to num vbo verts
    
    tVector4* aPos = pModel->maPos;
    tVector4* aNorm = pModel->maNorm;
    
    tVector4* aXFormPos = pModelInstance->maXFormPos;
    tVector4* aXFormNorm = pModelInstance->maXFormNorm;
    
    // copy the xformed position and normal over
    for( int i = 0; i < pModel->miNumVerts; i++ )
    {
        memcpy( aXFormPos, aPos, sizeof( tVector4 ) );
        ++aXFormPos;
        ++aPos;
        
    }   // for i = 0 to num vbo verts
    
    for( int i = 0; i < pModel->miNumNormals; i++ )
    {
        memcpy( aXFormNorm, aNorm, sizeof( tVector4 ) );
        ++aXFormNorm;
        ++aNorm;
    }
#endif // #if 0
}

/*
**
*/
void modelInstanceCopyVBData( tModelInstance* pModelInstance )
{
    int iNumVBOVerts = pModelInstance->miNumVBOVerts;
    
    tXFormMap* aXFormMap = pModelInstance->maXFormMaps;
    tInterleaveVert* aV = pModelInstance->maVBOVerts;
    
    tVector4* aXFormPos = pModelInstance->maXFormPos;
    tVector4* aXFormNorm = pModelInstance->maXFormNorm;
    
    // copy the xformed position and normal over
    for( int i = 0; i < iNumVBOVerts; i++ )
    {
        tXFormMap* pXFormMap = &aXFormMap[i];
        tInterleaveVert* pVert = aV;
        
        memcpy( &pVert->mPos, &aXFormPos[pXFormMap->miXFormPos], sizeof( tVector4 ) );
        memcpy( &pVert->mNorm, &aXFormNorm[pXFormMap->miXFormNorm], sizeof( tVector4 ) );
        
OUTPUT( "%d pos ( %f, %f, %f )\n", i, pVert->mPos.fX, pVert->mPos.fY, pVert->mPos.fZ );
        
        ++aV;
        
    }   // for i = 0 to num vbo verts
}

#if 0
/*
**
*/
void modelInstanceSetupGL( tModelInstance* pModelInstance )
{
    modelInstanceCopyVBData( pModelInstance );
    
    // data array
    glGenBuffers( 1, &pModelInstance->miVBO );
    glBindBuffer( GL_ARRAY_BUFFER, pModelInstance->miVBO );
    glBufferData( GL_ARRAY_BUFFER, sizeof( tInterleaveVert ) * pModelInstance->miNumVBOVerts, pModelInstance->maVBOVerts, GL_STATIC_DRAW );
    
    // index array
    glGenBuffers( 1, &pModelInstance->miIBO );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pModelInstance->miIBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( int ) * pModelInstance->miNumVBOIndices, pModelInstance->maiVBOIndices, GL_STATIC_DRAW );
    
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

/*
**
*/
void modelInstanceUpdate( tModelInstance* pModelInstance, tVector4* pRot, tVector4* pPos, tVector4* pScale )
{
    /*tMatrix44 rotMat, scaleMat, posMat, posRotMat;
    
    tQuaternion xRot, yRot, zRot, xyRot, totalRot;
    quaternionFromAxisAngle( &xRot, &gXAxis, pModelInstance->mRotation.fX );
    quaternionFromAxisAngle( &yRot, &gYAxis, pModelInstance->mRotation.fY );
    quaternionFromAxisAngle( &zRot, &gZAxis, pModelInstance->mRotation.fZ );
    
    quaternionMultiply( &xyRot, &xRot, &yRot );
    quaternionMultiply( &totalRot, &xyRot, &zRot );
    
    quaternionToMatrix( &rotMat, &totalRot );
    
    Matrix44Scale( &scaleMat, pModelInstance->mScaling.fX, pModelInstance->mScaling.fY, pModelInstance->mScaling.fZ );
    Matrix44Translate( &posMat, &pModelInstance->mPosition );
    
    Matrix44Multiply( &posRotMat, &posMat, &rotMat );
    Matrix44Multiply( &pModelInstance->mXFormMat, &posRotMat, &scaleMat );*/
    

}

/*
**
*/
void modelInstanceDraw( tModelInstance* pModelInstance,
                        tMatrix44 const* pViewMatrix,
                        tMatrix44 const* pProjMatrix,
                        GLint iShader )
{
    glUseProgram( iShader );

    GLint iViewProj = glGetUniformLocation( iShader, "viewProjMat" );
    WTFASSERT2( iViewProj >= 0, "invalid view proj matrix" );
    
    tMatrix44 normalMat;
    Matrix44Transpose( &normalMat, &pModelInstance->mRotMat );
    
    GLint iNormMat = glGetUniformLocation( iShader, "normMat" );
    
    tMatrix44 xformMatrix, transpose, viewProjMatrix;
    Matrix44Multiply( &xformMatrix, pViewMatrix, &pModelInstance->mXFormMat );
    Matrix44Transpose( &transpose, &xformMatrix );
    Matrix44Multiply( &viewProjMatrix, &transpose, pProjMatrix );
    
    glUniformMatrix4fv( iViewProj, 1, GL_FALSE, viewProjMatrix.afEntries );
    glUniformMatrix4fv( iNormMat, 1, GL_FALSE, normalMat.afEntries );
    
    GLint iPos = glGetAttribLocation( iShader, "position" );
    GLint iNorm = glGetAttribLocation( iShader, "normal" );
    
    WTFASSERT2( iPos >= 0, "invalid position shader semantic" );
    WTFASSERT2( iNorm >= 0, "invalid normal shader semantic" );
    
    glBindBuffer( GL_ARRAY_BUFFER, pModelInstance->miVBO );
    
    //glBufferData( GL_ARRAY_BUFFER, sizeof( tInterleaveVert ) * pModelInstance->miNumVBOVerts, pModelInstance->maVBOVerts, GL_STATIC_DRAW );
    
    glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, sizeof( tInterleaveVert ), (void *)0 );
    glEnableVertexAttribArray( iPos );
    
    glVertexAttribPointer( iNorm, 4, GL_FLOAT, GL_FALSE, sizeof( tInterleaveVert ), (void *)( sizeof( tVector4 ) + sizeof( tVector2 ) ) );
    glEnableVertexAttribArray( iNorm );
    
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pModelInstance->miIBO );
    glDrawElements( GL_TRIANGLES, pModelInstance->miNumVBOIndices, GL_UNSIGNED_INT, (void *)0 );
    
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}
#endif // #if 0

/*
**
*/
void modelInstanceUpdateXForm( tModelInstance* pModelInstance )
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
    
    tModel const* pModel = pModelInstance->mpModel;
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
