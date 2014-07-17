//
//  gamerender.h
//  animtest
//
//  Created by Tony Peng on 7/30/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#ifndef __GAMERENDER_H__
#define __GAMERENDER_H__

#include "animmodelinstance.h"

enum
{
    RENDEROBJECT_MODEL = 0,
    RENDEROBJECT_ANIMMODEL,
    
    NUM_RENDEROBJECT_TYPES,
};

struct JointMatrixData
{
    tJoint*         mpJoint;
    tMatrix44*      mpAnimMatrix;
};

typedef struct JointMatrixData tJointMatrixData;

struct JointJobData
{
    tAnimSequence const*          mpSequence;
    tAnimHierarchy const*         mpHierarchy;
    tVector4*                     maPositions;
    tVector4*                     maScalings;
    tQuaternion*                  maRotations;
    tVector4*                     maRotationVecs;
    tMatrix44*                    maAnimMatrices;
    float                         mfTime;
    
    tAnimHierarchy*               mpResultAnimHiearchy;
};

typedef struct JointJobData tJointJobData;

struct GameRenderObject
{
    tVector4*               mpHeading;
    tVector4*               mpSize;
    tVector4*               mpPosition;
    float                   mfSpeed;
    float                   mfAnimMult;
    
    int                     miType;
    
    tAnimModelInstance*     mpAnimModelInstance;
};

typedef struct GameRenderObject tGameRenderObject;

void gamerRenderInitAllObjects( tAnimModel const* pAnimModel,
                                tAnimSequence* pAnimSequence,
                                tAnimHierarchy const* pAnimHierarchy );
void gamerRenderUpdateAllObjects( float fDT );

void gameRenderReleaseAllObjects( void );

void gameRenderDraw( tMatrix44 const* pViewMatrix,
                     tMatrix44 const* pProjMatrix,
                     int iShader );

void gameRenderAnimModelJob( tAnimSequence const* pAnimSequence,
                             tAnimHierarchy const* pAnimHierarchy,
                             tAnimHierarchy* pResultHierarchy,
                             tVector4* aScalings,
                             tVector4* aPositions,
                             tQuaternion* aRotations,
                             tVector4* aRotationVecs,
                             tMatrix44* aAnimMatrices,
                             float fTime );

void gameRenderSetXForm( int iObject,
                         tVector4 const* pPosition,
                         tVector4 const* pScaling,
                         tVector4 const* pRotation );

tVector4 const* gameRenderGetObjectPosition( int iObject );

#endif // __GAMERENDER_H__
