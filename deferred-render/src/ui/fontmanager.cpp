//
//  fontmanager.cpp
//  CityBenchmark
//
//  Created by Dingwings on 7/31/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "fontmanager.h"
#include "ShaderManager.h"
#include "tinyxml.h"
#include "filepathutil.h"

#define NUM_FONTS_PER_ALLOC 2

CFontManager* CFontManager::mpInstance = NULL;

/*
**
*/
CFontManager* CFontManager::instance( void )
{
    if( mpInstance == NULL )
    {
        mpInstance = new CFontManager();
    }
    
    return mpInstance;
}

/*
**
*/
CFontManager::CFontManager( void )
{
    miNumFonts = 0;
    
}

/*
**
*/
CFontManager::~CFontManager( void )
{
}

/*
**
*/
void CFontManager::registerFont( const char* szFont )
{
    if( getFont( szFont ) == NULL )
    {
        assert( miNumFonts < MAX_FONTS );
        CFont* pFont = &maFonts[miNumFonts];
        pFont->readFile( szFont );
        ++miNumFonts;
    
        unsigned int iShader = CShaderManager::instance()->getShader( "noperspective" );
        int iPosHandle = glGetAttribLocation( iShader, "position" );
        int iUVHandle = glGetAttribLocation( iShader, "textureUV" );
        int iColorHandle = glGetAttribLocation( iShader, "color" );
        int iOrientationHandle = glGetUniformLocation( iShader, "orientationMat" );
        
        pFont->setShaderHandles( iPosHandle, iUVHandle, iColorHandle, iOrientationHandle );
    }
}

/*
**
*/
CFont* CFontManager::getFont( const char* szFont )
{
    const char* szExtension = strstr( szFont, "." );
    char szFormatName[128];
    if( szExtension == NULL )
    {
        snprintf( szFormatName, sizeof( szFormatName ), "%s.fnt", szFont ); 
    }
    else
    {
        strncpy( szFormatName, szFont, sizeof( szFormatName ) );
    }
    
    CFont* pFont = NULL;
    for( int i = 0; i < miNumFonts; i++ )
    {
        if( !strcmp( maFonts[i].getName(), szFormatName ) )
        {
            pFont = &maFonts[i];
            break;
        }
    }
    
    return pFont;
}

/*
**
*/
void CFontManager::loadFonts( const char* szFileName )
{
    char szFullPath[256];
    
    getFullPath( szFullPath, szFileName );
    TiXmlDocument doc( szFullPath );
    bool bLoaded = doc.LoadFile();
    
    if( bLoaded )
    {
        TiXmlNode* pNode = doc.FirstChild()->FirstChild();
        while( pNode )
        {
            const char* szFontName = pNode->FirstChild()->Value();
            registerFont( szFontName );
            
            pNode = pNode->NextSibling();
        }
    }
}