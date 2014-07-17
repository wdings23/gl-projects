//
//  fbo.h
//  ProjectG
//
//  Created by Dingwings on 2/12/14.
//  Copyright (c) 2014 Dingwings. All rights reserved.
//

#ifndef __FBO_H__
#define __FBO_H__

enum
{
	FILTER_NEAREST = 0,
	FILTER_BILINEAR,

	NUM_FILTER_TYPES,
};

struct FBOInfo
{
    GLuint           mFBO;
    GLuint           mColorRenderBuffer;
    GLuint           mDepthRenderBuffer;
    GLuint           mTexture;
};

typedef struct FBOInfo tFBOInfo;

void fboInit( tFBOInfo* pFBOInfo, int iWidth, int iHeight, int iFilter );


#endif // __FBO_H__
