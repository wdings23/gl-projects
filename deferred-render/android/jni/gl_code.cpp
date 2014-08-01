/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// OpenGL ES 2.0 code

#include <jni.h>
#include <android/log.h>

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <unistd.h>

#include "filepathutil.h"
#include "timeutil.h"
#include "render.h"

#include "menumanager.h"
#include "vector.h"
#include "game.h"

#include "hud.h"

#include <vector>

#include "android/asset_manager.h"
#include "android/asset_manager_jni.h"

struct TouchInfo
{
    tVector2       maTouchPos[5];
    int             miType;
};

typedef struct TouchInfo tTouchInfo;

static std::vector<tTouchInfo>     sTouchQueue;


static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    OUTPUT("GL %s = %s\n", name, v);

}

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        OUTPUT("after %s() glError (0x%x)\n", op, error);
    }
}

/*
**
*/
bool setupGraphics(int w, int h) {
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    
    glClearColor( 0.0f, 0.0f, 1.0f, 1.0f );

    glClearDepthf( 1.0f );
    glDepthRangef( 0.0f, 1.0f );
    glDepthMask( GL_TRUE );
    glDepthFunc( GL_LEQUAL );
    glEnable( GL_DEPTH_TEST );
    
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    
    renderSetScreenWidth( w );
    renderSetScreenHeight( h );
    renderSetScreenScale( 1.0f );
    
    return true;
}

/*
**
*/
void renderFrame()
{
    glClearColor( 0.65f, 0.65f, 0.65f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    CGame::instance()->draw();
}

/*
**
*/
void handleTouch( float fX, float fY, int iTouchType )
{
    OUTPUT( "%s fX = %f fY = %f type = %d",
            __FUNCTION__,
            fX,
            fY,
            iTouchType );
    
    CGame::instance()->inputUpdate( fX, fY, iTouchType );
    CHUD::instance()->inputUpdate( fX, fY, 0, 1, iTouchType );
}

extern "C" {
    JNIEXPORT void JNICALL Java_com_tableflipstudios_deferredrender_GameLib_init(JNIEnv * env, jobject obj,  jint width, jint height);
    JNIEXPORT void JNICALL Java_com_tableflipstudios_deferredrender_GameLib_step(JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_com_tableflipstudios_deferredrender_GameLib_setSaveFileDir(JNIEnv * env, jobject obj, jstring fileDir);
    JNIEXPORT void JNICALL Java_com_tableflipstudios_deferredrender_GameLib_touchBegan(JNIEnv * env, jobject obj, jint iX, jint iY, jint iID );
    JNIEXPORT void JNICALL Java_com_tableflipstudios_deferredrender_GameLib_touchMoved(JNIEnv * env, jobject obj, jint iX, jint iY, jint iID );
    JNIEXPORT void JNICALL Java_com_tableflipstudios_deferredrender_GameLib_touchEnded(JNIEnv * env, jobject obj, jint iX, jint iY, jint iID );
    JNIEXPORT void JNICALL Java_com_tableflipstudios_deferredrender_GameLib_setAssetManager(JNIEnv* env,
    																	jobject obj,
    																	jobject assetManager );
};

JNIEXPORT void JNICALL Java_com_tableflipstudios_deferredrender_GameLib_init(JNIEnv * env, jobject obj,  jint width, jint height)
{
    OUTPUT( "%s : %d\n", __FILE__, __LINE__ );
    setupGraphics(width, height);
    
    //usleep( 22000000 );
    
    // init game
    CGame::instance()->setSceneFileName( "scene.cac" );
    CGame::instance()->init();
    
    sTouchQueue.clear();
}

JNIEXPORT void JNICALL Java_com_tableflipstudios_deferredrender_GameLib_step(JNIEnv * env, jobject obj)
{
    static double sfLastTime = 0.0f;
    static float sfTime = 0.0f;
    
    double fCurrTime = getCurrTime();
    if( sfLastTime == 0.0f )
    {
        sfLastTime = fCurrTime;
    }
    
    float fElapsed = (float)( fCurrTime - sfLastTime ) * 0.001f;
    
    //OUTPUT( "FRAME ELAPSED = %.4f total time = %.4f\n", fElapsed, sfTime );
    
    // update input
    for( int i = 0; i < sTouchQueue.size(); i++ )
    {
        tTouchInfo touchInfo = sTouchQueue[i];
        float fX = touchInfo.maTouchPos[0].fX;
        float fY = touchInfo.maTouchPos[0].fY;
        int iTouchType = touchInfo.miType;
        
        handleTouch( fX, fY, iTouchType );
    }
    
    sTouchQueue.clear();
    
    // update game
    double fDT = 0.016666f; //fCurrTime - mfLastTime;
    CGame::instance()->update( (float)fDT );
    
    renderFrame();
    
    sfLastTime = fCurrTime;
}

JNIEXPORT void JNICALL Java_com_tableflipstudios_deferredrender_GameLib_setSaveFileDir(JNIEnv * env, jobject obj, jstring fileDir)
{
    const char* szFileDir = env->GetStringUTFChars(fileDir, 0);
    OUTPUT( "!!! szFileDir = %s !!!\n", szFileDir );
    
    char szWriteDir[256];
    snprintf( szWriteDir, sizeof( szWriteDir ), "%s_write", szFileDir );
    setFileDirectories( szFileDir, szWriteDir );
    
    env->ReleaseStringUTFChars( fileDir, szFileDir );
    env->DeleteLocalRef( fileDir );
}

/*
**
*/
JNIEXPORT void JNICALL Java_com_tableflipstudios_deferredrender_GameLib_touchBegan(JNIEnv * env, jobject obj, jint iX, jint iY, jint iID )
{
    OUTPUT( "touch began id %d ( %d, %d )\n", iX, iY, iID );

    tTouchInfo touchInfo;
    touchInfo.maTouchPos[0].fX = (float)iX;
    touchInfo.maTouchPos[0].fY = (float)iY;
    touchInfo.miType = TOUCHTYPE_BEGAN;
    
    sTouchQueue.push_back( touchInfo );
}

/*
**
*/
JNIEXPORT void JNICALL Java_com_tableflipstudios_deferredrender_GameLib_touchMoved(JNIEnv * env, jobject obj, jint iX, jint iY, jint iID )
{
    OUTPUT( "touch moved id %d ( %d, %d )\n", iX, iY, iID );
    
    tTouchInfo touchInfo;
    touchInfo.maTouchPos[0].fX = (float)iX;
    touchInfo.maTouchPos[0].fY = (float)iY;
    touchInfo.miType = TOUCHTYPE_MOVED;
    
    sTouchQueue.push_back( touchInfo );
}

/*
**
*/
JNIEXPORT void JNICALL Java_com_tableflipstudios_deferredrender_GameLib_touchEnded(JNIEnv * env, jobject obj, jint iX, jint iY, jint iID )
{
    OUTPUT( "touch ended id %d ( %d, %d )\n", iX, iY, iID );
    
    tTouchInfo touchInfo;
    touchInfo.maTouchPos[0].fX = (float)iX;
    touchInfo.maTouchPos[0].fY = (float)iY;
    touchInfo.miType = TOUCHTYPE_ENDED;
    
    sTouchQueue.push_back( touchInfo );
}

/*
**
*/
JNIEXPORT void JNICALL Java_com_tableflipstudios_deferredrender_GameLib_setAssetManager(JNIEnv* env,
																	jobject obj,
																	jobject assetManager )
{
	AAssetManager* pAssetManager = AAssetManager_fromJava( env, assetManager );
	OUTPUT( "ASSET MANAGER = 0x%08X\n", (unsigned int)pAssetManager );
	//androidSoundSetAssetManager( pAssetManager );
}
