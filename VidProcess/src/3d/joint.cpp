//
//  joint.cpp
//  animtest
//
//  Created by Tony Peng on 6/24/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "joint.h"
#include "quaternion.h"

static const tVector4 xAxis = { 1.0f, 0.0f, 0.0f, 1.0f };
static const tVector4 yAxis = { 0.0f, 1.0f, 0.0f, 1.0f };
static const tVector4 zAxis = { 0.0f, 0.0f, 1.0f, 1.0f };

static const tMatrix44 identityMatrix = { { 1.0f, 0.0f, 0.0f, 0.0f ,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f } };


/*
**
*/
void jointInit( tJoint* pJoint,
                CFactory<tMatrix44>* pMatrixFactory,
                tVector4 const* pRot,
                tVector4 const* pPosition,
                tVector4 const* pScale )
{
	tQuaternion qX, qY, qZ;
	quaternionInit( &qX );
	quaternionInit( &qY );
	quaternionInit( &qZ );
	
	// angle to quaternion
	quaternionFromAxisAngle( &qX, &xAxis, pRot->fX );
	quaternionFromAxisAngle( &qY, &yAxis, pRot->fY );
	quaternionFromAxisAngle( &qZ, &zAxis, pRot->fZ );
	
    tMatrix44 orientationMatrix;
    
#if 0
	// orientation
	tQuaternion qZY, orientation;
	quaternionMultiply( &qZY, &qZ, &qY );
	quaternionMultiply( &orientation, &qZY, &qX );
	quaternionToMatrix( &orientationMatrix, &orientation );
#endif // #if 0
    
//#if 0
    tMatrix44 rotMatX, rotMatY, rotMatZ, rotMatZY;
    Matrix44RotateX( &rotMatX, pRot->fX );
    Matrix44RotateY( &rotMatY, pRot->fY );
    Matrix44RotateZ( &rotMatZ, pRot->fZ );
    Matrix44Multiply( &rotMatZY, &rotMatZ, &rotMatY );
    Matrix44Multiply( &orientationMatrix, &rotMatZY, &rotMatX );
//#endif // #if 0
    
    
    tMatrix44* aMatrices = pMatrixFactory->alloc( 7 );
    pJoint->mpLocalMatrix = &aMatrices[0];
    pJoint->mpSkinMatrix = &aMatrices[1];
    pJoint->mpTotalMatrix = &aMatrices[2];
    pJoint->mpPoseMatrix = &aMatrices[3];
    pJoint->mpInvPoseMatrix = &aMatrices[4];
    pJoint->mpInvTransSkinningMatrix = &aMatrices[5];
    pJoint->mpAnimMatrix = &aMatrices[6];
    
    Matrix44Identity( pJoint->mpLocalMatrix );
    Matrix44Identity( pJoint->mpSkinMatrix );
    Matrix44Identity( pJoint->mpTotalMatrix );
    Matrix44Identity( pJoint->mpPoseMatrix );
    Matrix44Identity( pJoint->mpInvPoseMatrix );
    Matrix44Identity( pJoint->mpInvTransSkinningMatrix );
    Matrix44Identity( pJoint->mpAnimMatrix );
    
	// scale
	tMatrix44 scaleMatrix;
	Matrix44Scale( &scaleMatrix, pScale->fX, pScale->fY, pScale->fZ );
    
	// position
	tMatrix44 positionMatrix;
	Matrix44Translate( &positionMatrix, pPosition );
	
	// total local
	tMatrix44 posRotMatrix;
	Matrix44Multiply( &posRotMatrix, &positionMatrix, &orientationMatrix );
	Matrix44Multiply( pJoint->mpLocalMatrix, &posRotMatrix, &scaleMatrix );
    
	// parent's pose
	tMatrix44 const* pParentMatrix = &identityMatrix;
	if( pJoint->mpParent )
	{
		pParentMatrix = pJoint->mpParent->mpPoseMatrix;
	}
	
	// pose
	Matrix44Multiply( pJoint->mpPoseMatrix, pParentMatrix, pJoint->mpLocalMatrix );
    
	// inverse pose
	Matrix44Inverse( pJoint->mpInvPoseMatrix, pJoint->mpPoseMatrix );
    
    pJoint->miNumChildrenAlloc = 100;
    pJoint->miNumChildren = 0;
    pJoint->mapChildren = (tJoint **)MALLOC( pJoint->miNumChildrenAlloc * sizeof( tJoint* ) );
}

/*
**
*/
void jointRelease( tJoint* pJoint )
{
    FREE( pJoint->mszName );
    FREE( pJoint->mapChildren );
}

/*
**
*/
void jointUpdate( tJoint* pJoint, tMatrix44 const* pAnimMatrix )
{
	// parent
	tMatrix44 const* pParentMatrix = &identityMatrix;
	if( pJoint->mpParent )
	{
		pParentMatrix = pJoint->mpParent->mpTotalMatrix;
	}
	
	// parent * local * animation
	tMatrix44 parentLocalMatrix;
	Matrix44Multiply( &parentLocalMatrix,
                     pParentMatrix,
                     pJoint->mpLocalMatrix );
	Matrix44Multiply( pJoint->mpTotalMatrix,
                     &parentLocalMatrix,
                     pAnimMatrix );
	
	// skin = total * inverse pose
	Matrix44Multiply( pJoint->mpSkinMatrix,
                     pJoint->mpTotalMatrix,
                     pJoint->mpInvPoseMatrix );
	
	// inverse transpose skin
	tMatrix44 invSkinningMatrix;
	Matrix44Inverse( &invSkinningMatrix, pJoint->mpSkinMatrix );
	Matrix44Transpose( pJoint->mpInvTransSkinningMatrix, &invSkinningMatrix );
}

/*
**
*/
void jointSetParent( tJoint* pJoint, tAnimHierarchy const* pAnimHierarchy, const char* szParentName )
{
	int iNumJoints = pAnimHierarchy->miNumJoints;
	for( int i = 0; i < iNumJoints; i++ )
	{
		tJoint* pParentJoint = &pAnimHierarchy->maJoints[i];
		if( pParentJoint->mszName && !strcmp( pParentJoint->mszName, szParentName ) )
		{
			pJoint->mpParent = pParentJoint;
            if( pParentJoint->miNumChildren >= pParentJoint->miNumChildrenAlloc )
            {
                pParentJoint->miNumChildrenAlloc += 100;
                pParentJoint->mapChildren = (tJoint **)REALLOC( pParentJoint->mapChildren, pParentJoint->miNumChildrenAlloc );
            }
            
            pParentJoint->mapChildren[pParentJoint->miNumChildren++] = pJoint;
            
			break;
		}
        
	}	// for i = 0 to num joints
}