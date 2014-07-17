//
//  controller.cpp
//  CityBenchmark
//
//  Created by Dingwings on 7/25/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "controller.h"

#include "vector.h"
#include "camera.h"
#include "render.h"

class CController
{
    enum
    {
        STATE_NONE = 0,
        STATE_MOVING,
        STATE_PANNING,
        
        NUM_STATES,
    };
    
public:
    CController( void );
    ~CController( void );
    
    void touchBegan( double fX, double fY, int iNumTouches );
    void touchMoved( double fX, double fY, int iNumTouches );
    void touchEnded( double fX, double fY, int iNumTouches );
    
    void update( void );
    
public:
    static CController* instance( void );
    
protected:
    static CController* mpInstance;
    
protected:
    tVector2        mBeginTouch;
    int             miState;
    
    tVector2        mVelocity;
};

/*
**
*/
CController* CController::mpInstance = NULL;
CController* CController::instance( void )
{
    if( mpInstance == NULL )
    {
        mpInstance = new CController();
    }
    
    return mpInstance;
}

/*
**
*/
CController::CController( void )
{
    mBeginTouch.fX = 0.0f;
    mBeginTouch.fY = 0.0f;
    
    mVelocity.fX = 0.0f;
    mVelocity.fY = 0.0f;
    
    miState = STATE_NONE;
}

/*
**
*/
CController::~CController( void )
{
    
}

/*
**
*/
void CController::touchBegan( double fX, double fY, int iNumTouches )
{
    mBeginTouch.fX = fX;
    mBeginTouch.fY = fY;
    
    if( iNumTouches > 1 )
    {
        miState = STATE_PANNING;
    }
    else
    {
        miState = STATE_MOVING;
    }
}

/*
**
*/
void CController::touchMoved( double fX, double fY, int iNumTouches )
{
    float fDiffX = fabs( fX - mBeginTouch.fX );
    float fDiffY = fabs( fY - mBeginTouch.fY );
    
    mVelocity.fX = 0.0f;
    mVelocity.fY = 0.0f;
    
    if( fDiffX > fDiffY )
    {
        if( fX < mBeginTouch.fX )
        {
            mVelocity.fX = 0.1f;
        }
        else if( fX > mBeginTouch.fX )
        {
            mVelocity.fX = -0.1f; 
        }
    }
    else
    {
        if( fY < mBeginTouch.fY )
        {
            mVelocity.fY = -0.1f;
        }
        else if( fY > mBeginTouch.fY )
        {
            mVelocity.fY = 0.1f;
        }
    }
    
    if( iNumTouches > 1 )
    {
        miState = STATE_PANNING;
    }
    else
    {
        miState = STATE_MOVING;
    }
}

/*
**
*/
void CController::touchEnded( double fX, double fY, int iNumTouches )
{
    mBeginTouch.fX = -1.0;
    mBeginTouch.fY = -1.0;
    
    mVelocity.fX = 0.0f;
    mVelocity.fY = 0.0f;
}

/*
**
*/
void CController::update( void )
{
    
}

/*
**
*/
void controllerTouchBegan( double fX, double fY, int iNumTouches )
{
    CController::instance()->touchBegan( fX, fY, iNumTouches );
}

/*
**
*/
void controllerTouchMoved( double fX, double fY, int iNumTouches )
{
    CController::instance()->touchMoved( fX, fY, iNumTouches );
}

/*
**
*/
void controllerTouchEnded( double fX, double fY, int iNumTouches )
{
    CController::instance()->touchEnded( (float)fX, (float)fY, iNumTouches );
}

/*
**
*/
void controllerUpdate( void )
{
    CController::instance()->update();
}
