//
//  box.h
//  Game3
//
//  Created by Dingwings on 8/14/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef __BOX_H__
#define __BOX_H__

#include "control.h"

enum
{
	BOX_TOP_LEFT_CORNER = 0,
	BOX_TOP_RIGHT_CORNER,
	BOX_BOTTOM_LEFT_CORNER,
	BOX_BOTTOM_RIGHT_CORNER,

	NUM_BOX_CORNERS,
};

class CBox : public CControl
{
public:
    CBox( void );
    virtual ~CBox( void );
    
    virtual void draw( double fDT, int iScreenWidth, int iScreenHeight );
    virtual void setColor( float fRed, float fGreen, float fBlue, float fAlpha );
    
	void setCornerColor( int iCorner, tVector4 const* pColor );
	inline tVector4 const* getCornerColor( int iCorner ) { return &maCornerColors[iCorner]; }

    virtual CControl* copy( void );
    
protected:
    tVector4    mOrigColor;

	tVector4	maCornerColors[NUM_BOX_CORNERS];
};

#endif // __BOX_H__
