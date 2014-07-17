//
//  interleavevertex.h
//  animtest
//
//  Created by Tony Peng on 8/8/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#ifndef __INTERLEAVEVERTEX_H__
#define __INTERLEAVEVERTEX_H__

struct InterleaveVert
{
    tVector4       mPos;
    tVector2       mUV;
    tVector4       mNorm;
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

#endif // __INTERLEAVEVERTEX_H__