//
//  apprender.h
//  VidProcess
//
//  Created by Dingwings on 6/8/14.
//  Copyright (c) 2014 Dingwings. All rights reserved.
//

#pragma once

#include "interleavevertex.h"
#include "shadermanager.h"

class CApp;

class CAppRender
{
public:
    CAppRender( void );
    ~CAppRender( void );
    
    void init( void );
    void draw( void );
    
    void getFrameBuffer( unsigned char* acOut, int iWidth, int iHeight );
    inline int getShader( void ) { return miShader; }
    
    
    void setTextureData( uint8_t const* pTextureData, int iWidth, int iHeight );
    void setShaderFromIndex( int iShaderIndex );
    void setShader( tShaderProgram const* pShader );
    
    inline void setTimeElapsed( double fDT ) { mfDT = fDT; }
    inline void setCaptureTextureID( GLuint iTexID ) { miCaptureTexID = iTexID; }
    
protected:
    double                  mfDT;
    GLuint                  miCaptureTexID;
    
    tInterleaveVert         maData[4];
    GLuint                  miVBO;
    GLuint                  miShader;

    uint8_t*                macTextureData;
    uint8_t*                macTempLineData;
};
