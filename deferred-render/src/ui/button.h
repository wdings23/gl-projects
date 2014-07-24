#ifndef __BUTTON_H__
#define __BUTTON_H__

#include "sprite.h"
#include "text.h"

typedef void (*pressedFunc)( CControl* pControl, void* pUserData );

class CButton : public CControl
{
public:
	CButton( void );
	virtual ~CButton( void );
	
	virtual void draw( double fTime, int iScreenWidth, int iScreenHeight );
	virtual CControl* inputUpdate( float fCursorX,
							 float fCursorY,
							 int iNumTouches,
							 int iTouchType,
                             CControl* pLastInteract,
							 CControl* pParent = NULL );

	inline void setPressedFunc( pressedFunc pfnFunc, void* pUserData = NULL ) { mpPressFunc = pfnFunc; mpUserData = pUserData; }
    void setNextScreen( const char* szScreen );
    inline const char* getNextScreen( void ) { return mszNextScreen; }
    
    inline void* getUserData( void ) { return mpUserData; }
    
    virtual CButton* copy( void );
    
protected:
	int intersect( float fX, float fY );
    static void animFinished( void* pData );
    
protected:
	pressedFunc	mpPressFunc;
    void*       mpUserData;
    
    char        mszNextScreen[256];
    tVector2    mTouchDownPos;
};

#endif // __BUTTON_H__