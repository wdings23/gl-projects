#ifndef __GAMERENDER_H__
#define __GAMERENDER_H__

#include "modelbatchmanager.h"
#include "octgrid.h"
#include "level.h"
#include "font.h"

class CGame;

class CGameRender
{
public:
	CGameRender( void );
	~CGameRender( void );

	void init( void );
	void draw( float fDT );
	
	void updateLevel( void );
	
	inline void setLevel( tLevel* pLevel ) { mpLevel = pLevel; }
	inline void setCamera( CCamera const* pCamera ) { mpCamera = pCamera; }
	inline void setSceneFileName( const char* szSceneFileName ) { mszSceneFileName = szSceneFileName; }
	
	void toggleShader( void );

protected:
	CCamera const*			mpCamera;
	
	tLevel*					mpLevel;
	char const*				mszSceneFileName;

	tModelBatchManager		mBatchManager;

	tVisibleOctNodes		mVisibleOctNodes;
	tVisibleModels			mVisibleModels;
    
    CFont                   mFont;
	int						miShaderIndex;
	
	tShaderProgram const*	mpCurrShaderProgram;

protected:
	void getVisibleModels( CCamera const* pCamera, 
						   tVisibleOctNodes const* pVisibleNodes,
						   tVisibleModels* pVisibleModels );

	static void loadLevelJob( void* pData );
	static void setAttribs( GLuint iShader );
	static void* allocObject( const char* szName, 
							  void* pUserData0, 
							  void* pUserData1, 
							  void* pUserData2, 
							  void* pUserData3 );

};

#endif // __GAMERENDER_H__