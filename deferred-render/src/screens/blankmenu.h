//
//  blankmenu.h
//  Game1
//
//  Created by Dingwings on 1/16/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef __BLANKMENU_H__
#define __BLANKMENU_H__

#include "menuscreen.h"

class CBlankMenu : public CMenuScreen
{
public:
    CBlankMenu( void );
    ~CBlankMenu( void );
    
    void enter( void );
    void update( float fDT );
    void exit( void );

public:
    static CBlankMenu* instance( void );
    
protected:
    static CBlankMenu* mpInstance;

    
};
#endif  // __BLANKMENU_H__
