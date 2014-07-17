//
//  fontmanager.h
//  CityBenchmark
//
//  Created by Dingwings on 7/31/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//
#ifndef __FONTMANAGER_H__
#define __FONTMANAGER_H__

#include "font.h"

#define MAX_FONTS 15

typedef struct FontSlot tFontSlot;

class CFontManager 
{
public:
    CFontManager( void );
    ~CFontManager( void );
    
    void registerFont( const char* szFont );
    CFont* getFont( const char* szFont );
    
    void loadFonts( const char* szFileName );
    
protected:
    CFont       maFonts[MAX_FONTS];
    int         miNumFonts;
    
public:
    static CFontManager* instance( void );
    
protected:
    static CFontManager*   mpInstance;
};

#endif // __FONTMANAGER_H__