//
//  blankmenu.cpp
//  Game1
//
//  Created by Dingwings on 1/16/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "blankmenu.h"
#include "menumanager.h"

/*
**
*/
CBlankMenu* CBlankMenu::mpInstance = NULL;
CBlankMenu* CBlankMenu::instance( void )
{
    if( mpInstance == NULL )
    {
        mpInstance = new CBlankMenu();
    }
    
    return mpInstance;
}

/*
 **
 */
CBlankMenu::CBlankMenu( void )
{
}

/*
**
*/
CBlankMenu::~CBlankMenu( void )
{
}

/*
**
*/
void CBlankMenu::enter( void )
{
}

/*
**
*/
void CBlankMenu::exit( void )
{
}

/*
**
*/
void CBlankMenu::update( float fDT )
{
    CMenuScreen::update( fDT );
}