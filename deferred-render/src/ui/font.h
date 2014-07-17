//
//  font.h
//  CityBenchmark
//
//  Created by Dingwings on 7/27/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//
#ifndef __FONT_H__
#define __FONT_H__

#include "vector.h"
#include "texturemanager.h"
#include "textureatlasmanager.h"

#include <vector>

struct Coord2Di
{
	int		miX;
	int		miY;
};

typedef struct Coord2Di tCoord2Di;

struct Glyph
{
	tCoord2Di	mTopLeft;
	tCoord2Di	mBottomRight;
	int			miWidth;
	int			miHeight;
    
    int         miXOffset;
    int         miYOffset;
	int         miXAdvance;
    
	tCoord2Di	mOrigin;
    
	tVector4	maVerts[4];
	tVector2	maUV[4];
    
    std::vector<tCoord2Di *> maKerning;
};

typedef struct Glyph tGlyph;

class CFont
{
public:
    enum
    {
        ALIGN_LEFT = 0,
        ALIGN_CENTER,
        ALIGN_RIGHT,
        
        NUM_ALIGNMENTS,
    };
    
public:
    CFont( void );
    ~CFont( void );
    
    void readFile( const char* szFileName );
    void drawString( const char* szString,
                     float fX,
                     float fY,
                     float fSize,
                     float fScreenWidth,
                     float fScreenHeight,
                     float fRed,
                     float fGreen,
                     float fBlue,
                     float fAlpha,
                     int iAlignment,
                     bool bUseBatch = false );
    
    int getLineWidth( const char* szString, 
                      const float fTextSize, 
                      const int iCurrLine,
                      float fScreenWidth );
    
    inline void setShaderHandles( unsigned int iPosHandle,
                                 unsigned int iUVHandle,
                                 unsigned int iColorHandle,
                                 unsigned int iOrientationMatHandle )
    {
        miPositionHandle = iPosHandle;
        miColorHandle = iColorHandle;
        miTexCoordHandle = iUVHandle;
        miOrientationMatHandle = iOrientationMatHandle;
    }
    
    inline const char* getName( void ) { return mszName; }
    
    int getStringWidth( const char* szString, float fSize, float fScreenWidth );
    void wrapText( char* szResult, 
                    const char* szOrig, 
                    int iAlign, 
                    float fWidth,
                    float fSize,
                    int iResultSize,
                    float fScreenWidth );
    
    float getHeight( const char* szText, float fSize );
    
protected:
    tGlyph      maGlyphs[128];
    tVector4    maGlyphVerts[512];
    tTexture*   mpTexture;
    
    char        mszName[128];
    
    unsigned int    miPositionHandle;
    unsigned int    miColorHandle;
    unsigned int    miTexCoordHandle;
    unsigned int    miOrientationMatHandle;
    
    int             miOriginalSize;
    tTextureAtlasInfo*  mpTextureAtlasInfo;
    
};

#endif // __FONT_H__