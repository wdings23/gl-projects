#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <android/log.h>

#include "wtfassert.h"

#define ANDROID 1

#ifdef ANDROID
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#define OUTPUT( ... ) __android_log_print( ANDROID_LOG_INFO, "!!! WTF !!!", __VA_ARGS__ )
#define printf( ... ) __android_log_print( ANDROID_LOG_INFO, "!!! WTF !!!", __VA_ARGS__ )

#define SAVELOG OUTPUT

#else
#include <OpenGLES/ES2/gl.h>

#define OUTPUT( ... ) printf( __VA_ARGS__ )

#endif // ANDROID

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 480

#define MALLOC( X ) malloc( X )
#define FREE( X ) free( X )
#define REALLOC( X, Y ) realloc( X, Y ); OUTPUT( "REALLOC realloc( %s, %s )\n", #X, #Y );
#define PI 3.14159f

#define UI_USE_BATCH 1
#define USE_VBO 1

#define glClearDepth glClearDepthf
#define glDepthRange glDepthRangef
#define glDrawElementsInstancedEXT glDrawElementsInstanced
#define GL_RGB8_OES GL_RGB8
#define GL_DEPTH_COMPONENT24_OES GL_DEPTH_COMPONENT24
//#define WTFASSERT2( X, ... ) if( X == false ) { OUTPUT( "ASSERT" ); OUTPUT( #X ); OUTPUT( __VA_ARGS__ ); OUTPUT( "\n" ); assert( X ); }
