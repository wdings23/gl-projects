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
#include "textureatlasmanager.h"

enum
{
	MODELINSTANCE_GLSETUP_STATE_NONE = 0,
	MODELINSTANCE_GLSETUP_STATE_SETTING,
	MODELINSTANCE_GLSETUP_STATE_FINISHED,

	NUM_MODELINSTANCE_GLSETUP_STATES
};

#define NUM_LOD_MODELS			3

struct ModelInstance
{
    tModel const*           mpModel;
	tModel const*			mapLODModels[NUM_LOD_MODELS];

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
    tMatrix44               mRotationMatrix;
    
    GLuint                  miVBO;
    GLuint                  miIBO;
    GLint					miAlbedoID;
    GLint                   miLightmapID;
    
    float                   mfRadius;
    
    char                    mszName[256];
	char					mszTexture[256];
	char					mszLightName[256];
    bool                    mbDrawn;
    
	tFaceColorVert*			maFaceColorVerts;
	int*					maiFaceColorIndices;
	
	GLuint                  miFaceColorVBO;
    GLuint                  miFaceColorIBO;

    int						miGLSetupState;
    
    char                    mszDrawTexture[256];
	
	tTextureAtlasInfo const* mpDrawTextureAtlasInfo;
	bool					mbDrawModel;
};

typedef struct ModelInstance tModelInstance;

void modelInstanceInit( tModelInstance* pModelInstance );
void modelInstanceSet( tModelInstance* pModelInstance, tModel const* pModel );
void modelInstanceCopyVBData( tModelInstance* pModelInstance );
void modelInstanceSetupGL( tModelInstance* pModelInstance );
void modelInstanceDraw( tModelInstance* pModelInstance,
                        tMatrix44 const* pViewMatrix,
                        tMatrix44 const* pProjMatrix,
                        GLint iShader );
void modelInstanceUpdateXForm( tModelInstance* pModelInstance, tVector4 const* pOffset );

void modelInstanceSetupFaceColorGL( tModelInstance* pModelInstance );
void modelInstanceUpdateFaceColorVerts( tModelInstance* pModelInstance, int iVisibleModelIndex );
void modelInstanceDrawFaceColor( tModelInstance* pModelInstance,
								 GLint iShader );

void modelInstanceDrawDepth( tModelInstance* pModelInstance,
							 tMatrix44 const* pViewMatrix,
							 tMatrix44 const* pProjMatrix,
							 GLint iShader );

void modelInstanceDrawShadow( tModelInstance* pModelInstance,
							  tMatrix44 const* pViewMatrix,
							  tMatrix44 const* pProjMatrix,
							  tMatrix44 const* pLightViewMatrix,
							  tMatrix44 const* pLightProjMatrix,
							  GLuint iLightTexID,
							  GLint iShader );

void modelInstanceInitVBO( tModelInstance* pModelInstance );
void modelInstanceReleaseVBO( tModelInstance* pModelInstance );

#endif // __MODELINSTANCE_H__