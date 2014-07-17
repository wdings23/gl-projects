//
//  model.h
//  animtest
//
//  Created by Tony Peng on 8/8/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#ifndef __MODEL_H__
#define __MODEL_H__

#include "vector.h"
#include "face.h"
#include "interleavevertex.h"

struct Model
{
    tVector4*               maPos;
    tVector4*               maNorm;
    tVector2*               maUV;
    
    tFace*                  maFaces;
    
    int                     miNumVerts;
    int                     miNumUV;
    int                     miNumNormals;
    int                     miNumFaces;
    
    tInterleaveVert*        maVBOVerts;
    tInterleaveVertMap*     maVBOVertPtrs;  // ptr to xform pos and norm for copy operation in model instance
    int                     miNumVBOVerts;
    unsigned int*           maiVBOIndices;
    int                     miNumVBOIndices;
    
    tVector4                mDimension;
    tVector4                mCenter;
    
    char*                    mszName;
    
#if 0
    GLuint                  miVBO;
    GLuint                  miIBO;
#endif // #if 0

    bool                    mbLoaded;
};

typedef struct Model tModel;

void modelInit( tModel* pModel );
void modelLoad( tModel* pModel, const char* szFileName );
void modelRelease( tModel* pModel );
void modelSetupGL( tModel* pModel );

#endif // __MODEL_H__
