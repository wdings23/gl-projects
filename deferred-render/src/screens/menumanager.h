#ifndef __MENUMANAGER_H__
#define __MENUMANAGER_H__

#include "layer.h"
#include "menuscreen.h"

class CMenuManager
{
public:
	CMenuManager( void );
	~CMenuManager( void );
	
	void loadMenu( const char* szMenu, const char* szFileName );
	inline CLayer* getLayer( void ) { return mpLayer; }
	void releaseLayer( void );
    inline void setCurrScreen( CMenuScreen* pMenuScreen ) { mpCurrScreen = pMenuScreen; }
    
    void drawScreen( float fDT );
    
    void setNextMenuFileName( const char* szFileName );
    
    inline void setFinishedAllAnimation( bool bFinishedAnimation ) { mbFinishedAnimation = bFinishedAnimation; }
    inline bool hasFinishedAllAnimations( void ) { return mbFinishedAnimation; }
    
    void restart( const char* szStartMenu );
    void exitMenu( void );
    
    void registerScreen( CMenuScreen* pScreen );
    void releaseScreen( void );
    
    bool isInExitState( void );
    
    void showPopup( void );
    
    void setAnimPlayerToControl( const char* szFileName, CLayer* pLayer, int iType );
    
    inline void setInputEnabled( bool bEnabled ) { mbInputEnabled = bEnabled; }
    inline bool getInputEnabled( void ) { return mbInputEnabled; }
    
    inline void setUpdateFunc( void (*pfnUpdate)( void ) ) { mpfnUpdate = pfnUpdate; }
    
    void loadCurrScreen( const char* szMenu, const char* szFileName );
    
    CControl* inputUpdate( float fCursorX,
						   float fCursorY,
						   int iHit,
						   int iNumTouches,
						   int iTouchType );
    
protected:
        
    static void checkExitedMenu( CControl* pControl, CLayer* pLayer, void* pUserData );
    static void exitStateForControls( CControl* pControl, CLayer* pLayer, void* pUserData );
    static void setAppropriateNormalState( CControl* pControl, CLayer* pLayer, void* pUserData );
    static void setButtonGotoScreen( CControl* pControl, CLayer* pLayer, void* pUserData );
    static void gotoNextScreen( CControl* pControl, void* pUserData );
    static void checkExitState( CControl* pControl, CLayer* pLayer, void* pUserData );
    
protected:
	CLayer*						mpLayer;
    CMenuScreen*                mpCurrScreen;
    bool                        mbFinishedAnimation;
    
    char                        mszNextMenuFileName[256];
    
    std::vector<CMenuScreen *>  mapScreens;
    
    CLayer                      mPopupLayer;
    
    bool                        mbInputEnabled;
    
    void (*mpfnUpdate)( void );
    
public:
	static CMenuManager*				instance( void );

protected:
	static CMenuManager*				mpInstance;
    
};

#endif // __MENUMANAGER_H__