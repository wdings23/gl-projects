#include "layer.h"
#include "texturemanager.h"
#include "filepathutil.h"
#include "ShaderManager.h"
#include "controller.h"

#define NUM_CONTROLS_PER_ALLOC 50

/*
**
*/
CLayer::CLayer( void )
{
	miNumControlsAlloc = NUM_CONTROLS_PER_ALLOC;
	miNumControls = 0;

	mapControls = new CControl*[miNumControlsAlloc];
	memset( mapControls, 0, sizeof( CControl* ) * miNumControlsAlloc );
    
    mbFinishedAnimation = true;
    miState = CControl::STATE_NORMAL;
    
    memset( maiNumEmitters, 0, sizeof( maiNumEmitters ) );
    memset( maEmitterInfo, 0, sizeof( maEmitterInfo ) );
    miAnimType = CMenuAnimPlayer::TYPE_ENTER;
    
    mScale.fX = mScale.fY = 1.0f;
    
    mfTotalTime = 0.0f;
    mpLastInteractControl = NULL;
}

/*
**
*/
CLayer::~CLayer( void )
{
	for( int i = 0; i < miNumControlsAlloc; i++ )
	{
		if( mapControls[i] )
		{
			delete mapControls[i];
		}
	}

	delete[] mapControls;

	miNumControlsAlloc = 0;
	miNumControls = 0;
    
    mfTotalTime = 0.0f;
}

/*
**
*/
void CLayer::draw( unsigned int iShaderProgram, double fTime, int iScreenWidth, int iScreenHeight )
{
    if( miState == CControl::STATE_DISABLED )
    {
        return;
    }
    
    int iPrevAnimType = miAnimType;
    
    mbFinishedAnimation = true;
    traverseControls( checkNormalState );
    if( mbFinishedAnimation )
    {
        miAnimType = CMenuAnimPlayer::TYPE_NORMAL;
        traverseControls( setNormalState );
    }
    
    // check focus and exit state
    traverseControls( checkFocusState );
    traverseControls( checkExitState );
    
	for( int i = 0; i < miNumControls; i++ )
	{
		//mapControls[i]->setShader( iShaderProgram );
		mapControls[i]->draw( fTime, iScreenWidth, iScreenHeight );
	}
    
    // start emitter based on state
    if( miAnimType == CMenuAnimPlayer::TYPE_ENTER )
    {
        startEmitters( CMenuAnimPlayer::TYPE_ENTER );
    }
    else if( miAnimType == CMenuAnimPlayer::TYPE_NORMAL )
    {
        if( iPrevAnimType != CMenuAnimPlayer::TYPE_NORMAL )
        {
            mfStartStateTime = mfTotalTime;
        }
        
        startEmitters( CMenuAnimPlayer::TYPE_NORMAL );

    }
    else if( miAnimType == CMenuAnimPlayer::TYPE_FOCUS )
    {
        if( iPrevAnimType != CMenuAnimPlayer::TYPE_FOCUS )
        {
            mfStartStateTime = mfTotalTime;
        }
        
        startEmitters( CMenuAnimPlayer::TYPE_FOCUS );

    }
    else if( miAnimType == CMenuAnimPlayer::TYPE_EXIT )
    {
        if( iPrevAnimType != CMenuAnimPlayer::TYPE_EXIT )
        {
            mfStartStateTime = mfTotalTime;
        }
        
        startEmitters( CMenuAnimPlayer::TYPE_EXIT );

    }
    
    for( int i = 0; i < CMenuAnimPlayer::TYPE_USER_0; i++ )
    {
        int iNumEmitters = maiNumEmitters[i];
        
        for( int j = 0; j < iNumEmitters; j++ )
        {
            maEmitters[i][j].update( fTime );
            maEmitters[i][j].render();
        }
    }
    
    mfTotalTime += fTime;

}

/*
**
*/
void CLayer::addControl( CControl* pControl )
{
	if( miNumControls + 1 >= miNumControlsAlloc )
	{
		miNumControlsAlloc += NUM_CONTROLS_PER_ALLOC;
		CControl** pTemp = new CControl*[miNumControls];
        for( int i = 0; i < miNumControls; i++ )
        {
            pTemp[i] = mapControls[i];
        }
        
        delete[] mapControls;
    
		mapControls = new CControl*[miNumControlsAlloc];
        for( int i = 0; i < miNumControls; i++ )
        {
            mapControls[i] = pTemp[i];
        }
        delete[] pTemp;
	}

	mapControls[miNumControls] = pControl;
	++miNumControls;
}

/*
**
*/
void CLayer::load( const char* szFileName, float fScreenWidth )
{
    OUTPUT( "load layer %s\n", szFileName );
    
	strncpy( mszName, szFileName, sizeof( mszName ) );
    
    char szFullPath[256];
	getFullPath( szFullPath, szFileName );

	TiXmlDocument doc( szFullPath );
	bool bLoaded = doc.LoadFile();
	
	if( bLoaded )
	{
		TiXmlElement* pElement = doc.FirstChildElement();
		TiXmlNode* pNode = pElement->FirstChild();
		
		while( pNode )
		{
//			// skip comments
			while( pNode && pNode->Type() == TiXmlNode::TINYXML_COMMENT )
			{
				pNode = pNode->NextSibling();
			}
			
			if( pNode == NULL )
			{
				break;
			}
			
			TiXmlElement* pElement = pNode->FirstChild()->ToElement();
			const char* szValue = pNode->Value();
			
			if( !strcmp( szValue, "texture" ) )
			{
				handleTexture( pElement );
			}
			else if( !strcmp( szValue, "button" ) )
			{
				CButton* pButton = handleButton( NULL, pNode, fScreenWidth );
				addControl( pButton );
			}
			else if( !strcmp( szValue, "text" ) )
			{
				CText* pText = handleText( pElement, fScreenWidth );
				addControl( pText );
			}
			else if( !strcmp( szValue, "sprite" ) )
			{
				CSprite* pSprite = handleSprite( pNode->ToElement() );
				addControl( pSprite );
			}
			else if( !strcmp( szValue, "control" ) )
			{
				CControl* pControl = handleControl( NULL, pNode, fScreenWidth );
				addControl( pControl );
			}
            else if( !strcmp( szValue, "list" ) )
			{
				CScrollList* pScrollList = handleScrollList( NULL, pNode, fScreenWidth );
				addControl( pScrollList );
			}
            else if( !strcmp( szValue, "box" ) )
            {
                CBox* pBox = handleBox( pNode->ToElement() );
                addControl( pBox );
            }
			
			pNode = pNode->NextSibling();
		}	// while valid node
        
        traverseControls( setShader );
        
	}	// if loaded
    else
    {
        OUTPUT( "error loading %s: %s row: %d col: %d\n", szFileName, doc.ErrorDesc(), doc.ErrorRow(), doc.ErrorCol() );
        WTFASSERT2( 0, "can not load file: %s", szFileName );
    }
    
    traverseControls( checkDuplicateNames );
}

/*
**
*/
CControl* CLayer::handleControl( CControl* pParent, TiXmlNode* pNode, float fScreenWidth )
{
	TiXmlNode* pChild = pNode->FirstChild();
	CControl* pControl = new CControl();

	// add child
	if( pParent )
	{
		pParent->addChild( pControl );
	}
	
	pControl->setParent( pParent );

	while( pChild )
	{
		const char* szValue = pChild->Value();
		if( !strcmp( szValue, "control" ) )
		{
			handleControl( pControl, pChild, fScreenWidth );
		}
		else if( !strcmp( szValue, "sprite" ) )
		{
			CSprite* pSprite = handleSprite( pChild->ToElement() );
			pControl->addChild( pSprite );
		}
		else if( !strcmp( szValue, "button" ) )
		{
			handleButton( pControl, pChild->ToElement(), fScreenWidth );
		}
        else if( !strcmp( szValue, "text" ) )
		{
			CText* pText = handleText( pChild->ToElement(), fScreenWidth );
            pControl->addChild( pText );
        }
        else if( !strcmp( szValue, "list" ) )
        {
            CScrollList* pScrollList = handleScrollList( NULL, pChild, fScreenWidth );
            pControl->addChild( pScrollList );
        }
        else if( !strcmp( szValue, "box" ) )
        {
            CBox* pBox = handleBox( pChild->ToElement() );
            pControl->addChild( pBox );
        }
		else if( !strcmp( szValue, "name" ) )
		{
			const char* szName = pChild->FirstChild()->Value();
			pControl->setName( szName );
        }
		else if( !strcmp( szValue, "x" ) )
		{
			const char* szX = pChild->FirstChild()->Value();
			tVector2 position = pControl->getPosition();
			position.fX = atof( szX ) * mScale.fX;
			pControl->setPosition( position );
            pControl->setOrigPosition( position );
		}
		else if( !strcmp( szValue, "y" ) )
		{
			const char* szY = pChild->FirstChild()->Value();
			tVector2 position = pControl->getPosition();
			position.fY = atof( szY ) * mScale.fY;
			pControl->setPosition( position );
            pControl->setOrigPosition( position );

		}
		else if( !strcmp( szValue, "width" ) )
		{
			const char* szWidth = pChild->FirstChild()->Value();
			tVector2 dimension = pControl->getDimension();
			dimension.fX = atof( szWidth ) * mScale.fX;
			pControl->setDimension( dimension );
		}
		else if( !strcmp( szValue, "height" ) )
		{
			const char* szHeight = pChild->FirstChild()->Value();
			tVector2 dimension = pControl->getDimension();
			dimension.fY = atof( szHeight ) * mScale.fY;
			pControl->setDimension( dimension );
		}

		pChild = pChild->NextSibling();
	}
	
	return pControl;
}

/*
**
*/
CSprite* CLayer::handleSprite( TiXmlElement* pNode )
{
	CTextureManager* pTextureManager = CTextureManager::instance();
	tTexture* pTexture = NULL;

	CSprite* pSprite = new CSprite();
	assert( pSprite );

	TiXmlNode* pChild = pNode->FirstChild();
	while( pChild )
	{
		const char* szValue = pChild->Value();
		if( !strcmp( szValue, "name" ) )
		{
			const char* szName = pChild->FirstChild()->Value();
			pSprite->setName( szName );
		}
		else if( !strcmp( szValue, "x" ) )
		{
			const char* szX = pChild->FirstChild()->Value();
			tVector2 position = pSprite->getPosition();
			position.fX = atof( szX ) * mScale.fX;
			pSprite->setPosition( position );
            pSprite->setOrigPosition( position );

		}
		else if( !strcmp( szValue, "y" ) )
		{
			const char* szY = pChild->FirstChild()->Value();
			tVector2 position = pSprite->getPosition();
			position.fY = atof( szY ) * mScale.fY;
			pSprite->setPosition( position );
            pSprite->setOrigPosition( position );
		}
		else if( !strcmp( szValue, "width" ) )
		{
			const char* szWidth = pChild->FirstChild()->Value();
			tVector2 dimension = pSprite->getDimension();
			dimension.fX = atof( szWidth ) * mScale.fX;
			pSprite->setDimension( dimension );
		}
		else if( !strcmp( szValue, "height" ) )
		{
			const char* szHeight = pChild->FirstChild()->Value();
			tVector2 dimension = pSprite->getDimension();
			dimension.fY = atof( szHeight ) * mScale.fY;
			pSprite->setDimension( dimension );
		}
		else if( !strcmp( szValue, "texture" ) )
		{
			const char* szTexture = pChild->FirstChild()->Value();
            WTFASSERT2( szTexture, "no texture specified" );
			tTextureAtlasInfo* pInfo = CTextureAtlasManager::instance()->getTextureAtlasInfo( szTexture );
            
            // check for texture atlas info
            if( pInfo == NULL )
            {
                pTexture = pTextureManager->getTexture( szTexture );
                if( pTexture == NULL )
                {
                    pTextureManager->registerTexture( szTexture );
                    pTexture = pTextureManager->getTexture( szTexture );
                    assert( pTexture );
                }
            }
            else
            {
                pTexture = pInfo->mpTexture;
                pSprite->setTextureAtlasInfo( pInfo );
            }
            
            WTFASSERT2( pTexture, "can't find texture: %s", szTexture );
			pSprite->setTexture( pTexture );
		}

		pChild = pChild->NextSibling();
	}

	return pSprite;
}

/*
**
*/
CText* CLayer::handleText( TiXmlElement* pNode, float fScreenWidth )
{
	CText* pText = new CText();
	assert( pText );
	
    float fX, fY, fWidth, fHeight, fSize;
    float fRed, fGreen, fBlue;
    char szText[256];
    const char* szFont = "Verdana";
    int iAlignment = CFont::ALIGN_LEFT;
    
    memset( szText, 0, sizeof( szText ) );
    strncpy( szText, "A Quick Brown Fox Jumps Over The Lazy Dog.", sizeof( szText ) );
    
    TiXmlNode* pChild = pNode->FirstChild();
	while( pChild )
	{
        const char* szValue = pChild->Value();
        if( !strcmp( szValue, "name" ) )
        {
            const char* szName = pChild->FirstChild()->Value();
            pText->setName( szName );
        }
        else if( !strcmp( szValue, "x" ) )
        {
            const char* szX = pChild->FirstChild()->Value();
            fX = atof( szX ) * mScale.fX;
        }
        else if( !strcmp( szValue, "y" ) )
        {
            const char* szY = pChild->FirstChild()->Value();
            fY = atof( szY ) * mScale.fY;
        }
        else if( !strcmp( szValue, "width" ) )
        {
            const char* szWidth = pChild->FirstChild()->Value();
            fWidth = atof( szWidth ) * mScale.fX;
        }
        else if( !strcmp( szValue, "height" ) )
        {
            const char* szHeight = pChild->FirstChild()->Value();
            fHeight = atof( szHeight ) * mScale.fY;
        }
        else if( !strcmp( szValue, "string" ) )
        {
            const char* szString = pChild->FirstChild()->Value();
            strncpy( szText, szString, sizeof( szText ) );
        }
        else if( !strcmp( szValue, "size" ) )
        {
            const char* szSize = pChild->FirstChild()->Value();
            
            if( mScale.fX > mScale.fY )
            {
                fSize = atof( szSize ) * mScale.fX;
            }
            else
            {
                fSize = atof( szSize ) * mScale.fY;
            }
        }
        else if( !strcmp( szValue, "font" ) )
        {
            szFont = pChild->FirstChild()->Value();
        }
        else if( !strcmp( szValue, "red" ) )
        {
            const char* szRed = pChild->FirstChild()->Value();
            fRed = atof( szRed );
            fRed /= 255.0f;
        }
        else if( !strcmp( szValue, "green" ) )
        {
            const char* szGreen = pChild->FirstChild()->Value();
            fGreen = atof( szGreen );
            fGreen /= 255.0f;
        }
        else if( !strcmp( szValue, "blue" ) )
        {
            const char* szBlue = pChild->FirstChild()->Value();
            fBlue = atof( szBlue );
            fBlue /= 255.0f;
        }
        else if( !strcmp( szValue, "alignment" ) )
        {
            const char* szAlignment = pChild->FirstChild()->Value();
            iAlignment = CFont::ALIGN_LEFT;
            if( !strcmp( szAlignment, "center" ) )
            {
                iAlignment = CFont::ALIGN_CENTER;
            }
            else if( !strcmp( szAlignment, "right" ) )
            {
                iAlignment = CFont::ALIGN_RIGHT;
            }
        }
        else if( !strcmp( szValue, "wrap" ) )
        {
            const char* szWrap = pChild->FirstChild()->Value();
            if( !strcmp( szWrap, "true" ) )
            {
                pText->setWrap( true );
            }
        }
        
        pChild = pChild->NextSibling();
    }
	
	tVector2 dimension = { fWidth, fHeight };
	tVector2 position = { fX - fWidth * 0.5f, fY - fHeight * 0.5f };
    if( iAlignment == CFont::ALIGN_CENTER )
    {
        position.fX = fX;
        position.fY = fY - fHeight * 0.75f;
    }
    
	pText->setDimension( dimension );
	pText->setPosition( position );
    pText->setOrigPosition( position );
    pText->setFont( szFont );
    pText->setText( szText, fScreenWidth );
    pText->setSize( fSize );
	pText->setColor( fRed, fGreen, fBlue, 1.0f );
    pText->setAlign( iAlignment );
    
	return pText;
}

/*
**
*/
CButton* CLayer::handleButton( CControl* pParent, TiXmlNode* pNode, float fScreenWidth )
{
	TiXmlNode* pChild = pNode->FirstChild();
	CButton* pButton = new CButton();

	// add child
	if( pParent )
	{
		pParent->addChild( pButton );
	}
	
	pButton->setParent( pParent );

	while( pChild )
	{
		const char* szValue = pChild->Value();
		if( !strcmp( szValue, "control" ) )
		{
			handleControl( pButton, pChild, fScreenWidth );
		}
		else if( !strcmp( szValue, "sprite" ) )
		{
			CSprite* pSprite = handleSprite( pChild->ToElement() );
			pButton->addChild( pSprite );
		}
        else if( !strcmp( szValue, "text" ) )
        {
            CText* pText = handleText( pChild->ToElement(), fScreenWidth );
            pButton->addChild( pText );
        }
        else if( !strcmp( szValue, "box" ) )
        {
            CBox* pBox = handleBox( pChild->ToElement() );
            pButton->addChild( pBox );
        }
		else if( !strcmp( szValue, "name" ) )
		{
			const char* szName = pChild->FirstChild()->Value();
			pButton->setName( szName );
        }
		else if( !strcmp( szValue, "x" ) )
		{
			const char* szX = pChild->FirstChild()->Value();
			tVector2 position = pButton->getPosition();
			position.fX = atof( szX ) * mScale.fX;
			pButton->setPosition( position );
            pButton->setOrigPosition( position );
		}
		else if( !strcmp( szValue, "y" ) )
		{
			const char* szY = pChild->FirstChild()->Value();
			tVector2 position = pButton->getPosition();
			position.fY = atof( szY ) * mScale.fY;
			pButton->setPosition( position );
            pButton->setOrigPosition( position );
		}
		else if( !strcmp( szValue, "width" ) )
		{
			const char* szWidth = pChild->FirstChild()->Value();
			tVector2 dimension = pButton->getDimension();
			dimension.fX = atof( szWidth ) * mScale.fX;
			pButton->setDimension( dimension );
		}
		else if( !strcmp( szValue, "height" ) )
		{
			const char* szHeight = pChild->FirstChild()->Value();
			tVector2 dimension = pButton->getDimension();
			dimension.fY = atof( szHeight ) * mScale.fY;
			pButton->setDimension( dimension );
		}
        else if( !strcmp( szValue, "link" ) )
        {
            const char* szLink = pChild->FirstChild()->Value();
            pButton->setNextScreen( szLink );
        }
        
		pChild = pChild->NextSibling();
	}
	
	return pButton;
}

/*
**
*/
void CLayer::handleTexture( TiXmlElement* pNode )
{
	const char* szName = pNode->Attribute( "name" );
	assert( szName );

	CTextureManager* pTextureManager = CTextureManager::instance();
	pTextureManager->registerTexture( szName );
}

/*
**
*/
CScrollList* CLayer::handleScrollList( CControl* pParent, TiXmlNode* pNode, float fScreenWidth )
{
    CScrollList* pScrollList = new CScrollList();
    
    float fX, fY, fWidth, fHeight;
    CControl* pTemplate = NULL;
    
    TiXmlNode* pChild = pNode->FirstChild();
	while( pChild )
	{
        const char* szValue = pChild->Value();
        if( !strcmp( szValue, "name" ) )
        {
            const char* szName = pChild->FirstChild()->Value();
            pScrollList->setName( szName );
        }
        else if( !strcmp( szValue, "x" ) )
        {
            const char* szX = pChild->FirstChild()->Value();
            fX = atof( szX ) * mScale.fX;
        }
        else if( !strcmp( szValue, "y" ) )
        {
            const char* szY = pChild->FirstChild()->Value();
            fY = atof( szY ) * mScale.fY;
        }
        else if( !strcmp( szValue, "width" ) )
        {
            const char* szWidth = pChild->FirstChild()->Value();
            fWidth = atof( szWidth ) * mScale.fX;
        }
        else if( !strcmp( szValue, "height" ) )
        {
            const char* szHeight = pChild->FirstChild()->Value();
            fHeight = atof( szHeight ) * mScale.fY;
        }
        else if( !strcmp( szValue, "sprite" ) )
        {
            CSprite* pSprite = handleSprite( pChild->ToElement() );
            pScrollList->addChild( pSprite );
        }
		else if( !strcmp( szValue, "box" ) )
        {
			CBox* pBox = handleBox( pChild->ToElement() );
            pScrollList->addChild( pBox );
        }
        else if( !strcmp( szValue, "template" ) )
        {
            pTemplate = handleControl( NULL, pChild->ToElement(), fScreenWidth );
        }
        
        pChild = pChild->NextSibling();
    }
    
    // dimension and position
    tVector2 position = { fX, fY };
    tVector2 dimension = { fWidth, fHeight };
    pScrollList->setPosition( position );
    pScrollList->setOrigPosition( position );
    pScrollList->setDimension( dimension );
    
    pScrollList->setTemplate( pTemplate );
    delete pTemplate;
    
    return pScrollList;
}

/*
**
*/
CBox* CLayer::handleBox( TiXmlElement* pNode )
{
    CBox* pBox = new CBox();
    
    tVector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    
    TiXmlNode* pChild = pNode->FirstChild();
	while( pChild )
	{
		const char* szValue = pChild->Value();
		if( !strcmp( szValue, "name" ) )
		{
			const char* szName = pChild->FirstChild()->Value();
			pBox->setName( szName );
		}
		else if( !strcmp( szValue, "x" ) )
		{
			const char* szX = pChild->FirstChild()->Value();
			tVector2 position = pBox->getPosition();
			position.fX = atof( szX ) * mScale.fX;
			pBox->setPosition( position );
            pBox->setOrigPosition( position );
            
		}
		else if( !strcmp( szValue, "y" ) )
		{
			const char* szY = pChild->FirstChild()->Value();
			tVector2 position = pBox->getPosition();
			position.fY = atof( szY ) * mScale.fY;
			pBox->setPosition( position );
            pBox->setOrigPosition( position );
		}
		else if( !strcmp( szValue, "width" ) )
		{
			const char* szWidth = pChild->FirstChild()->Value();
			tVector2 dimension = pBox->getDimension();
			dimension.fX = atof( szWidth ) * mScale.fX;
			pBox->setDimension( dimension );
		}
		else if( !strcmp( szValue, "height" ) )
		{
			const char* szHeight = pChild->FirstChild()->Value();
			tVector2 dimension = pBox->getDimension();
			dimension.fY = atof( szHeight ) * mScale.fY;
			pBox->setDimension( dimension );
		}
        else if( !strcmp( szValue, "red" ) )
        {
            const char* szRed = pChild->FirstChild()->Value();
            color.fX = atof( szRed );
            color.fX /= 255.0f;
            
            pBox->setColor( color.fX, color.fY, color.fZ, color.fW );
        }
        else if( !strcmp( szValue, "green" ) )
        {
            const char* szGreen = pChild->FirstChild()->Value();
            color.fY = atof( szGreen );
            color.fY /= 255.0f;
            
            pBox->setColor( color.fX, color.fY, color.fZ, color.fW );
        }
        else if( !strcmp( szValue, "blue" ) )
        {
            const char* szBlue = pChild->FirstChild()->Value();
            color.fZ = atof( szBlue );
            color.fZ /= 255.0f;
            
            pBox->setColor( color.fX, color.fY, color.fZ, color.fW );
        }
        else if( !strcmp( szValue, "alpha" ) )
        {
            const char* szAlpha = pChild->FirstChild()->Value();
            color.fW = atof( szAlpha );
            color.fW /= 255.0f;
            
            pBox->setColor( color.fX, color.fY, color.fZ, color.fW );
        }
        
        pChild = pChild->NextSibling();
        
    }   // while still have child
    
    return pBox;
}

/*
**
*/
CControl* CLayer::inputUpdate( float fCursorX, 
                               float fCursorY, 
                               int iHit, 
                               int iNumTouches,
                               int iTouchType )
{
    if( miState == CControl::STATE_DISABLED )
    {
        return 0;
    }
    
	CControl* pRet = NULL;
	for( int i = miNumControls - 1; i >= 0; i-- )
	{
		pRet = mapControls[i]->inputUpdate( fCursorX, 
                                            fCursorY, 
                                            iNumTouches,
                                            iTouchType,
                                            mpLastInteractControl );
	
        // hit a control
        if( pRet )
        {
            break;
        }
    }
    
    if( iTouchType == TOUCHCONTROL_BEGAN )
    {
        mpLastInteractControl = pRet;
    }
    else if( iTouchType == TOUCHCONTROL_ENDED )
    {
        mpLastInteractControl = NULL;
    }

	return pRet;
}

/*
**
*/
bool CLayer::checkControlName( const char* szName, const char* szToFind )
{
	bool bRet = false;
	int iStrLen = (int)strlen( szToFind );
	int iNameLen = (int)strlen( szName );
	
	// compare the last letters of the name
	int iStart = iNameLen - iStrLen;
	if( iStart >= 0 )
	{
		if( !strcmp( &szName[iStart], szToFind ) )
		{
			bRet = true;
		}
	}

	return bRet;
}

/*
**
*/
CControl* CLayer::findControl( const char* szName, CControl* pParent )
{
	CControl* pControl = NULL;
	if( pParent == NULL )
	{
		for( int i = 0; i < miNumControls; i++ )
		{
			if( checkControlName( mapControls[i]->getName(), szName ) )
			{
				pControl = mapControls[i];
				break;
			}

			pControl = findControl( szName, mapControls[i] );
			if( pControl )
			{
				break;
			}
		}
	}
	else
	{
		for( unsigned int i = 0; i < pParent->getNumChildren(); i++ )
		{
			CControl* pChild = pParent->getChild( i );
			if( checkControlName( pChild->getName(), szName ) )
			{
				pControl = pChild;
				break;
			}

			pControl = findControl( szName, pChild );
			if( pControl )
			{
				break;
			}
		}
	}

	return pControl;
}

/*
**
*/
CControl* CLayer::handleGroup( TiXmlNode* pNode, CControl* pParent )
{
	CSprite* pSprite = NULL;
	
	// children
	TiXmlNode* pChild = pNode->FirstChild();
	while( pChild )
	{
		const char* szName = pChild->Value();
		if( !strcmp( szName, "group" ) )
		{
			if( pSprite == NULL )
			{
				pSprite = new CSprite();
				if( pParent )
				{
					pParent->addChild( pSprite );
				}
			}

			handleGroup( pChild, pSprite );
		}
		else if( !strcmp( szName, "data" ) )
		{
			if( pSprite == NULL )
			{
				pSprite = new CSprite();
				if( pParent )
				{
					pParent->addChild( pSprite );
				}
			}

			TiXmlElement* pElement = pChild->ToElement();
			
			const char* szName = pElement->Attribute( "name" );
			assert( szName );
			OUTPUT( "name = %s\n", szName );
			pSprite->setName( szName );

			const char* szTexture = pElement->Attribute( "texture" );
			
			const char* szX = pElement->Attribute( "x" );
			assert( szX );
			
			const char* szY = pElement->Attribute( "y" );
			assert( szY );
			
			const char* szWidth = pElement->Attribute( "w" );
			assert( szWidth );
			
			const char* szHeight = pElement->Attribute( "h" );
			assert( szHeight );

			float fX = atof( szX ) * mScale.fX;
			float fY = atof( szY ) * mScale.fY;
			float fWidth = atof( szWidth ) * mScale.fX;
			float fHeight = atof( szHeight ) * mScale.fY;
			
			if( pParent )
			{
				fX += pParent->getPosition().fX;
				fY += pParent->getPosition().fY;
			}

			tVector2 dimension = { fWidth, fHeight };
			tVector2 position = { fX, fY };

			pSprite->setDimension( dimension );
			pSprite->setPosition( position );
            pSprite->setOrigPosition( position );
            
			pSprite->setName( szName );
			
			CTextureManager* pTextureManager = CTextureManager::instance();
			tTexture* pTexture = NULL;
			if( szTexture )
			{
				pTexture = pTextureManager->getTexture( szTexture );
				if( pTexture == NULL )
				{
					pTextureManager->registerTexture( szTexture );
					pTexture = pTextureManager->getTexture( szTexture );
				}
			}
			else
			{
				pTextureManager->registerTexture( szName );
				pTexture = pTextureManager->getTexture( szName );
			}
	
			assert( pTexture );
			pSprite->setTexture( pTexture );
		}

		pChild = pChild->NextSibling();
	}

	return pSprite;
}

/*
**
*/
void CLayer::traverseControls( void (*pfnFunc)( CControl* pControl, CLayer* pLayer, void* pUserData ), 
                               CControl* pParent,
                               void* pUserData )
{
	if( pParent == NULL )
	{
		for( int i = 0; i < miNumControls; i++ )
		{
			pfnFunc( mapControls[i], this, pUserData );
			traverseControls( pfnFunc, mapControls[i], pUserData );
		}
	}
	else
	{
		for( unsigned int i = 0; i < pParent->getNumChildren(); i++ )
		{
			CControl* pChild = pParent->getChild( i );
			pfnFunc( pChild, this, pUserData );
			traverseControls( pfnFunc, pChild, pUserData );
		}
	}
}

/*
**
*/
void CLayer::checkNormalState( CControl* pControl, CLayer* pLayer, void* pUserData )
{    
    if( pControl->getState() == CControl::STATE_NORMAL )
    {
        CMenuAnimPlayer* pAnimPlayer = pControl->getAnimPlayer();
        if( pAnimPlayer )
        {	
            if( pAnimPlayer->getType() == CMenuAnimPlayer::TYPE_ENTER )
            {
                if( pAnimPlayer->getTime() < pAnimPlayer->getLastTime( CMenuAnimPlayer::TYPE_ENTER ) )
                {
                    pLayer->setAnimationFinished( false );
                }
            }
            else
            {
                if( pAnimPlayer->getType() != CMenuAnimPlayer::TYPE_NORMAL )
                {
                    pLayer->setAnimationFinished( false );
                }
            }
        }
        
        // check if particle is done
        int iAnimType = pLayer->miAnimType;
        for( int i = 0; i < pLayer->maiNumEmitters[iAnimType]; i++ )
        {
            CEmitter* pEmitter = &pLayer->maEmitters[iAnimType][i];
            if( pEmitter->getDuration() + pLayer->maEmitterInfo[iAnimType][i].mfStartTime > pLayer->mfTotalTime )
            {
                pLayer->setAnimationFinished( false );
            }
        }
    }
}

/*
**
*/
void CLayer::checkFocusState( CControl* pControl, CLayer* pLayer, void* pUserData )
{
    CMenuAnimPlayer* pAnimPlayer = pControl->getAnimPlayer();
	if( pAnimPlayer )
	{	
		if( pAnimPlayer->getType() == CMenuAnimPlayer::TYPE_FOCUS )
        {
            pLayer->setAnimType( CMenuAnimPlayer::TYPE_FOCUS );
        }
    }

}

/*
 **
 */
void CLayer::checkExitState( CControl* pControl, CLayer* pLayer, void* pUserData )
{
    CMenuAnimPlayer* pAnimPlayer = pControl->getAnimPlayer();
	if( pAnimPlayer )
	{	
		if( pAnimPlayer->getType() == CMenuAnimPlayer::TYPE_EXIT )
        {
            pLayer->setAnimType( CMenuAnimPlayer::TYPE_EXIT );
        }
    }
    
}

/*
**
*/
void CLayer::setNormalState( CControl* pControl, CLayer* pLayer, void* pUserData )
{
    CMenuAnimPlayer* pAnimPlayer = pControl->getAnimPlayer();
	if( pAnimPlayer )
    {
        if( pAnimPlayer->getType() != CMenuAnimPlayer::TYPE_NORMAL )
        {
            pAnimPlayer->setType( CMenuAnimPlayer::TYPE_NORMAL );
        }
    }
}

/*
**
*/
void CLayer::setShader( CControl* pControl, CLayer* pLayer, void* pUserData )
{
    int iShader = CShaderManager::instance()->getShader( "ui" );
    pControl->setShader( iShader );
}

/*
**
*/
CControl* CLayer::getControl( int iIndex )
{
    return mapControls[iIndex];
}

/*
**
*/
int CLayer::getNumControls( void )
{
    return miNumControls;
}

/*
**
*/
void CLayer::findControls( const char* szName, std::vector<CControl *> *pList, CControl* pParent )
{
    if( pParent == NULL )
	{
		for( int i = 0; i < miNumControls; i++ )
		{
			if( checkControlName( mapControls[i]->getName(), szName ) )
			{
				pList->push_back( mapControls[i] );
            }
            
            findControls( szName, pList, mapControls[i] );
		}
	}
	else
	{
		for( unsigned int i = 0; i < pParent->getNumChildren(); i++ )
		{
			CControl* pChild = pParent->getChild( i );
			if( checkControlName( pChild->getName(), szName ) )
			{
				pList->push_back( pChild );
			}
            
			findControls( szName, pList, pChild );
		}
	}
}

/*
**
*/
void CLayer::addEmitter( const char* szName, int iState, tVector2 const* pPosition, float fStartTime )
{    
    int iNumEmitters = maiNumEmitters[iState];
    WTFASSERT2( iState < CMenuAnimPlayer::TYPE_USER_0, "invalid menu state to add emitter" );
    maEmitters[iState][iNumEmitters].loadFile( szName );
    
    tVector4 pos = { pPosition->fX * 0.5f, SCREEN_HEIGHT - pPosition->fY * 0.5f, 0.0f, 1.0f };
    maEmitters[iState][iNumEmitters].setSourcePosition( &pos );
    maEmitters[iState][iNumEmitters].setFullScreen( true );
    
    maEmitterInfo[iState][iNumEmitters].mfStartTime = fStartTime;
    maEmitterInfo[iState][iNumEmitters].mbStarted = false;
    
    ++maiNumEmitters[iState];
}

/*
**
*/
void CLayer::startEmitters( int iState )
{
    for( int i = 0; i < maiNumEmitters[iState]; i++ )
    {
        if( maEmitterInfo[iState][i].mfStartTime + mfStartStateTime <= mfTotalTime )
        {
            if( !maEmitterInfo[iState][i].mbStarted )
            {
                maEmitters[iState][i].start();
                maEmitterInfo[iState][i].mbStarted = true;
            }
        }
    }
}

/*
**
*/
void CLayer::setOffset( float fX, float fY )
{
    for( int i = 0; i < miNumControls; i++ )
    {
        mapControls[i]->setOffset( fX, fY );
    }
}

/*
**
*/
void CLayer::checkDuplicateNames( CControl* pControl, CLayer* pLayer, void* pUserData )
{
    
    pLayer->traverseControls( getControlName, NULL, pControl );
}

/*
**
*/
void CLayer::getControlName( CControl* pControl, CLayer* pLayer, void* pUserData )
{
#if defined( DEBUG )
    CControl* pOrig = (CControl *)pUserData;
    if( pControl && 
        pControl != pUserData &&
        !strstr( pControl->getName(), "_template" ) )
    {
        int iComp = strcmp( pControl->getName(), pOrig->getName() );
        WTFASSERT2( iComp != 0, "duplicate names in %s : %s", pLayer->mszName, pOrig->getName() );
    }
#endif // DEBUG
}
