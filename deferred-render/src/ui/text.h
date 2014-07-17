#ifndef __TEXT_H__
#define __TEXT_H__

#include "control.h"
#include "fontmanager.h"

class CText : public CControl
{
public:
	enum
	{
		ALIGN_LEFT = 0,
		ALIGN_CENTER,
		ALIGN_RIGHT,

		NUM_ALIGNMENTS,
	};
	
    enum 
    {
        VERTALIGN_NONE = 0,
        VERTALIGN_CENTER,
        
        NUM_VERTALIGNS,
    };
    
public:
    
	CText( void );
	virtual ~CText( void );

	void setText( const char* szText, float fScreenWidth = SCREEN_WIDTH );
	inline void setSize( float fSize ) { mfSize = fSize; }
	inline void setAlign( int iAlign ) { miAlign = iAlign; }
	
	virtual void draw( double fTime, int iScreenWidth, int iScreenHeight );
    void setFont( const char* szFont );
    inline CFont* getFont( void ) { return mpFont; }
    inline float getSize( void ) { return mfSize; }
    
    virtual CControl* copy( void );
    
    void setWrap( bool bWrap, float fScreenWidth = SCREEN_WIDTH );
    inline bool getWrap( void ) { return mbWrap; }
    
    inline const char* getText( void ) { return mszText; }
    inline void setVertAlign( int iAlign ) { miVertAlign = iAlign; }
    
protected:
	char	mszText[256];
    CFont*  mpFont;
	float	mfSize;
	int		miAlign;
    
    bool    mbWrap;
    int     miVertAlign;
    
    float   mfOffsetY;
};

#endif // __TEXT_H__