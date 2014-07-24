#ifndef __CLAYER_H__
#define __CLAYER_H__

#include "control.h"
#include "sprite.h"
#include "text.h"
#include "button.h"
#include "scrolllist.h"
#include "box.h"
#include "Particle.h"

#include "tinyxml.h"

#define MAX_LAYER_EMITTERS 10

struct EmitterInfo
{
    bool    mbStarted;
    float   mfStartTime;
};

typedef struct EmitterInfo tEmitterInfo;

class CLayer
{
public:
	CLayer( void );
	virtual ~CLayer( void );

	void init( void );
	void release( void );

	void load( const char* szFileName, float fScreenWidth = SCREEN_WIDTH );
	void addControl( CControl* pControl );
	virtual void draw( unsigned int iShaderProgram, double fTime, int iScreenWidth, int iScreenHeight );
	virtual CControl* inputUpdate( float fCursorX, 
					 float fCursorY, 
					 int iHit, 
					 int iNumTouches,
					 int iTouchType );
	CControl* findControl( const char* szName, CControl* pParent = NULL );
	void traverseControls( void (*pfnFunc)( CControl* pControl, CLayer* pLayer, void* pUserData ), 
                           CControl* pParent = NULL, 
                           void* pUserData = NULL );

	inline void setName( const char* szName ) { strcpy( mszName, szName ); }
	inline const char* getName( void ) { return mszName; }

    inline void setAnimationFinished( bool bFinished ) { mbFinishedAnimation = bFinished; }
    
    CControl* getControl( int iIndex );
    int getNumControls( void );
    
    void findControls( const char* szName, std::vector<CControl *> *pList, CControl* pParent );
    inline void setState( int iState ) { miState = iState; }
    inline int getState( void ) { return miState; }
    
    void addEmitter( const char* szName, int iState, tVector2 const* pPosition, float fStartTime );
    
    void startEmitters( int iState );
    
    inline void setAnimType( int iType ) { miAnimType = iType; }
    inline void markStartStateTime( void ) { mfStartStateTime = mfTotalTime; }
    
    void setOffset( float fX, float fY );
    
    inline void setScale( float fScaleX, float fScaleY ) { mScale.fX = fScaleX, mScale.fY = fScaleY; }

protected:
	CControl**		mapControls;
	int				miNumControls;
	int				miNumControlsAlloc;
	
	char			mszName[256];
    bool            mbFinishedAnimation;
    
    int             miState;
    int             miAnimType;
    
    int             maiNumEmitters[CMenuAnimPlayer::TYPE_USER_0];
    CEmitter        maEmitters[CMenuAnimPlayer::TYPE_USER_0][MAX_LAYER_EMITTERS];
    tEmitterInfo    maEmitterInfo[CMenuAnimPlayer::TYPE_USER_0][MAX_LAYER_EMITTERS];
    
    float           mfStartStateTime;
    float           mfTotalTime;
    
    tVector2        mScale;
    
    CControl*       mpLastInteractControl;  
    
protected:
	CSprite* handleSprite( TiXmlElement* pNode );
	CText* handleText( TiXmlElement* pNode, float fScreenWidth );
	CButton* handleButton( CControl* pParent, TiXmlNode* pNode, float fScreenWidth );
	
	void handleTexture( TiXmlElement* pNode );
	CControl* handleGroup( TiXmlNode* pNode, CControl* pParent );

	CControl* handleControl( CControl* pParent, TiXmlNode* pNode, float fScreenWidth );
    
    CScrollList* handleScrollList( CControl* pParent, TiXmlNode* pNode, float fScreenWidth );
    
    CBox* handleBox( TiXmlElement* pNode );
    
	bool checkControlName( const char* szName, const char* szToFind );
    
protected:
    static void setNormalState( CControl* pControl, CLayer* pLayer, void* pUserData );
    static void checkNormalState( CControl* pControl, CLayer* pLayer, void* pUserData );
    static void checkFocusState( CControl* pControl, CLayer* pLayer, void* pUserData );
    static void checkExitState( CControl* pControl, CLayer* pLayer, void* pUserData );
    
    static void setShader( CControl* pControl, CLayer* pLayer, void* pUserData );
    
    static void checkDuplicateNames( CControl* pControl, CLayer* pLayer, void* pUserData );
    static void getControlName( CControl* pControl, CLayer* pLayer, void* pUserData );
};

#endif // __CLAYER_H__