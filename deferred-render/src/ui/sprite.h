#ifndef __CSPRITE_H__
#define __CSPRITE_H__

#include "control.h"
#include "texturemanager.h"
#include "textureatlasmanager.h"

class CSprite : public CControl
{
public:
	CSprite( void );
	virtual ~CSprite( void );
	
	void setTexture( tTexture* pTexture ) { mpTexture = pTexture; }
	inline void setOrigTextureWidth( int iWidth ) { miTexOrigWidth = iWidth; }
	inline void setOrigTextureHeight( int iHeight ) { miTexOrigHeight = iHeight; }
	
	inline tTexture* getTexture( void ) { return mpTexture; }
	
	virtual void draw( double fTime, int iScreenWidth, int iScreenHeight );
    
    virtual CControl* copy( void );
    
    inline void setClipUV( float fU, float fV ) { mfClipU = fU; mfClipV = fV; }
    inline void setClip( bool bClip ) { mbClip = bClip; }
    inline void setTextureAtlasInfo( tTextureAtlasInfo* pInfo ) { mpAtlasInfo = pInfo; }
    inline void setUV( float fLeftU, float fRightU, float fTopV, float fBottomV ) 
    { 
        mTopLeftUV.fX = fLeftU; mTopLeftUV.fY = fTopV; mBottomRightUV.fX = fRightU; mBottomRightUV.fY = fBottomV; 
    }
    
    inline tTextureAtlasInfo* getAtlasInfo( void ) { return mpAtlasInfo; }
    
    inline void setAtlasInfo( tTextureAtlasInfo* pInfo ) { mpAtlasInfo = pInfo; }
    
protected:
	char			mszTextureName[128];

	tTexture*		mpTexture;
	int				miTexOrigWidth;
	int				miTexOrigHeight;
    
    bool            mbClip;
    float           mfClipU;
    float           mfClipV;
    
    tVector2        mTopLeftUV;
    tVector2        mBottomRightUV;
    
    tTextureAtlasInfo*  mpAtlasInfo;
};

#endif // __CSPRITE_H__