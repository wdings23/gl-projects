//
//  app.h
//  VidProcess
//
//  Created by Dingwings on 6/8/14.
//  Copyright (c) 2014 Dingwings. All rights reserved.
//

#pragma once

#include "apprender.h"
#include <vector>

struct KeyFrame
{
    char            mszMovieName[64];
    char            mszShaderName[64];
    double          mfTime;
};

typedef struct KeyFrame tKeyFrame;

class CApp
{
public:
    CApp( void );
    ~CApp( void );
    
    void init( void );
    void update( double fDT );
    void draw( void );
    
    void setCaptureTextureID( GLuint iTexID );
    inline void setTextureData( uint8_t const* pTextureData, int iWidth, int iHeight )
    {
        mAppRender.setTextureData( pTextureData, iWidth, iHeight );
    }
    
    void setShaderFromIndex( int iShaderIndex );
    void setShader( tShaderProgram const* pShader );
    
    inline void getFrameBuffer( unsigned char* acOut, int iWidth, int iHeight )
    {
        mAppRender.getFrameBuffer( acOut, iWidth, iHeight );
    }
    
    void startRecording( void );
    void stopRecording( void );
    
    void setMovieName( const char* szMovieName );
    
protected:
    double                          mfTotalTime;
    
    double                          mfStartRecordingTime;
    bool                            mbRecording;
    
    CAppRender                      mAppRender;
    
    std::vector<tKeyFrame>          maKeyFrames;
    char                            mszMovieName[256];
    
public:
    static CApp* instance( void );
    
    static void getValidFileName( char* szMovieName, const char* szExtension );
    
protected:
    static CApp*        mpInstance;
};
