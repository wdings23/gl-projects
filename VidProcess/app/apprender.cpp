//
//  apprender.cpp
//  VidProcess
//
//  Created by Dingwings on 6/8/14.
//  Copyright (c) 2014 Dingwings. All rights reserved.
//

#include "apprender.h"
#include "shadermanager.h"
#include "tga.h"
#include "texturemanager.h"
#include "timeutil.h"

#define CAPTURE_TEXTURE_WIDTH 640
#define CAPTURE_TEXTURE_HEIGHT 480

enum
{
    SHADER_ATTRIB_POSITION = 0,
    SHADER_ATTRIB_UV,
    SHADER_ATTRIB_COLOR,
    
    NUM_SHADER_ATTRIBS,
};

/*
**
*/
CAppRender::CAppRender( void ) : macTextureData( NULL ),
                                 macTempLineData( NULL )
{
}

/*
**
*/
CAppRender::~CAppRender( void )
{
    free( macTextureData );
    free( macTempLineData );
}

/*
**
*/
void CAppRender::init( void )
{
    // load all shaders
    CShaderManager::instance()->loadAllPrograms( "shaders.txt" );
    
    // screen quad data
    maData[0].mPos.fX = -1.0f; maData[0].mPos.fY = 1.0f; maData[0].mPos.fZ = 0.0f; maData[0].mPos.fW = 1.0f;
    maData[1].mPos.fX = -1.0f; maData[1].mPos.fY = -1.0f; maData[1].mPos.fZ = 0.0f; maData[1].mPos.fW = 1.0f;
    maData[2].mPos.fX = 1.0f; maData[2].mPos.fY = 1.0f; maData[2].mPos.fZ = 0.0f; maData[2].mPos.fW = 1.0f;
    maData[3].mPos.fX = 1.0f; maData[3].mPos.fY = -1.0f; maData[3].mPos.fZ = 0.0f; maData[3].mPos.fW = 1.0f;
    
    maData[0].mUV.fX = 0.0f; maData[0].mUV.fY = 0.0f;
    maData[1].mUV.fX = 0.0f; maData[1].mUV.fY = 1.0f;
    maData[2].mUV.fX = 1.0f; maData[2].mUV.fY = 0.0f;
    maData[3].mUV.fX = 1.0f; maData[3].mUV.fY = 1.0f;
    
    maData[0].mColor.fX = maData[0].mColor.fY = maData[0].mColor.fZ = maData[0].mColor.fW = 1.0f;
    maData[1].mColor.fX = maData[1].mColor.fY = maData[1].mColor.fZ = maData[1].mColor.fW = 1.0f;
    maData[2].mColor.fX = maData[2].mColor.fY = maData[2].mColor.fZ = maData[2].mColor.fW = 1.0f;
    maData[3].mColor.fX = maData[3].mColor.fY = maData[3].mColor.fZ = maData[3].mColor.fW = 1.0f;
    
    miShader = CShaderManager::instance()->getShaderProgram( "noperspective" )->miID;
    
    if( macTextureData == NULL )
    {
        macTextureData = (uint8_t *)malloc( CAPTURE_TEXTURE_WIDTH * CAPTURE_TEXTURE_HEIGHT * 4 );
        memset( macTextureData, 0, CAPTURE_TEXTURE_WIDTH * CAPTURE_TEXTURE_HEIGHT * 4 );
    }
    
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    glGenTextures( 1, &miCaptureTexID );
    glBindTexture( GL_TEXTURE_2D, miCaptureTexID );
    
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    
    glTexImage2D( GL_TEXTURE_2D,
                  0,
                  GL_RGBA,
                  CAPTURE_TEXTURE_WIDTH,
                  CAPTURE_TEXTURE_HEIGHT,
                  0,
                  GL_RGBA,
                  GL_UNSIGNED_BYTE,
                  macTextureData );
}

/*
**
*/
void CAppRender::draw( void )
{
    // update texture data
    glBindTexture( GL_TEXTURE_2D, miCaptureTexID );
    glTexImage2D( GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 CAPTURE_TEXTURE_WIDTH,
                 CAPTURE_TEXTURE_HEIGHT,
                 0,
                 GL_BGRA,
                 GL_UNSIGNED_BYTE,
                 macTextureData );
    
    glClearColor( 0.0f, 0.0f, 1.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    glEnable( GL_TEXTURE_2D );
    
    glUseProgram( miShader );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, miCaptureTexID );
    
    // sprite quad in local space
    tVector4 quad[] =
    {
        { -1.0f, 1.0f, 0.0f, 1.0f },
        { -1.0f, -1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, -1.0f, 0.0f, 1.0f },
    };
    
    tVector4 aColor[] =
    {
        { 1.0f, 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f },
    };
    
    tVector2 aUV[] =
    {
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 0.0f },
        { 0.0f, 1.0f },
    };
    
    GLint iPos = glGetAttribLocation( miShader, "position" );
    GLint iUV = glGetAttribLocation( miShader, "textureUV" );
    GLint iColor = glGetAttribLocation( miShader, "color" );
    
    glEnableVertexAttribArray( iPos );
    glEnableVertexAttribArray( iUV );
    glEnableVertexAttribArray( iColor );
    
    glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, quad );
    glVertexAttribPointer( iUV, 2, GL_FLOAT, GL_FALSE, 0, aUV );
    glVertexAttribPointer( iColor, 4, GL_FLOAT, GL_FALSE, 0, aColor );
    
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
    
}

/*
**
*/
void CAppRender::setTextureData( uint8_t const* pTextureData, int iWidth, int iHeight )
{
    if( macTextureData == NULL )
    {
        macTextureData = (uint8_t *)malloc( CAPTURE_TEXTURE_WIDTH * CAPTURE_TEXTURE_HEIGHT * 4 );
        memset( macTextureData, 0, CAPTURE_TEXTURE_WIDTH * CAPTURE_TEXTURE_HEIGHT * 4 );
    }
    
#if 0
    // scaled to texture data
    float fRatioX = (float)iWidth / (float)CAPTURE_TEXTURE_WIDTH;
    float fRatioY = (float)iHeight / (float)CAPTURE_TEXTURE_HEIGHT;

    for( int iY = 0; iY < CAPTURE_TEXTURE_HEIGHT; iY++ )
    {
        int iScaledY = (int)( (float)iY * fRatioY );
        int iLineSize = iY * CAPTURE_TEXTURE_WIDTH;
        int iScaledLineSize = iScaledY * iWidth;
        
        for( int iX = 0; iX < CAPTURE_TEXTURE_WIDTH; iX++ )
        {
            int iScaledX = (int)( (float)iX * fRatioX );
            int iScaledIndex = ( ( iScaledLineSize + iScaledX ) << 2 );
            
            int iIndex = ( ( iLineSize + iX ) << 2 );
            
            macTextureData[iIndex+2] = pTextureData[iScaledIndex];
            macTextureData[iIndex+1] = pTextureData[iScaledIndex+1];
            macTextureData[iIndex] = pTextureData[iScaledIndex+2];
            macTextureData[iIndex+3] = pTextureData[iScaledIndex+3];
        
        }   // for x = 0 to capture texture width
        
    }   // for y = 0 to capture texture height
#endif // #if 0
    
    memcpy( macTextureData, pTextureData, iWidth * iHeight * 4 * sizeof( char ) );
    
}

/*
**
*/
void CAppRender::setShaderFromIndex( int iShaderIndex )
{
    miShader = CShaderManager::instance()->getShaderFromIndex( iShaderIndex )->miID;
}

/*
**
*/
void CAppRender::setShader( tShaderProgram const* pShader )
{
    miShader = pShader->miID;
}

/*
**
*/
void CAppRender::getFrameBuffer( unsigned char* acOut, int iWidth, int iHeight )
{
double fStartTime = getTime();
    
    glReadPixels( 0, 0, iWidth, iHeight, GL_BGRA, GL_UNSIGNED_BYTE, acOut );
    
double fElapsed = getTime() - fStartTime;
//OUTPUT( "0 fElapsed = %.2f\n", fElapsed );

fStartTime = getTime();
    
#if 0
    size_t iScanLineSize = iWidth * 4;
    if( macTempLineData == NULL )
    {
        macTempLineData = (unsigned char *)malloc( sizeof( char ) * iWidth * 4 );
    }
    
    // flip upside down
    for( int iY = 0; iY < iHeight >> 1; iY++ )
    {
        int iTopIndex = ( iY * iWidth ) << 2;
        int iBottomIndex = ( ( iHeight - iY - 1 ) * iWidth ) << 2;
        
        memcpy( macTempLineData, &acOut[iTopIndex], iScanLineSize );
        memcpy( &acOut[iTopIndex], &acOut[iBottomIndex], iScanLineSize );
        memcpy( &acOut[iBottomIndex], macTempLineData, iScanLineSize );
        
    }
#endif // #if 0
    
fElapsed = getTime() - fStartTime;
//OUTPUT( "1 fElapsed = %.2f\n", fElapsed );
    
}
