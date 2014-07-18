//
//  app.cpp
//  VidProcess
//
//  Created by Dingwings on 6/8/14.
//  Copyright (c) 2014 Dingwings. All rights reserved.
//

#include "app.h"
#include "shadermanager.h"
#include "filepathutil.h"

/*
**
*/
CApp* CApp::mpInstance = NULL;
CApp* CApp::instance( void )
{
    if( mpInstance == NULL )
    {
        mpInstance = new CApp();
    }
    
    return mpInstance;
}

/*
**
*/
CApp::CApp( void ) : mfTotalTime( 0.0 ),
                     mbRecording( false ),
                     mfStartRecordingTime( 0.0 )
{
    memset( mszMovieName, 0, sizeof( mszMovieName ) );
}

/*
**
*/
CApp::~CApp( void )
{
    
}

/*
**
*/
void CApp::init( void )
{
    mAppRender.init();
    mfTotalTime = 0.0;
}

/*
**
*/
void CApp::update( double fDT )
{
    mfTotalTime += fDT;
    if( mbRecording )
    {
        
    }
    
    mAppRender.setTimeElapsed( fDT );
}

/*
**
*/
void CApp::draw( void )
{
    mAppRender.draw();
}

/*
**
*/
void CApp::setCaptureTextureID( GLuint iTexID )
{
    mAppRender.setCaptureTextureID( iTexID );
}

/*
**
*/
void CApp::startRecording( void )
{
    mbRecording = true;
    mfStartRecordingTime = mfTotalTime;
    
    GLuint iShader = mAppRender.getShader();
    tShaderProgram const* pShader = CShaderManager::instance()->getShaderProgramWithShaderID( iShader );
    WTFASSERT2( pShader, "can't find shader with id: %d", iShader );
    maKeyFrames.clear();
    
    tKeyFrame keyFrame;
    keyFrame.mfTime = 0.0;
    strncpy( keyFrame.mszShaderName, pShader->mszName, sizeof( keyFrame.mszShaderName ) );
    
    maKeyFrames.push_back( keyFrame );
    
}

/*
**
*/
void CApp::stopRecording( void )
{
    char szValidMovieName[64];
    getValidFileName( szValidMovieName, "nfo" );
    
    char szFullPath[256];
    getWritePath( szFullPath, szValidMovieName );
    
    FILE* fp = fopen( szFullPath, "wb" );
    fprintf( fp, "<fx>\n" );
    
    fprintf( fp, "\t<movie>%s</movie>\n", mszMovieName );
    
    std::vector<tKeyFrame>::iterator keyFrameIter = maKeyFrames.begin();
    
    // save out all the keyframes
    for( ; keyFrameIter != maKeyFrames.end(); ++keyFrameIter )
    {
        tKeyFrame keyFrame = *keyFrameIter;
    
        fprintf( fp, "\t<keyframe>\n" );
        fprintf( fp, "\t\t<time>%f</time>\n", keyFrame.mfTime );
        fprintf( fp, "\t\t<shader>%s</shader>\n", keyFrame.mszShaderName );
        fprintf( fp, "\t</keyframe>\n" );
    }
    
    fprintf( fp, "</fx>\n" );
    
    fclose( fp );
    
    OUTPUT( "movie keyframe file: %s SAVED\n", szValidMovieName );
}

/*
**
*/
void CApp::setMovieName( const char* szMovieName )
{
    strncpy( mszMovieName, szMovieName, sizeof( mszMovieName ) );
}

/*
**
*/
void CApp::getValidFileName( char* szMovieName, const char* szExtension )
{
    std::vector<std::string> aFileNames;
    getAllFilesInDirectory( "", aFileNames );
    
    // get the last number
    int iLastNum = 0;
    std::vector<std::string>::iterator iter = aFileNames.begin();
    for( ; iter != aFileNames.end(); ++iter )
    {
        std::string fileName = *iter;
        const char* szFileName = fileName.c_str();
        
        char szTotalExtension[32];
        snprintf( szTotalExtension, sizeof( szTotalExtension ), ".%s", szExtension );
        if( strstr( szFileName, szTotalExtension ) )
        {
            if( !strcmp( szFileName, "temp.mov" ) )
            {
                continue;
            }
            
            const char* szStart = (const char *)( (uint64_t)szFileName + (uint64_t)strlen( "movie" ) );
            const char* szEnd = strstr( szFileName, "." );
            
            char szNum[32];
            memset( szNum, 0, sizeof( szNum ) );
            memcpy( szNum, szStart, (size_t)( (uint64_t)szEnd - (uint64_t)szStart ) );
            int iNum = atoi( szNum );
            
            if( iNum > iLastNum )
            {
                iLastNum = iNum;
            }
        }   // if has .nfo
        
    }   // for filename iter = begin to end
    
    iLastNum += 1;
    
    // file name
    snprintf( szMovieName, sizeof( char ) * 64, "movie%d.%s", iLastNum, szExtension );
}

/*
**
*/
void CApp::setShaderFromIndex( int iShaderIndex )
{
    mAppRender.setShaderFromIndex( iShaderIndex );
 
    tShaderProgram const* pShader = CShaderManager::instance()->getShaderFromIndex( iShaderIndex );
    WTFASSERT2( pShader, "can't get shader with iindex: %d", iShaderIndex );
    
    tKeyFrame newKeyFrame;
    
    // save out info
    newKeyFrame.mfTime = mfTotalTime - mfStartRecordingTime;
    strncpy( newKeyFrame.mszShaderName, pShader->mszName, sizeof( newKeyFrame.mszShaderName ) );
    strncpy( newKeyFrame.mszMovieName, mszMovieName, sizeof( newKeyFrame.mszMovieName ) );
    
    maKeyFrames.push_back( newKeyFrame );

}

/*
**
*/
void CApp::setShader( tShaderProgram const* pShader )
{
    mAppRender.setShader( pShader );
}