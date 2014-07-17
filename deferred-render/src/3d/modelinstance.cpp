//
//  modelinstance.cpp
//  animtest
//
//  Created by Dingwings on 8/8/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "modelinstance.h"
#include "quaternion.h"
#include "texturemanager.h"

#include <stdlib.h>

static void packColor( tVector4* pResult, int iColor );

/*
**
*/
void modelInstanceInit( tModelInstance* pModelInstance )
{
    memset( pModelInstance, 0, sizeof( tModelInstance ) );
    Matrix44Identity( &pModelInstance->mXFormMat );
	Matrix44Identity( &pModelInstance->mRotationMatrix );
	Matrix44Identity( &pModelInstance->mAnimMatrix );
	pModelInstance->mbDrawModel = true;
}

/*
**
*/
void modelInstanceRelease( tModelInstance* pModelInstance )
{
    FREE( pModelInstance->maiVBOIndices );
    FREE( pModelInstance->maUV );
    FREE( pModelInstance->maVBOVerts );
    FREE( pModelInstance->maXFormNorm );
    FREE( pModelInstance->maXFormPos );
    FREE( pModelInstance->maXFormMaps );
    
    FREE( pModelInstance->maPositions );
    FREE( pModelInstance->maRotations );
    FREE( pModelInstance->maScalings );
    FREE( pModelInstance->maScalePivot );
    FREE( pModelInstance->maRotatePivot );

	FREE( pModelInstance->maFaceColorVerts );
	FREE( pModelInstance->maiFaceColorIndices );

	if( pModelInstance->mpModelKeyAnim )
	{
		FREE( pModelInstance->mpModelKeyAnim );
	}
}

/*
**
*/
void modelInstanceSet( tModelInstance* pModelInstance, tModel const* pModel )
{
    pModelInstance->mpModel = pModel;
    
#if 0
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

/*
**
*/
void modelInstanceSetupGL( tModelInstance* pModelInstance )
{
	modelInstanceCopyVBData( pModelInstance );

    // data array
    glGenBuffers( 1, &pModelInstance->miVBO );
    glBindBuffer( GL_ARRAY_BUFFER, pModelInstance->miVBO );
    glBufferData( GL_ARRAY_BUFFER, sizeof( tFaceColorVert ) * pModelInstance->miNumVBOVerts, pModelInstance->maVBOVerts, GL_STATIC_DRAW );
    
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
    if( pModelInstance->mpModel->miLoadState != MODEL_LOADSTATE_LOADED )
	{
        return;
    }

    glUseProgram( iShader );

    GLint iViewProj = glGetUniformLocation( iShader, "viewProjMat" );
    WTFASSERT2( iViewProj >= 0, "invalid view proj matrix" );

	tMatrix44 normalMat;
	Matrix44Identity( &normalMat );

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
    //WTFASSERT2( iNorm >= 0, "invalid normal shader semantic" );

	// texture
	if( *pModelInstance->mszTexture )
	{
		GLuint iAlbedo = glGetUniformLocation( iShader, "albedo" );
		WTFASSERT2( iAlbedo >= 0, "invalid albedo shader semantic" );
		glUniform1i( iAlbedo, 0 );

		GLuint iLight = glGetUniformLocation( iShader, "lightTex" );
		WTFASSERT2( iLight >= 0, "invalid light shader semantic" );
		glUniform1i( iLight, 1 );
		
		GLuint iDraw = glGetUniformLocation( iShader, "drawTex" );
		WTFASSERT2( iDraw >= 0, "invalid draw shader semantic" );
		glUniform1i( iDraw, 2 );

		tTexture* pTexture = CTextureManager::instance()->getTexture( pModelInstance->mszTexture );
		if( pTexture == NULL )
		{
			CTextureManager::instance()->registerTexture( pModelInstance->mszTexture );
			pTexture = CTextureManager::instance()->getTexture( pModelInstance->mszTexture );
			WTFASSERT2( pTexture, "can't load texture %s", pModelInstance->mszTexture );
		}

		// texture
		// albedo
		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, pTexture->miID );

#if 0
		tTexture* pTexture = CTextureManager::instance()->getTexture( pModelInstance->mszLightName );
		if( pTexture == NULL )
		{
			CTextureManager::instance()->registerTexture( pModelInstance->mszLightName );
			pTexture = CTextureManager::instance()->getTexture( pModelInstance->mszLightName );
			WTFASSERT2( pTexture, "can't load texture %s", pModelInstance->mszLightName );
		}

		// light texture
		glActiveTexture( GL_TEXTURE1 );
		glBindTexture( GL_TEXTURE_2D, pTexture->miID );
#endif // #if 0
	}
    
	tTexture* pTexture = CTextureManager::instance()->getTexture( pModelInstance->mszTexture );

    glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, pTexture->miID );

    glBindBuffer( GL_ARRAY_BUFFER, pModelInstance->mpModel->miVBO );
    
    glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, sizeof( tInterleaveVert ), (void *)0 );
    glEnableVertexAttribArray( iPos );
    
    glVertexAttribPointer( iNorm, 4, GL_FLOAT, GL_FALSE, sizeof( tInterleaveVert ), (void *)( sizeof( tVector4 ) + sizeof( tVector2 ) ) );
    glEnableVertexAttribArray( iNorm );
    
	if( *pModelInstance->mszTexture )
	{
		GLint iUV = glGetAttribLocation( iShader, "uv" );
		WTFASSERT2( iUV >= 0, "invalid uv shader semantic" );
		
		glVertexAttribPointer( iUV, 2, GL_FLOAT, GL_FALSE, sizeof( tInterleaveVert ), (void *)( sizeof( tVector4 ) ) );
		glEnableVertexAttribArray( iUV );
	}
	
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pModelInstance->mpModel->miIBO );
    
    glDrawElements( GL_TRIANGLES, pModelInstance->mpModel->miNumVBOIndices, GL_UNSIGNED_INT, (void *)0 );
    
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

/*
**
*/
void modelInstanceUpdateXForm( tModelInstance* pModelInstance, tVector4 const* pOffset )
{
    int iNumXForms = pModelInstance->miNumXForms;
    Matrix44Identity( &pModelInstance->mXFormMat );
    Matrix44Identity( &pModelInstance->mRotationMatrix );
    
    for( int i = 0; i < iNumXForms; i++ )
    {
        tMatrix44 rotMat, scaleMat, posMat, posRotMat, posRotScaleMat;
        tMatrix44 prevMatrix, prevRotMatrix;
        memcpy( &prevMatrix, &pModelInstance->mXFormMat, sizeof( tMatrix44 ) );
        memcpy( &prevRotMatrix, &pModelInstance->mRotationMatrix, sizeof( tMatrix44 ) );
        
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
        
        Matrix44Multiply( &pModelInstance->mRotationMatrix, &rotMat, &prevRotMatrix );
        
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
    
    // apply offset
    pModelInstance->mXFormMat.M( 0, 3 ) += pOffset->fX;
    pModelInstance->mXFormMat.M( 1, 3 ) += pOffset->fY;
    pModelInstance->mXFormMat.M( 2, 3 ) += pOffset->fZ;
    
    tModel const* pModel = pModelInstance->mpModel;
    tVector4 newDimension;
    memcpy( &newDimension, &pModel->mDimension, sizeof( tVector4 ) );
    
    // mult dimension by scale 
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

/*
**
*/
void modelInstanceSetupFaceColorGL( tModelInstance* pModelInstance )
{
	tModel const* pModel = pModelInstance->mpModel;

	// need to wait until the model is loaded
	if( pModel->miLoadState != MODEL_LOADSTATE_LOADED )
	{
		pModelInstance->miGLSetupState = MODELINSTANCE_GLSETUP_STATE_NONE;
		return;
	}

    int iNumFaces = pModel->miNumFaces;
	
	// first time allocation
	int iNumVerts = iNumFaces * 3;
	pModelInstance->maFaceColorVerts = (tFaceColorVert *)MALLOC( sizeof( tFaceColorVert ) * iNumVerts );
	pModelInstance->maiFaceColorIndices = (int *)MALLOC( sizeof( int ) * iNumVerts );

	tFaceColorVert* pV = pModelInstance->maFaceColorVerts;
	int* piIndex = pModelInstance->maiFaceColorIndices;
	
	// copy position
	int iCount = 0;
	for( int i = 0; i < iNumFaces; i++ )
	{
		for( int j = 0; j < 3; j++ )
		{
			int iIndex = pModel->maFaces[i].maiV[j];
			tVector4* pPos = &pModel->maPos[iIndex];
			memcpy( &pV->mPos, pPos, sizeof( tVector4 ) );
			*piIndex++ = iCount++;
			++pV;

		}	// for j = 0 to 3

	}	// for i = 0 to num verts

#if 0
	// data array
    glGenBuffers( 1, &pModelInstance->miFaceColorVBO );
    glBindBuffer( GL_ARRAY_BUFFER, pModelInstance->miFaceColorVBO );
	glBufferData( GL_ARRAY_BUFFER, sizeof( tFaceColorVert ) * iNumVerts, pModelInstance->maFaceColorVerts, GL_DYNAMIC_DRAW );
    
    // index array
    glGenBuffers( 1, &pModelInstance->miFaceColorIBO );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pModelInstance->miFaceColorIBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( int ) * iNumVerts, pModelInstance->maiFaceColorIndices, GL_STATIC_DRAW );
    
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
#endif // #if 0

	pModelInstance->miGLSetupState = MODELINSTANCE_GLSETUP_STATE_FINISHED;
}

/*
**
*/
void modelInstanceInitVBO( tModelInstance* pModelInstance )
{
	tModel const* pModel = pModelInstance->mpModel;

	// need to wait until the model is loaded
	if( pModel->miLoadState != MODEL_LOADSTATE_LOADED )
	{
		pModelInstance->miGLSetupState = MODELINSTANCE_GLSETUP_STATE_NONE;
		return;
	}

    int iNumFaces = pModel->miNumFaces;
	
	// first time allocation
	int iNumVerts = iNumFaces * 3;

	// data array
    glGenBuffers( 1, &pModelInstance->miFaceColorVBO );
    glBindBuffer( GL_ARRAY_BUFFER, pModelInstance->miFaceColorVBO );
	glBufferData( GL_ARRAY_BUFFER, sizeof( tFaceColorVert ) * iNumVerts, pModelInstance->maFaceColorVerts, GL_DYNAMIC_DRAW );
    
    // index array
    glGenBuffers( 1, &pModelInstance->miFaceColorIBO );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pModelInstance->miFaceColorIBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( int ) * iNumVerts, pModelInstance->maiFaceColorIndices, GL_STATIC_DRAW );
}

/*
**
*/
void modelInstanceReleaseVBO( tModelInstance* pModelInstance )
{
	glDeleteBuffers( 1, &pModelInstance->miFaceColorVBO );
	glDeleteBuffers( 1, &pModelInstance->miFaceColorIBO );
}

/*
**
*/
void modelInstanceUpdateFaceColorVerts( tModelInstance* pModelInstance, int iVisibleModelIndex )
{
	int iNumFaces = pModelInstance->mpModel->miNumFaces;
	tFaceColorVert* pVert = pModelInstance->maFaceColorVerts;

	for( int i = 0; i < iNumFaces; i++ )
	{
		for( int j = 0; j < 3; j++ )
		{
			// model index = upper 10 bits, face index = rest of 14 bits
			// start is at 1, all indices = 0 = nothing is selected
			int iColor = ( ( ( iVisibleModelIndex + 1 ) << 22 ) | ( ( i + 1 ) << 8 ) );
			packColor( &pVert->mColor, iColor );
			pVert->mColor.fW = 1.0f;		// to show up during glReadPixels

			++pVert;

		}	// for j = 0 to 3

	}	// for i = 0 to num verts
}

/*
**
*/
void modelInstanceDrawFaceColor( tModelInstance* pModelInstance,
								 tMatrix44 const* pViewMatrix,
								 tMatrix44 const* pProjMatrix,
								 GLint iShader )
{
	if( pModelInstance->mpModel->miLoadState != MODEL_LOADSTATE_LOADED )
	{
        return;
    }

    glUseProgram( iShader );

	// view projection matrix
    GLint iViewProj = glGetUniformLocation( iShader, "viewProjMat" );
    WTFASSERT2( iViewProj >= 0, "invalid view proj matrix" );

    tMatrix44 xformMatrix, transpose, viewProjMatrix;
    Matrix44Multiply( &xformMatrix, pViewMatrix, &pModelInstance->mXFormMat );
    Matrix44Transpose( &transpose, &xformMatrix );
    Matrix44Multiply( &viewProjMatrix, &transpose, pProjMatrix );
    
    glUniformMatrix4fv( iViewProj, 1, GL_FALSE, viewProjMatrix.afEntries );
    
	// position, color semantic
    GLint iPos = glGetAttribLocation( iShader, "position" );
    GLint iColor = glGetAttribLocation( iShader, "color" );
   
    WTFASSERT2( iPos >= 0, "invalid position shader semantic" );
    WTFASSERT2( iColor >= 0, "invalid color shader semantic" );
    
	// vbo
    glBindBuffer( GL_ARRAY_BUFFER, pModelInstance->miFaceColorVBO );
    
    glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, sizeof( tFaceColorVert ), (void *)0 );
    glEnableVertexAttribArray( iPos );
    
    glVertexAttribPointer( iColor, 4, GL_FLOAT, GL_FALSE, sizeof( tFaceColorVert ), (void *)( sizeof( tVector4 ) ) );
    glEnableVertexAttribArray( iColor );
    
	// update vbo data
	glBufferData( GL_ARRAY_BUFFER, 
				  sizeof( tFaceColorVert ) * pModelInstance->mpModel->miNumFaces * 3, 
				  pModelInstance->maFaceColorVerts, 
				  GL_DYNAMIC_DRAW );
	
	// ibo
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pModelInstance->miFaceColorIBO );
    
    glDrawElements( GL_TRIANGLES, pModelInstance->mpModel->miNumFaces * 3, GL_UNSIGNED_INT, (void *)0 );
    
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

/*
**
*/
void modelInstanceDrawDepth( tModelInstance* pModelInstance,
							 tMatrix44 const* pViewMatrix,
							 tMatrix44 const* pProjMatrix,
							 GLint iShader )
{
	if( pModelInstance->mpModel->miLoadState != MODEL_LOADSTATE_LOADED )
	{
        return;
    }

	tMatrix44 transposeViewMatrix, transposeProjMatrix, viewProjMatrix;
	
	tMatrix44 xformMatrix;
    Matrix44Multiply( &xformMatrix, pViewMatrix, &pModelInstance->mXFormMat );
	
	Matrix44Transpose( &transposeViewMatrix, &xformMatrix );
	Matrix44Transpose( &transposeProjMatrix, pProjMatrix );
	Matrix44Multiply( &viewProjMatrix, &transposeViewMatrix, pProjMatrix );

	glUseProgram( iShader );

	GLint iViewMatrix = glGetUniformLocation( iShader, "viewMatrix" );
	//WTFASSERT2( iViewMatrix >= 0, "invalid view matrix" );

	GLint iProjMatrix = glGetUniformLocation( iShader, "projMatrix" );
	//WTFASSERT2( iProjMatrix >= 0, "invalid proj matrix" ); 

	GLint iViewProjMatrix = glGetUniformLocation( iShader, "viewProjMatrix" );

	glUniformMatrix4fv( iViewMatrix, 1, GL_FALSE, transposeViewMatrix.afEntries );
	glUniformMatrix4fv( iProjMatrix, 1, GL_FALSE, transposeProjMatrix.afEntries );
	glUniformMatrix4fv( iViewProjMatrix, 1, GL_FALSE, viewProjMatrix.afEntries );

	GLint iPos = glGetAttribLocation( iShader, "position" );
    WTFASSERT2( iPos >= 0, "invalid position shader semantic" );
    
    glBindBuffer( GL_ARRAY_BUFFER, pModelInstance->mpModel->miVBO );
    
    glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, sizeof( tInterleaveVert ), (void *)0 );
    glEnableVertexAttribArray( iPos );
    
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pModelInstance->mpModel->miIBO );
    
    glDrawElements( GL_TRIANGLES, pModelInstance->mpModel->miNumVBOIndices, GL_UNSIGNED_INT, (void *)0 );
    
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

/*
**
*/
void modelInstanceDrawShadow( tModelInstance* pModelInstance,
							  tMatrix44 const* pViewMatrix,
							  tMatrix44 const* pProjMatrix,
							  tMatrix44 const* pLightViewMatrix,
							  tMatrix44 const* pLightProjMatrix,
							  GLuint iLightTexID,
							  GLint iShader )
{
	if( pModelInstance->mpModel->miLoadState != MODEL_LOADSTATE_LOADED )
	{
        return;
    }

	tMatrix44 xformLightViewMatrix, lightViewMatrix, lightProjMatrix;
	
	// light view projection
	//Matrix44Multiply( &xformLightViewMatrix, pLightViewMatrix, &pModelInstance->mXFormMat );
	//Matrix44Transpose( &transposeLightViewMatrix, &xformLightViewMatrix );
	//Matrix44Multiply( &lightViewProjMatrix, &transposeLightViewMatrix, pLightProjMatrix );
	
	Matrix44Multiply( &xformLightViewMatrix, pLightViewMatrix, &pModelInstance->mXFormMat );
	Matrix44Transpose( &lightViewMatrix, &xformLightViewMatrix );
	Matrix44Transpose( &lightProjMatrix, pLightProjMatrix );

	tMatrix44 xformMatrix;
    Matrix44Multiply( &xformMatrix, pViewMatrix, &pModelInstance->mXFormMat );
	
	// transpose for gl
	tMatrix44 transposeViewMatrix, transposeProjMatrix;
	Matrix44Transpose( &transposeViewMatrix, &xformMatrix );
	Matrix44Transpose( &transposeProjMatrix, pProjMatrix );

	glUseProgram( iShader );
	
	GLuint iTex0 = glGetUniformLocation( iShader, "lightPOVDepth0" );
	glUniform1i( iTex0, 0 );

	GLuint iTex1 = glGetUniformLocation( iShader, "lightPOVDepth1" );
	glUniform1i( iTex1, 0 );

	GLuint iTex2 = glGetUniformLocation( iShader, "lightPOVDepth2" );
	glUniform1i( iTex2, 0 );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, iLightTexID );
	
	GLint iViewMatrix = glGetUniformLocation( iShader, "viewMatrix" );
	GLint iProjMatrix = glGetUniformLocation( iShader, "projMatrix" );
	
	GLint iLightProjMatrix = glGetUniformLocation( iShader, "lightProjMatrix" );
	GLint iLightViewMatrix = glGetUniformLocation( iShader, "lightViewMatrix" );

	glUniformMatrix4fv( iViewMatrix, 1, GL_FALSE, transposeViewMatrix.afEntries );
	glUniformMatrix4fv( iProjMatrix, 1, GL_FALSE, transposeProjMatrix.afEntries );
	
	glUniformMatrix4fv( iLightProjMatrix, 1, GL_FALSE, lightProjMatrix.afEntries );
	glUniformMatrix4fv( iLightViewMatrix, 1, GL_FALSE, lightViewMatrix.afEntries );

	GLint iPos = glGetAttribLocation( iShader, "position" );
    WTFASSERT2( iPos >= 0, "invalid position shader semantic" );
    
	tModel const* pModel = pModelInstance->mpModel;

    glBindBuffer( GL_ARRAY_BUFFER, pModel->miVBO );
    
    glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, sizeof( tInterleaveVert ), (void *)0 );
    glEnableVertexAttribArray( iPos );
    
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pModel->miIBO );
    glDrawElements( GL_TRIANGLES, pModel->miNumVBOIndices, GL_UNSIGNED_INT, (void *)0 );
     
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

/*
**
*/
static void packColor( tVector4* pResult, int iColor )
{
	int iComp0 = ( ( iColor & 0xff000000 ) >> 24 );
	int iComp1 = ( ( iColor & 0x00ff0000 ) >> 16 );
	int iComp2 = ( ( iColor & 0x0000ff00 ) >> 8 );
	int iComp3 = ( iColor & 0x000000ff );

	pResult->fX = (float)iComp0 / 255.0f;
	pResult->fY = (float)iComp1 / 255.0f;
	pResult->fZ = (float)iComp2 / 255.0f;
	pResult->fW = (float)iComp3 / 255.0f;
}