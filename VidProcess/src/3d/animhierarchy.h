//
//  animhierarchy.h
//  animtest
//
//  Created by Tony Peng on 6/24/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#ifndef __ANIMHIERARCHY_H__
#define __ANIMHIERARCHY_H__

#include "factory.h"
#include "matrix.h"

struct Joint;

struct AnimHierarchy
{
	struct Joint*			mpRoot;
	struct Joint*			maJoints;
	int                     miNumJoints;
    int                     miMaxJointLevel;
};

typedef struct AnimHierarchy tAnimHierarchy;

void animHierarchyInit( tAnimHierarchy* pAnimHierarchy );
void animHierarchyLoad( tAnimHierarchy* pAnimHierarchy,
                        const char* szFileName,
                        CFactory<struct Joint>* pJointFactory,
                        CFactory<tMatrix44>* pMatrixFactory );

void animHierarchyCopy( tAnimHierarchy* pAnimHierarchy,
                        tAnimHierarchy const* pOrig,
                        CFactory<struct Joint>* pJointFactory,
                        CFactory<tMatrix44>* pMatrixFactory );

void animHierarchyPrintMatrices( tAnimHierarchy const* pAnimHierarchy );

#endif // __ANIMHIERARCHY_H__
