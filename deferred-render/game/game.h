#ifndef __GAME_H__
#define __GAME_H__
#include "camera.h"
#include "gamerender.h"
#include "level.h"

enum
{
	TOUCHTYPE_BEGAN = 0,
	TOUCHTYPE_MOVED,
	TOUCHTYPE_ENDED,

	NUM_TOUCHTYPES,
};

class CGame
{
public:
	enum
	{
		STATE_LOADING = 0,
		STATE_INGAME,

		NUM_STATES,
	};

public:
	CGame( void );
	~CGame( void );
	
	void init( void );

	void update(  float fDT );
	void draw( void );

	void inputUpdate( float fX, float fY, int iType );
	
	void setSceneFileName( const char* szSceneFileName );
	void toggleShader( void );

	void zoomCamera( float fDPos );

    inline void clearInputPos( void )   { mLastTouch.fX = mLastTouch.fY = -1.0f; }
    
protected:
	int						miPrevState;
	int						miState;

	CGameRender				mGameRender;
	CCamera					mCamera;

	int						miScreenWidth;
	int						miScreenHeight;

	tLevel					mLevel;
	float					mfDT;

	char					mszSceneFileName[256];

	tVector2				mLastTouch;

public:
	static CGame*					instance( void );

protected:
	static CGame*					mpInstance;

};

#endif // __GAME_H__