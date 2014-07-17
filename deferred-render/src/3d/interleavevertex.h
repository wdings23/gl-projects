//
//  interleavevertex.h
//  animtest
//
//  Created by Tony Peng on 8/8/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#ifndef __INTERLEAVEVERTEX_H__
#define __INTERLEAVEVERTEX_H__

#include "vector.h"

struct InterleaveVert
{
    tVector4		mPos;
    tVector2		mUV;
    tVector4		mNorm;
	tVector4		mColor;
	
	tVector4		maLightPos[1];
	tVector4		mOrigPos;
	tVector4		mOrigNorm;
	tVector4		maProjPos[1];
};

typedef struct InterleaveVert tInterleaveVert;

struct InterleaveVertMap
{
    int         miPos;
    int         miNorm;
    int         miUV;
};

typedef struct InterleaveVertMap tInterleaveVertMap;


struct XFormMap
{
    int     miXFormPos;
    int     miXFormNorm;
    int     miUV;
};

typedef struct XFormMap tXFormMap;

struct FaceColorVert
{
	tVector4		mPos;
	tVector4		mColor;
};

typedef struct FaceColorVert tFaceColorVert;

#endif // __INTERLEAVEVERTEX_H__