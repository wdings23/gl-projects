//
//  modelinstance.h
//  animtest
//
//  Created by Tony Peng on 8/8/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#ifndef __MODELINSTANCE_H__
#define __MODELINSTANCE_H__

#include "vector.h"
#include "interleavevertex.h"
#include "matrix.h"
#include "model.h"

struct ModelInstance
{
    tModel const*           mpModel;
    
    tVector4*               maXFormPos;
    tVector2*               maUV;
    tVector4*               maXFormNorm;
    
    int                     miNumPos;
    int                     miNumUV;
    int                     miNumNormals;
    
    tInterleaveVert*        maVBOVerts;
    int                     miNumVBOVerts;
    
    unsigned int*           maiVBOIndices;
    int                     miNumVBOIndices;
    
    tXFormMap*              maXFormMaps;
    
    tVector4*                maRotations;
    tVector4*                maScalings;
    tVector4*                maPositions;
    tVector4*                maRotatePivot;
    tVector4*                maScalePivot;
    int                      miNumXForms;
    
    
    tMatrix44               mXFormMat;
    tMatrix44               mRotMat;
    
#if 0
    GLuint                  miVBO;
    GLuint                  miIBO;
#endif // #if 0

    float                   mfRadius;
    
    char                    mszName[256];
    bool                    mbDrawn;

	tVector4				mLargest;
	tVector4				mSmallest;
};

typedef struct ModelInstance tModelInstance;

void modelInstanceInit( tModelInstance* pModelInstance );
void modelInstanceSet( tModelInstance* pModelInstance, tModel const* pModel );

#if 0
void modelInstanceCopyVBData( tModelInstance* pModelInstance );
void modelInstanceSetupGL( tModelInstance* pModelInstance );
void modelInstanceDraw( tModelInstance* pModelInstance,
                        tMatrix44 const* pViewMatrix,
                        tMatrix44 const* pProjMatrix,
                        GLint iShader );
#endif // #if 0
void modelInstanceUpdateXForm( tModelInstance* pModelInstance );

#endif // __MODELINSTANCE_H__