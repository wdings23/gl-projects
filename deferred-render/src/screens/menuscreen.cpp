//
//  menuscreen.cpp
//  Game1
//
//  Created by Dingwings on 1/14/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "menuscreen.h"
#include "ShaderManager.h"
#include "render.h"

/*
**
*/
CMenuScreen::CMenuScreen( void )
{
    mpLayer = NULL;
    miShader = 0;
    
    mfDT = 0.0f;
    mfTotalTime = 0.0f;
    mbFinishedAnimation = false;
    
    memset( mszName, 0, sizeof( mszName ) );
}

/*
**
*/
CMenuScreen::~CMenuScreen( void )
{
    
}

/*
**
*/
void CMenuScreen::load( const char* szFileName, CLayer* pLayer )
{
    mpLayer = pLayer;
    mpLayer->load( szFileName, SCREEN_WIDTH );
    
    miShader = CShaderManager::instance()->getShader( "ui" );
}

/*
**
*/
void CMenuScreen::enter( void )
{
    mfTotalTime = 0.0f;
}

/*
**
*/
void CMenuScreen::update( float fDT )
{
    mfDT = fDT;
    mfTotalTime += fDT;
}

/*
**
*/
void CMenuScreen::draw( void )
{
    assert( miShader > 0 );
	int iScreenWidth = (int)( (float)renderGetScreenWidth() * renderGetScreenScale() );
	int iScreenHeight = (int)( (float)renderGetScreenHeight() * renderGetScreenScale() );

    mpLayer->draw( miShader, mfDT, iScreenWidth, iScreenHeight );
}

/*
**
*/
void CMenuScreen::exit( void )
{
    mpLayer = NULL;
    mfTotalTime = 0.0f;
}

/*
**
*/
void CMenuScreen::releaseLayer( void )
{
    delete mpLayer;
	mpLayer = NULL;
}
