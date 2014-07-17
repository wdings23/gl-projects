//
//  face.h
//  animtest
//
//  Created by Tony Peng on 7/19/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#ifndef __FACE_H__
#define __FACE_H__

struct Face
{
    int             maiV[3];
    int             maiUV[3];
    int             maiNorm[3];
};

typedef struct Face tFace;

#endif // __FACE_H__
