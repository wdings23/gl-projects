//
//  morphtarget.h
//  Game7
//
//  Created by Dingwings on 5/1/14.
//  Copyright (c) 2014 Dingwings. All rights reserved.
//

#ifndef __MORPHTARGET_H__
#define __MORPHTARGET_H__

#include "model.h"
#include "matrix.h"

struct MorphTarget
{
    tModel*         mpModel;
    tVector4**      maaAnimVertPos;
	tVector4**		maaAnimVertNorm;
    int             miNumFrames;
    
    tMatrix44       mXFormMatrix;
    tMatrix44       mRotationMatrix;
};

typedef struct MorphTarget tMorphTarget;

struct MorphTargetAnimation
{
    tMorphTarget*       mpMorphTarget;
    float               mfDuration;
    float               mfTime;
	int					miAnimFrame;
};

typedef struct MorphTargetAnimation tMorphTargetAnimation;

void morphTargetInit( tMorphTarget* pMorphTarget );
void morphTargetRelease( tMorphTarget* pMorphTarget );
void morphTargetLoad( tMorphTarget* pMorpthTarget, const char* szFileName );


void morphTargetAnimFrameUpdate( tMorphTargetAnimation* pAnimation, float fTime );

#endif // __MORPHTARGET_H__