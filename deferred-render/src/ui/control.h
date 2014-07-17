#ifndef __CCONTROL_H__
#define __CCONTROL_H__

#include <vector>
#include "vector.h"
#include "matrix.h"
#include "menuanimplayer.h"

#define BUTTON_TOUCHRANGE_X 10.0f
#define BUTTON_TOUCHRANGE_Y 20.0f

class CControl
{
public:
	enum
	{
		STATE_NORMAL = 0,
		STATE_DISABLED,
        STATE_INPUT_DISABLED,
        
		NUM_STATES,
	};
    
    enum
    {
        TYPE_CONTROL = 0,
        TYPE_SPRITE,
        TYPE_TEXT,
        TYPE_BUTTON,
        TYPE_SCROLLLIST,
        
        NUM_TYPES
    };

public:
	CControl( void );
	virtual ~CControl( void );
	
	void setPosition( const tVector2& position );
	void setDimension( const tVector2& dimension );
	
    void setOrigPosition( const tVector2& position );
    
	inline tVector2 const& getPosition( void ) { return mPosition; }
	inline tVector2 const& getDimension( void ) { return mDimension; }
    
    void getBounds( tVector2* pBounds );
    
	void setName( const char* szName );
	inline const char* getName( void ) { return mszName; }
	
	virtual void draw( double fDT, int iScreenWidth, int iScreenHeight );
	virtual CControl* inputUpdate( float fCursorX, 
							 float fCursorY, 
							 int iNumTouches,
							 int iTouchType,
                             CControl* pLastInteract,
							 CControl* pParent = NULL );
	
	void setShader( unsigned int iShader );
	
	void addChild( CControl* pControl );
	
	inline void setParent( CControl* pParent ) { mpParent = pParent; }
	inline tMatrix44 const& getMatrix( void ) { return mTotalMatrix; }
	inline void setAnimPlayer( CMenuAnimPlayer* pAnimPlayer ) { mpAnimPlayer = pAnimPlayer; }
	inline CMenuAnimPlayer* getAnimPlayer( void ) { return mpAnimPlayer; }

	void setAlpha( float fAlpha );

	void reverseChildren( void );
	
	inline unsigned int getNumChildren( void ) { return (unsigned int)mapChildren.size(); }
	inline CControl* getChild( int iIndex ) { return mapChildren[iIndex]; }
	
	void setState( int iState );
    
    inline tVector4 const& getAnchorPoint( void ) { return mAnchorPt; }
    
    void setAnimState( int iState );
    
    inline int getType( void ) { return miType; }
    virtual inline void setColor( float fRed, float fGreen, float fBlue, float fAlpha ) { mColor.fX = fRed; mColor.fY = fGreen; mColor.fZ = fBlue; mColor.fW = fAlpha; }
    
    virtual CControl* copy( void );
    
    void copyControl( CControl* pDestination );
    
    CControl* find( const char* szName );
    
    inline CControl* getParent( void ) { return mpParent; }
    inline int getState( void ) { return miState; }
    
    tVector2& getScreenPos( void );
    void setScreenPos( tVector2& screenPos ); 
    
    void screenToLocal( tVector2* pScreenPos, tVector2* pLocalPos );
    
    void traverse( CControl* pControl, void (*pFunc)( CControl* pControl, void* pUserData ), void* pTraverseData );
    
    inline void setOffset( float fX, float fY ) { mOffset.fX = fX; mOffset.fY = fY; }
	inline tMatrix44 const* getTotalMatrix( void ) { return &mTotalMatrix; }
	
	tVector2 getAbsolutePosition( void );

protected:
	tVector2		mPosition;
	tVector2		mDimension;

    tVector2        mOrigPosition;
    
    tVector2        mScreenPos;
    
	char			mszName[128];
	tVector2		maUV[4];

	tVector4		mColor;
    int             miType;
    
	unsigned int miShaderProgram;

	std::vector<CControl *>		mapChildren;

	CControl*					mpParent;
	tMatrix44					mTotalMatrix;

	tVector4					mAnchorPt;
	CMenuAnimPlayer*			mpAnimPlayer;

	int							miState;
    
    int                         miShaderPos;
    int                         miShaderColor;
    int                         miShaderUV;
    
    void*                       mpTraverseData;
    
    tVector2                    mOffset;
protected:
    void updateMatrix( double fDT );
    
};

#endif // __CCONTROL_H__