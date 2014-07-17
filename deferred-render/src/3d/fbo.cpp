//
//  fbo.cpp
//  ProjectG
//
//  Created by Dingwings on 2/12/14.
//  Copyright (c) 2014 Dingwings. All rights reserved.
//

#include "fbo.h"

//GLint           mFBO;
//GLint           mColorRenderBuffer;
//GLint           mDepthRenderBuffer;
//GLint           mTexture;

/*
**
*/
void fboInit( tFBOInfo* pFBOInfo, int iWidth, int iHeight, int iTextureFilter )
{
    // bind fbo to texture
    glGenFramebuffers( 1, &pFBOInfo->mFBO );
    glBindFramebuffer( GL_FRAMEBUFFER, pFBOInfo->mFBO );
    
    // color buffer for fbo
    glGenRenderbuffers( 1, &pFBOInfo->mColorRenderBuffer );
    glBindRenderbuffer( GL_RENDERBUFFER, pFBOInfo->mColorRenderBuffer );
#if defined( WIN32 ) || defined( MACOS )
	glRenderbufferStorage( GL_RENDERBUFFER, GL_RGB8, iWidth, iHeight );
#else
    glRenderbufferStorage( GL_RENDERBUFFER, GL_RGB8_OES, iWidth, iHeight );
#endif // WIN32

	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, pFBOInfo->mColorRenderBuffer );
    
    // depth buffer for the fbo
    glGenRenderbuffers( 1, &pFBOInfo->mDepthRenderBuffer );
    glBindRenderbuffer( GL_RENDERBUFFER, pFBOInfo->mDepthRenderBuffer );
#if defined( WIN32 ) || defined( MACOS )
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, iWidth, iHeight );
#else
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT24_OES, iWidth, iHeight );
#endif // WIN32

	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pFBOInfo->mDepthRenderBuffer );
    
    // texture
    glGenTextures( 1, &pFBOInfo->mTexture );
    glBindTexture( GL_TEXTURE_2D, pFBOInfo->mTexture );
    
	GLuint iGLFilterType = GL_NEAREST;
	if( iTextureFilter == FILTER_BILINEAR )
	{
		iGLFilterType = GL_LINEAR;
	}
	else if( iTextureFilter == FILTER_NEAREST )
	{
		iGLFilterType = GL_NEAREST;
	}

    // attribute
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, iGLFilterType );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, iGLFilterType );
    
    glTexImage2D( GL_TEXTURE_2D,
                  0,
                  GL_RGBA,
                  iWidth,
                  iHeight,
                  0,
                  GL_RGBA,
                  GL_UNSIGNED_BYTE,
                  NULL );
    
    glFramebufferTexture2D( GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           pFBOInfo->mTexture,
                           0 );

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}