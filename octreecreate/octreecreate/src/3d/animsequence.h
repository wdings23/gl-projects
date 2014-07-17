//
//  animsequence.h
//  animtest
//
//  Created by Tony Peng on 6/25/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#ifndef __ANIMSEQUENCE_H__
#define __ANIMSEQUENCE_H__

#include "vector.h"
#include "quaternion.h"
#include "factory.h"
#include "joint.h"

struct AnimSequence
{
    float*              mafTime;
    tVector4*           maPositions;
    tQuaternion*        maRotation;
    tVector4*           maScalings;
    
    tVector4*           maRotationVec;
    
    tJoint**            mapJoints;
    
    unsigned int        miNumFrames;
    
    unsigned int        miNumJoints;
    tJoint**            mapUniqueJoints;
    
    unsigned int*       maiStartFrames;
    unsigned int*       maiEndFrames;
    
    float               mfLastTime;
};

typedef struct AnimSequence tAnimSequence;

void animSequenceInit( tAnimSequence* pAnimSequence );
void animSequenceLoad( tAnimSequence* pAnimSequence,
                      const char* szFileName,
                      tAnimHierarchy const* pAnimHierarchy,
                      CFactory<tVector4>* pVectorFactory,
                      CFactory<tQuaternion>* pQuaternionFactory );

void animSequencePlay( tAnimSequence const* pAnimSequence,
                       tVector4* aPositions,
                       tVector4* aScalings,
                       tQuaternion* aRotations,
                       tVector4* aRotationVecs,
                       tMatrix44* aAnimMatrices,
                       float fTime );

void animSequenceUpdateJointAnimValues( tAnimSequence const* pAnimSequence,
                                        tAnimHierarchy const* pAnimHierarchy,
                                        tVector4* aPositions,
                                        tVector4* aScalings,
                                        tQuaternion* aRotations,
                                        tVector4* aRotationVecs,
                                        float fTime );

void animSequenceUpdateAnimMatrices( tAnimSequence const* pAnimSequence,
                                     tAnimHierarchy const* pAnimHierarchy,
                                     tVector4* aPositions,
                                     tVector4* aScalings,
                                     tQuaternion* aRotations,
                                     tVector4* aRotationVecs,
                                     tMatrix44* aAnimMatrices );

#endif // __ANIMSEQUENCE_H__