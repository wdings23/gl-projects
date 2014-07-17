//
//  main.cpp
//  worldviewer_MacOS
//
//  Created by Dingwings on 5/8/14.
//  Copyright (c) 2014 Dingwings. All rights reserved.
//

#include <GLUT/glut.h>

#include "timeutil.h"

#include "filepathutil.h"
#include "render.h"
#include "game.h"

#include "hud.h"

static void release( void );
static void render( void );
static void initGL( char* argv[] );
static void changeSize( int iW, int iH );
static void mouseInput( int iButton, int iState, int iX, int iY );
static void mouseMotionInput( int iX, int iY );
static void keyboardInput( unsigned char cKey, int iX, int iY );
static void specialKBInput( int iKey, int iX, int iY );

static void update( void );
static void renderScene( void );

#define WINDOW_WIDTH 640
#define WINDOWN_HEIGHT 960

/*
**
*/
int main(int argc, char* argv[])
{
	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA );
	glutInitWindowPosition( 100, 100 );
	glutInitWindowSize( WINDOW_WIDTH, WINDOWN_HEIGHT );
	glutCreateWindow("World Viewer");
	
	glutDisplayFunc( render );
	glutIdleFunc( render );
	glutReshapeFunc( changeSize );
	glutMouseFunc( mouseInput );
	glutMotionFunc( mouseMotionInput );
	glutKeyboardFunc( keyboardInput );
	glutSpecialFunc( specialKBInput );
    
	initGL( argv );
	glutMainLoop();
	
	release();
    
	return 0;
}


/*
**
*/
static void initGL( char* argv[] )
{
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_TEXTURE_2D );
	
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
    
	glDepthMask( GL_TRUE );
    
	glEnableClientState( GL_COLOR_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_INDEX_ARRAY );
	
	char szDir[256];
	memset( szDir, 0, sizeof( szDir ) );
	
	char szFileName[256];
	memset( szFileName, 0, sizeof( szFileName ) );
    
	int iSize = (int)strlen( argv[1] );
	for( int i = iSize - 1; i >= 0; i-- )
	{
		if( argv[1][i] == '/' ||
           argv[1][i] == '\\' )
		{
			memcpy( szDir, argv[1], i );
			
			size_t iStringSize = iSize - i;
			memcpy( szFileName, &argv[1][i+1], iStringSize );
			break;
		}
	}
    
    char szWriteDir[256];
    snprintf( szWriteDir, sizeof( szWriteDir ), "%s\\write", szDir );
    
	setFileDirectories( szDir,
                        szWriteDir );
    
	renderSetScreenWidth( WINDOW_WIDTH );
	renderSetScreenHeight( WINDOWN_HEIGHT );
	renderSetScreenScale( 1.0f );
    
	CGame::instance()->init();
	CGame::instance()->setSceneFileName( szFileName );
}

/*
**
*/
static double sfLastTime = 0.0;
static void render( void )
{
    double fTime = getTime();
    if( sfLastTime == 0.0 )
    {
        sfLastTime = fTime;
    }
    
    float fElapsed = (float)( fTime - sfLastTime ) * 0.001f;

	glClearColor( 0.6f, 0.6f, 0.6f, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	CGame::instance()->update( fElapsed );
	CGame::instance()->draw();
    
	glutSwapBuffers();
    
    sfLastTime = getTime();
}

/*
**
*/
static void release( void )
{
}

/*
**
*/
static void changeSize( int iW, int iH )
{
	gluPerspective( 45.0f, SCREEN_WIDTH / SCREEN_HEIGHT, 1.0f, 100.0f );
	glViewport( 0, 0, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2 );
}

/*
**
*/
static void mouseInput( int iButton, int iState, int iX, int iY )
{
	int iTouchType = TOUCHTYPE_BEGAN;
	if( iButton == GLUT_LEFT_BUTTON )
	{
		if( iState == GLUT_DOWN )
		{
			iTouchType = TOUCHTYPE_BEGAN;
		}
		else if( iState == GLUT_UP )
		{
			iTouchType = TOUCHTYPE_ENDED;
		}
		
		float fX = (float)iX;
		float fY = (float)iY;
	}
	else if( iButton == GLUT_RIGHT_BUTTON )
	{
		
	}
    
    float fX = (float)iX;
    float fY = (float)iY;
    
	CGame::instance()->inputUpdate( fX, fY, iTouchType );
    CHUD::instance()->inputUpdate( fX, fY, 0, 1, iTouchType );
}

/*
**
*/
static void keyboardInput( unsigned char cKey, int iX, int iY )
{
	
    
	switch( cKey )
	{
		case 'a':
        {
			
        }
			break;
		case 'd':
        {
            
        }
			break;
            
		case 'w':
        {
            
        }
			break;
            
		case 's':
        {
            
        }
			break;
            
		default:
			break;
            
	}
    
	
}

/*
**
*/
static void specialKBInput( int iKey, int iX, int iY )
{
	switch( iKey )
	{
		case GLUT_KEY_LEFT:
			break;
            
		case GLUT_KEY_RIGHT:
			break;
            
		case GLUT_KEY_UP:
			break;
            
		case GLUT_KEY_DOWN:
			break;
	}
}

/*
**
*/
static void mouseMotionInput( int iX, int iY )
{
	float fX = (float)iX;
	float fY = (float)iY;
	
	CGame::instance()->inputUpdate( fX, fY, TOUCHTYPE_MOVED );
    CHUD::instance()->inputUpdate( fX, fY, 0, 1, TOUCHTYPE_MOVED );
}

/*
**
*/
static void update( void )
{
    
    //OUTPUT( "elapsed time = %f\n", fElapsed );
}

/*
**
*/
static void renderScene( void )
{
	glClearColor(0.65f, 0.65f, 0.65f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    
    
}

