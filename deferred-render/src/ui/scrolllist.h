//
//  scrolllist.h
//  Game1
//
//  Created by Dingwings on 2/9/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef __SCROLLLIST_H__
#define __SCROLLLIST_H__

#include "control.h"

class CScrollList : public CControl
{
public:
    enum
    {
        TYPE_VERTICAL = 0,
        TYPE_HORIZONTAL,
        
        NUM_TYPES,
    };

public:
    CScrollList( void );
    virtual ~CScrollList( void );
    
    virtual CControl* inputUpdate( float fCursorX,
                            float fCursorY,
                            int iNumTouches,
                            int iTouchType,
                            CControl* pLastInteract,
                            CControl* pParent = NULL );
    
    void draw( double fDT, int iScreenWidth, int iScreenHeight );
    void setTemplate( CControl* pControl );
    
    void setFillDataFunc( void (*pfnFillData)( CControl* pControl, int iIndex ), void* pUserData );
    
    virtual void updatePositions( void );
    void setNumEntries( int iNumEntries );
    
    inline int getTop( void ) { return miTop; }
    void refresh( void );
    void startScrollBack( void );
    
    inline void setTop( int iTop ) { miTop = iTop; }
    inline void setWrap( bool bWrap ) { mbWrap = bWrap; }
    inline void setVelocityDecay( float fDecay ) { mfVelocityDecay = fDecay; }
    inline void setVelocity( float fX, float fY ) { mVelocity.fX = fX; mVelocity.fY = fY; }
    inline void setAtStartFunc( void (*pfnAtStart)( int iIndex ) ) { mpfnAtStart = pfnAtStart; }
    
    inline bool getWrap( void ) { return mbWrap; }
    inline float getVelocityDecay( void ) { return mfVelocityDecay; }
    inline tVector2 const* getVelocity( void ) { return &mVelocity; }
    
protected:
    int                 miDisplayType;
    int                 miMaxEntries;
    int                 miNumEntries;
    CControl*           mpTemplate;
    
    tVector2            mVelocity;
    tVector2            mPositionOffset;
    
    tVector2            mScrollBackVelocity;
    double              mfStartScrollBackTime;
    
    int                 miTouchType;
    tVector2            mLastTouchPt;
    int                 miTop;
    int                 miStartList;
    double              mfLastTime;
    
    bool                mbWrap;
    float               mfVelocityDecay;
    
    tVector2            mStartTouchPt;
    
    void*               mpUserData;
    
    void                (*mpfnFillData)( CControl* pControl, int iIndex );
    void                (*mpfnAtStart)( int iIndex );
    
protected:
    void updateChildren( int iOffset );
    
};

#endif  // __SCROLLLIST_H__
