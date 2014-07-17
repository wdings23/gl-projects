//
//  textureatlasmanager.h
//  Game2
//
//  Created by Dingwings on 6/14/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef __TEXTUREATLASMANAGER_H__
#define __TEXTUREATLASMANAGER_H__

#include "texturemanager.h"
#include "vector.h"

struct TextureAtlasInfo
{
    tTexture*       mpTexture;
    
    char            mszTextureName[128];
    int             miHashID;
    
    tVector2        mTopLeft;
    tVector2        mBottomRight;

	GLuint			miTextureID;
	int				miTextureAtlasWidth;
	int				miTextureAtlasHeight;

	double			mfTimeAccessed;
	bool			mbInMemory;
};

typedef struct TextureAtlasInfo tTextureAtlasInfo;


class CTextureAtlasManager
{
public:
    CTextureAtlasManager( void );
    ~CTextureAtlasManager( void );
    
    tTextureAtlasInfo*      getTextureAtlasInfo( const char* szTextureName );
    void                    loadInfo( const char* szFileName, const char* szExtension );
    
    inline int              getNumTextureAtlasInfo( void ) { return miNumAtlasInfo; }
    tTextureAtlasInfo*      getTextureAtlasInfo( int iIndex ) { return &maAtlasInfo[iIndex]; }
    
protected:
    tTextureAtlasInfo*          maAtlasInfo;
    int                         miNumAtlasInfo;
    int                         miNumAtlasInfoAlloc;
    
public:
    static CTextureAtlasManager*        instance( void );
    
protected:
    static CTextureAtlasManager*        mpInstance;
    
};

#endif // __TEXTUREATLASMANAGER_H__