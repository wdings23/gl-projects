//
//  joint.h
//  animtest
//
//  Created by Tony Peng on 6/24/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#ifndef __JOINT_H__
#define __JOINT_H__

#include "matrix.h"
#include "animhierarchy.h"
#include "factory.h"

struct Joint
{
	char*					mszName;
	struct Joint*			mpParent;
	struct Joint**			mapChildren;
	int						miNumChildren;
    int                     miNumChildrenAlloc;
    int                     miLevel;
    
	tMatrix44*				mpLocalMatrix;
	tMatrix44*				mpSkinMatrix;
	tMatrix44*				mpTotalMatrix;
	tMatrix44*				mpPoseMatrix;
	tMatrix44*				mpInvPoseMatrix;
	tMatrix44*				mpInvTransSkinningMatrix;
    tMatrix44*              mpAnimMatrix;
};

typedef struct Joint tJoint;

void jointInit( tJoint* pJoint,
                CFactory<tMatrix44>* pMatrixFactory,
                tVector4 const* pRot,
                tVector4 const* pPosition,
                tVector4 const* pScale );

void jointRelease( tJoint* pJoint );

void jointUpdate( tJoint* pJoint, tMatrix44 const* pAnimMatrix );

void jointSetParent( tJoint* pJoint, tAnimHierarchy const* pAnimHierarchy, const char* szParentName );


#endif // __JOINT_H__