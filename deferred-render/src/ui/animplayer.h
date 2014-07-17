//
//  animplayer.h
//  CityBenchmark
//
//  Created by Dingwings on 7/31/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//
#ifndef __ANIMPLAYER_H__
#define __ANIMPLAYER_H__

#include "vector.h"

#define MAX_ANIM_FRAMES 10

struct AnimFrame
{
    tVector4    mColor;
    tVector4    mPos;
    tVector4    mRotation;
    tVector4    mScale;
    
    double      mfTime;
    char        mszLabel[64];
};

typedef struct AnimFrame tAnimFrame;

class CAnimPlayer
{
public:
    CAnimPlayer( void );
    ~CAnimPlayer( void );
    
    bool getFrame( tAnimFrame* pAnimFrame, double fTime );
    bool addFrame( const tAnimFrame* pAnimFrame );
    
protected:
    int miNumFrames;
    tAnimFrame  maFrames[MAX_ANIM_FRAMES];
};


#endif // __ANIMPLAYER_H__