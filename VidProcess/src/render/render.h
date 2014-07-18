//
//  render.h
//  ProjectG
//
//  Created by Dingwings on 11/6/13.
//  Copyright (c) 2013 Dingwings. All rights reserved.
//

#ifndef __RENDER_H__
#define __RENDER_H__

#include "vector.h"

int renderGetScreenWidth( void );
int renderGetScreenHeight( void );
float renderGetScreenScale( void );

void renderQuad( const char* szTextureName,
                 tVector4 const* aTransformedV,
                 tVector4 const* aTextColor,
                 tVector2 const* aUV,
                 int iShader,
				 GLuint iSrcBlend = GL_SRC_ALPHA,
				 GLuint iDestBlend = GL_ONE_MINUS_SRC_ALPHA );

void renderSetToUseScissorBatch( bool bScissor,
                                 int iX,
                                 int iY,
                                 int iWidth,
                                 int iHeight );

void renderSetScreenWidth( int iScreenWidth );
void renderSetScreenHeight( int iScreenHeight );
void renderSetScreenScale( float fScreenScale );

#endif
