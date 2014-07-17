#include "text.h"
#include "fontmanager.h"

/*
**
*/
CText::CText( void )
{
	memset( mszText, 0, sizeof( mszText ) );
	mfSize = 32.0f;
	miAlign = ALIGN_LEFT;
    mpFont = NULL;
    
    miType = CControl::TYPE_TEXT;
    
    strncpy( mszText, "HELLO", sizeof( mszText ) );
    mbWrap = false;
    
    miVertAlign = VERTALIGN_NONE;
    mfOffsetY = 0.0f;
}

/*
**
*/
CText::~CText( void )
{
}

/*
**
*/
void CText::setText( const char* szText, float fScreenWidth )
{
    memset( mszText, 0, sizeof( mszText ) );
    
    if( mbWrap )
    {
        if( mpFont )
        {
            mpFont->wrapText( mszText, szText, miAlign, mDimension.fX, mfSize, sizeof( mszText ), fScreenWidth );
        }
        else
        {
            strncpy( mszText, szText, sizeof( mszText ) );
        }
    }
    else
    {
        strncpy( mszText, szText, sizeof( mszText ) );
    }
}

/*
**
*/
void CText::draw( double fTime, int iScreenWidth, int iScreenHeight )
{
    updateMatrix( fTime );
    
    if( miState == STATE_DISABLED )
	{
		return;
	}
    
    WTFASSERT2( mpFont, "No valid font for %s", mszName );
        
    if( miVertAlign == VERTALIGN_CENTER )
    {
        float fHeight = mpFont->getHeight( mszText, mfSize );
        mfOffsetY = -( fHeight - mfSize ) * 0.5f;
    }
    
    float fScale = 1.0f;
    if( mpAnimPlayer )
    {
        tVector4 scaling = { 1.0f, 1.0f, 1.0f, 1.0f };
        mpAnimPlayer->getScaling( &scaling );
        fScale = scaling.fX;
    }
    
	bool bUseBatch = false;
#if defined( UI_USE_BATCH )
	bUseBatch = true;
#endif // UI_USE_BATCH

    mpFont->drawString( mszText, 
                        mTotalMatrix.M( 0, 3 ), 
                        mTotalMatrix.M( 1, 3 ) + mfOffsetY, 
                        mfSize * fScale,
                        (float)iScreenWidth, 
                        (float)iScreenHeight, 
                        mColor.fX, 
                        mColor.fY, 
                        mColor.fZ, 
                        mColor.fW, 
                        miAlign,
						bUseBatch );
    
    // draw emitter
    if( mpAnimPlayer )
    {
        CEmitter* pEmitter = mpAnimPlayer->getEmitter();
        if( pEmitter )
        {
            pEmitter->render();
        }
    }
}

/*
**
*/
void CText::setFont( const char* szFont )
{    
    mpFont = CFontManager::instance()->getFont( szFont );
    if( mpFont == NULL )
    {
        mpFont = CFontManager::instance()->getFont( "Verdana.fnt" );
        OUTPUT( "can't find font %s\n", szFont );
    }
    
    WTFASSERT2( mpFont, "can't find font : %s", szFont );
}

/*
**
*/
CControl* CText::copy( void )
{
    CText* pText = new CText(); 
        
    copyControl( pText );
    
    memcpy( pText->mszText, mszText, sizeof( mszText ) );
    pText->mpFont = mpFont;
	pText->mfSize = mfSize;
	pText->miAlign = miAlign;
    pText->miVertAlign = miVertAlign;
    
    return pText;
}

/*
**
*/
void CText::setWrap( bool bWrap, float fScreenWidth )
{
    mbWrap = bWrap;
    /*if( mbWrap )
    {
        char szText[256];
        memcpy( szText, mszText, sizeof( szText ) );
        mpFont->wrapText( mszText, szText, miAlign, mDimension.fX, mfSize, sizeof( mszText ), fScreenWidth );
    }*/
}