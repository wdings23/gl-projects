#include <glut.h>
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

#define WINDOW_WIDTH 1920//640
#define WINDOW_HEIGHT 1080//960

static bool sbZooming = false;
static bool sbTilt = false;
static float sfLastY = -1.0f;

/*
**
*/
int main(int argc, char* argv[])
{
	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA );
	glutInitWindowPosition( 100, 100 );
	glutInitWindowSize( WINDOW_WIDTH, WINDOW_HEIGHT );
	glutCreateWindow("Discus");
	
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

#if defined( _WIN32 )
	wchar_t buffer[MAX_PATH];
    GetModuleFileName( NULL, buffer, MAX_PATH );

	wchar_t* pStartWindows = wcsstr( buffer, L"windows" );
	wchar_t dir[MAX_PATH];
	memset( dir, 0, sizeof( dir ) );
	memcpy( dir, buffer, (unsigned long)pStartWindows - (unsigned long)buffer );

	char szBuffer[MAX_PATH];
	wcstombs( szBuffer, dir, sizeof( szDir ) );
	snprintf( szDir, sizeof( szDir ), "%sassets", szBuffer );

#endif // _WIN32

	char szWriteDir[256];
	snprintf( szWriteDir, sizeof( szWriteDir ), "%s\\save", szDir );

	setFileDirectories( szDir, szWriteDir);

	renderSetScreenWidth( WINDOW_WIDTH );
	renderSetScreenHeight( WINDOW_HEIGHT );
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
	double fElapsed = fTime - sfLastTime;
	float fDT = (float)( fElapsed * 0.001 );
	sfLastTime = fTime;	

	glClearColor( 0.6f, 0.6f, 0.6f, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	CGame::instance()->update( fDT );
	CGame::instance()->draw();

	glutSwapBuffers();
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
	gluPerspective( 45.0f, WINDOW_WIDTH / WINDOW_HEIGHT, 1.0f, 100.0f );
	glViewport( 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT );
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
		if( iState == GLUT_DOWN )
		{
			sfLastY = -1.0f;
			sbZooming = true;
		}
		else if( iState == GLUT_UP )
		{
			sfLastY = -1.0f;
			sbZooming = false;
		}
	}
	else if( iButton == GLUT_MIDDLE_BUTTON )
	{
		if( iState == GLUT_DOWN )
		{
			sfLastY = -1.0f;
			sbTilt = true;
		}
		else if( iState == GLUT_UP )
		{
			sfLastY = -1.0f;
			sbTilt = false;
		}
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

		case 'f':
			{
				glutGameModeString("1920x1080:32");
				glutEnterGameMode();
			}
			break;

		case 'F':
			{
				glutReshapeWindow( WINDOW_WIDTH, WINDOW_HEIGHT );
			}
			break;

		case 'v':
			{
				CGame::instance()->toggleVRView();
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
	
	if( sbZooming )
	{
		if( sfLastY < 0.0f )
		{
			sfLastY = fY;
		}
		
		CGame::instance()->zoomCamera( ( sfLastY - fY ) * 0.01f );
	}
	else if( sbTilt )
	{
		if( sfLastY < 0.0f )
		{
			sfLastY = fY;
		}

		float fDY = sfLastY - fY;
		CGame::instance()->tiltCamera( fDY );
	}
	else
	{
		CGame::instance()->inputUpdate( fX, fY, TOUCHTYPE_MOVED );
	}

	sfLastY = fY;
}

/*
**
*/
static void update( void )
{
	static double sfLastTime = 0.0;
    
    double fTime = getTime();
    if( sfLastTime == 0.0 )
    {
        sfLastTime = fTime;
    }
    float fElapsed = (float)( fTime - sfLastTime ) * 0.001f;
    sfLastTime = fTime;
    
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

