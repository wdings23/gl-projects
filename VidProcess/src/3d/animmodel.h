//
//  animmodel.h
//  animtest
//
//  Created by Tony Peng on 7/19/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#ifndef __ANIMMODEL_H__
#define __ANIMMODEL_H__

#include "vector.h"
#include "face.h"
#include "animhierarchy.h"
#include "joint.h"
#include "interleavevertex.h"

struct AnimModel
{
    float*                  mafBlendVals;
    tJoint**                mapInfluenceJoints;
    int*                    maiJointIndices;
    int*                    maiNormalJointIndices;
    int                     miNumBlendVals;
    
    float*                  mafNormalBlendVals;
    int                     miNumNormalBlendVals;
    
    tVector4*               maPos;
    tVector4*               maNorm;
    tVector2*               maUV;
    
    tVector4*               maXFormPos;
    tVector4*               maXFormNorm;
    
    tInterleaveVert*        maVBOVerts;
    tInterleaveVertMap*     maVBOVertPtrs;  // ptr to xform pos and norm for copy operation in model instance
    int                     miNumVBOVerts;
    unsigned int*           maiVBOIndices;
    
    tFace*                  maFaces;
    
    tAnimHierarchy const*    mpAnimHierarchy;
    
    int                     miNumVerts;
    int                     miNumUV;
    int                     miNumNormals;
    int                     miNumFaces;
    
    tVector4                mDimension;
    tVector4                mCenter;
    
    char                    mszName[256];
};

typedef struct AnimModel tAnimModel;

void animModelInit( tAnimModel* pAnimModel );
void animModelRelease( tAnimModel* pAnimModel );
void animModelLoad( tAnimModel* pAnimModel,
                    tAnimHierarchy const* pAnimHierarchy,
                    const char* szFileName );

void animModelUpdate( tAnimModel* pAnimModel );

#endif // __ANIMMODEL_H__
