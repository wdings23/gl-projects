//
//  modelinstance.h
//  animtest
//
//  Created by Tony Peng on 7/23/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#ifndef __ANIMMODELINSTANCE_H__
#define __ANIMMODELINSTANCE_H__

#include "animmodel.h"
#include "animsequence.h"
#include "interleavevertex.h"

struct AnimModelInstance
{
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
    
    tAnimModel const*       mpAnimModel;
    tAnimHierarchy const*   mpAnimHierarchy;
    
    tAnimSequence const*    mpAnimSequence;
    float                   mfAnimTime;
    
    tMatrix44*              maAnimMatrices;
    
    tVector4                mRotation;
    tVector4                mScaling;
    tVector4                mPosition;
    tMatrix44               mXFormMat;
    
    tVector4                mDimension;
    
    GLuint                  miVBO;
    GLuint                  miIBO;

    tVector4*                maRotations;
    tVector4*                maScalings;
    tVector4*                maPositions;
    tVector4*                maRotatePivot;
    tVector4*                maScalePivot;
    int                      miNumXForms;
    
    tMatrix44               mRotMat;
    float                   mfRadius;
    
    bool                    mbPreXform;
    
    char                    mszName[256];
	tMatrix44				mRotationMatrix;

	char					mszTexture[256];
};

typedef struct AnimModelInstance tAnimModelInstance;

void animModelInstanceInit( tAnimModelInstance* pAnimModelInstance );
void animModelInstanceRelease( tAnimModelInstance* pAnimModelInstance );

void animModelInstanceUpdate( tAnimModelInstance* pAnimModelInstance );
void animModelInstanceUpdateXForm( tAnimModelInstance* pAnimModelInstance );


void animModelInstanceSet( tAnimModelInstance* pAnimModelInstance, tAnimModel const* pAnimModel );
void animModelInstanceSetAnimSequence( tAnimModelInstance* pAnimModelInstance, tAnimSequence const* pSequence );
void animModelInstanceCopyVBData( tAnimModelInstance* pAnimModelInstance );
void animModelInstanceSetupGL( tAnimModelInstance* pAnimModelInstance );

void animModelInstanceDraw( tAnimModelInstance* pAnimModelInstance,
                            tMatrix44 const* pViewMatrix,
                            tMatrix44 const* pProjMatrix,
                            GLint iShader );

void animModelInstanceDrawDepth( tAnimModelInstance* pAnimModelInstance,
								 tMatrix44 const* pViewMatrix,
								 tMatrix44 const* pProjMatrix,
								 GLint iShader );

void animModelInstanceDrawShadow( tAnimModelInstance* pAnimModelInstance,
								  tMatrix44 const* pViewMatrix,
								  tMatrix44 const* pProjMatrix,
								  tMatrix44 const* pLightViewMatrix,
								  tMatrix44 const* pLightProjMatrix,
								  GLuint iLightTexID,
								  GLint iShader );

void animModelInstanceUpdateXForm( tAnimModelInstance* pModelInstance );

#endif // __ANIMMODELINSTANCE_H__
