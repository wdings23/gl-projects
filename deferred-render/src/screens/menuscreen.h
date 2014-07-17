//
//  Header.h
//  Game1
//
//  Created by Dingwings on 1/14/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef __MENUSCREEN_H__
#define __MENUSCREEN_H__

#include "layer.h"

class CMenuScreen 
{
public:
    CMenuScreen( void );
    ~CMenuScreen( void );
    
    inline void setLayer( CLayer* pLayer ) { mpLayer = pLayer; }
    
    void load( const char* szFileName, CLayer* pLayer );
    
    virtual void enter( void );
    virtual void update( float fDT );
    virtual void draw( void );
    virtual void exit( void );
    
    inline void setShader( int iShader ) { miShader = iShader; }
    void releaseLayer( void );
    
    inline CLayer* getLayer( void ) { return mpLayer; }
    inline const char* getName( void ) { return mszName; }
    inline void setName( const char* szName ) { strncpy( mszName, szName, sizeof( mszName ) ); }
    
protected:
    CLayer*     mpLayer;
    float       mfTotalTime;
    float       mfDT;
    int         miShader;
    
    bool        mbFinishedAnimation;
    char        mszName[256];
};

#endif // __MENUSCREEN_H__
