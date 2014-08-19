#ifndef __GAMERENDER_H__
#define __GAMERENDER_H__

#include "modelbatchmanager.h"
#include "octgrid.h"
#include "level.h"
#include "font.h"

struct LightInfo
{
	tVector4		mPosition;
	tVector4		mXFormPosition;
	tVector4		mColor;
	float			mfSize;
	float			mfAngle;
	float			mfAngleInc;
};

typedef struct LightInfo tLightInfo;

class CGame;

class CGameRender
{
public:
	CGameRender( void );
	virtual ~CGameRender( void );

	void init( void );
	void draw( float fDT );
	
	void updateLevel( void );
	
	inline void setLevel( tLevel* pLevel ) { mpLevel = pLevel; }
	inline void setCamera( CCamera const* pCamera ) { mpCamera = pCamera; }
	inline void setSceneFileName( const char* szSceneFileName ) { mszSceneFileName = szSceneFileName; }
	inline void setVRView( bool bVRView ) { mbVRView = bVRView; }

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

	void createSphere( void );
	void initInstancing( void );

	void drawInstances( CCamera const* pCamera, GLuint iFBO, GLuint iShader );
	void drawLightModel( CCamera const* pCamera, int iEye );
	void drawDeferredScene( int iEye );
	void drawScene( void );

	GLuint miLeftDeferredFBO;
	GLuint miLeftPositionTexture;
	GLuint miLeftAlbedoTexture;
	GLuint miLeftDepthTexture;
	GLuint miLeftNormalTexture;
	GLuint miLeftStencilBuffer;

	GLuint miLeftLightFBO; 
	GLuint miLeftLightTexture;
	GLuint miLeftLightDepthTexture;
	GLuint miLeftLightStencilBuffer;

	GLuint miLeftAmbientOcclusionFBO;
	GLuint miLeftAmbientOcclusionTexture;

	GLuint miLeftFinalFBO;
	GLuint miLeftFinalTexture;

	GLuint miRightDeferredFBO;
	GLuint miRightPositionTexture;
	GLuint miRightAlbedoTexture;
	GLuint miRightDepthTexture;
	GLuint miRightNormalTexture;
	GLuint miRightStencilBuffer;

	GLuint miRightLightFBO; 
	GLuint miRightLightTexture;
	GLuint miRightLightDepthTexture;
	GLuint miRightLightStencilBuffer;

	GLuint miRightAmbientOcclusionFBO;
	GLuint miRightAmbientOcclusionTexture;

	GLuint miRightFinalFBO;
	GLuint miRightFinalTexture;

	GLuint miInstanceVBO;
	GLuint miInstanceInfoVBO;

	GLuint miCubeIndexBuffer;

	GLuint miSphereBuffer;
	GLuint miSphererIndexBuffer;

	int miNumSphereTris;
	int miNumCubes;

	tLightInfo* maLightInfo;
	int miNumLights;

	bool	mbVRView;
};

#endif // __GAMERENDER_H__