#pragma once

#include <stdio.h>
#include <tchar.h>

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

#include <glee.h>
#include <glut.h>

#include "memutil.h"
#include "wtfassert.h"

#define OUTPUT printf
#define snprintf( X, Y, ... ) _snprintf_s( X, Y, Y, __VA_ARGS__ )

//#define WTFASSERT2( X, ... ) if( ( X ) == false ) { OUTPUT( "ASSERT" ); OUTPUT( #X ); OUTPUT( __VA_ARGS__ ); OUTPUT( "\n" ); assert( X ); }

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 480

#define WINDOWS 1

//#define REALLOC( X, Y ) debugRealloc( #X, X, Y )
//#define FREE( X ) debugFree( #X, X )
//#define MALLOC( X ) debugMalloc( #X, X )

#define REALLOC( X, Y ) realloc( X, Y )
#define FREE( X ) free( X )
#define MALLOC( X ) malloc( X )

#define USE_VBO 1

#define sleep( X ) Sleep( X )

#define PI 3.14159f

#define UI_USE_BATCH 1