#include "game.h"
#include "render.h"
#include "jobmanager.h"

/*
**
*/
CGame* CGame::mpInstance = NULL;
CGame* CGame::instance( void )
{
	if( mpInstance == NULL )
	{
		mpInstance = new CGame();
	}

	return mpInstance;
}

/*
**
*/
CGame::CGame( void ) :
	mbVRView(false)
{
}

/*
**
*/
CGame::~CGame( void )
{
	
}

/*
**
*/
void CGame::init( void )
{
	levelInit( &mLevel );

	miScreenWidth = renderGetScreenWidth();
	miScreenHeight = renderGetScreenHeight();

	tVector4 pos = { 0.0f, 0.0f, -12.0f, 1.0f };
	tVector4 lookAt = { 0.0f, 0.0f, 100.0f, 1.0f };
	tVector4 up = { 0.0f, 1.0f, 0.0f, 1.0f };

	mCamera.setPosition( &pos );
	mCamera.setLookAt( &lookAt );
	mCamera.setFar( 100.0f );
	mCamera.setUp( &up );

	mGameRender.setCamera( &mCamera );
	mGameRender.setLevel( &mLevel );
	mGameRender.init();

	jobManagerInit( gpJobManager );

	mLastTouch.fX = mLastTouch.fY = -1.0f;
}

/*
**
*/
void CGame::update( float fDT )
{
    jobManagerUpdate( gpJobManager );
    
	mCamera.update( miScreenWidth, miScreenHeight );
	mGameRender.updateLevel();

	mfDT = fDT;
}

/*
**
*/
void CGame::draw( void )
{
	mGameRender.draw( mfDT );
}

/*
**
*/
void CGame::inputUpdate( float fX, float fY, int iType )
{
	if( iType == TOUCHTYPE_BEGAN || iType == TOUCHTYPE_ENDED )
	{
		mLastTouch.fX = fX;
		mLastTouch.fY = fY;
	}

    if( mLastTouch.fX < 0.0f || mLastTouch.fY < 0.0f )
    {
        mLastTouch.fX = fX;
        mLastTouch.fY = fY;
    }
    
	tVector4 diff = 
	{
		fX - mLastTouch.fX,
		fY - mLastTouch.fY
	};

	tVector4 const* pCamPos = mCamera.getPosition();
	tVector4 const* pCamLookAt = mCamera.getLookAt();

	tVector4 newCamPos = 
	{
		pCamPos->fX + diff.fX * 0.01f,
		pCamPos->fY + diff.fY * 0.01f,
		pCamPos->fZ + diff.fZ * 0.01f,
		1.0f
	};

	tVector4 newCamLookAt = 
	{
		pCamLookAt->fX + diff.fX * 0.01f,
		pCamLookAt->fY + diff.fY * 0.01f,
		pCamLookAt->fZ + diff.fZ * 0.01f,
		1.0f
	};
	
	mCamera.setPosition( &newCamPos );
	mCamera.setLookAt( &newCamLookAt );

	mLastTouch.fX = fX;
	mLastTouch.fY = fY;
}

/*
**
*/
void CGame::setSceneFileName( const char* szSceneFileName )
{
	strncpy( mszSceneFileName, szSceneFileName, sizeof( mszSceneFileName ) );
	mGameRender.setSceneFileName( mszSceneFileName );
}

/*
**
*/
void CGame::toggleShader( void )
{
	mGameRender.toggleShader();
}

/*
**
*/
void CGame::zoomCamera( float fDPos )
{
	tVector4 const* pPos = mCamera.getPosition();
	tVector4 const* pLookAt = mCamera.getLookAt();

	tVector4 newPos, newLookAt;
	memcpy( &newPos, pPos, sizeof( tVector4 ) );
	memcpy( &newLookAt, pLookAt, sizeof( tVector4 ) );

	newPos.fZ += fDPos;
	newLookAt.fZ += fDPos;

	mCamera.setPosition( &newPos );
	mCamera.setLookAt( &newLookAt );
}

/*
**
*/
void CGame::tiltCamera( float fDPos )
{
	tVector4 const* pPos = mCamera.getPosition();
	tVector4 const* pLookAt = mCamera.getLookAt();

	tVector4 newPos, newLookAt;
	memcpy( &newPos, pPos, sizeof( tVector4 ) );
	memcpy( &newLookAt, pLookAt, sizeof( tVector4 ) );

	newLookAt.fY += fDPos;

	mCamera.setPosition( &newPos );
	mCamera.setLookAt( &newLookAt );
}