#include "menumanager.h"
#include "menuanimmanager.h"
#include "filepathutil.h"

#include "blankmenu.h"
#include "textureatlasmanager.h"
#include "menus.h"

#include "render.h"

/*
**
 
                MENU FLOW:
 
                1) loadMenu from menuinfo.txt, getting the layer file name and animation file name
                    a) load layer with layer file name
                    b) load animation file
                    c) goes through all the control names in the animation file and set the animation player for the 
                       controls in the layer
 
                2) goes into menuscreen's enter(), set normal state if enter animation is done using setAppropriateNormalState()
                
                3) update and draw, check if focus animation is done using setAppropriateNormalState()
                
                4) exit button pressed, or any other mechanism to change menu - need to call exitMenu(), which sets all the control to play exit animation
                
                5) play exit animation, traverse layer checking for exit state using exitStateForControls()
                
                6) call menu screen's exit function
                
                7) delete layer and set current menu screen to null
                
                8) if there's a next menu name then load the menu
            
 
 
*/

/*
**
*/
CMenuManager* CMenuManager::mpInstance = NULL;
CMenuManager* CMenuManager::instance( void )
{
	if( mpInstance == NULL )
	{
		mpInstance = new CMenuManager();
    
        initMenus();
    }
    
	return mpInstance;
}

/*
**
*/
CMenuManager::CMenuManager( void )
{
	mpLayer = NULL;
    mpCurrScreen = NULL;
    mbFinishedAnimation = false;
    memset( mszNextMenuFileName, 0, sizeof( mszNextMenuFileName ) );
    mbInputEnabled = true;
    
    mpfnUpdate = NULL;
}

/*
**
*/
CMenuManager::~CMenuManager( void )
{
	
}

/*
**
*/
void CMenuManager::loadMenu( const char* szMenu, const char* szFileName )
{
    OUTPUT( "%s : %d szMenu = %s szFileName = %s\n",
            __FUNCTION__,
            __LINE__,
            szMenu,
           szFileName );
    
	CMenuAnimManager* pAnimManager = CMenuAnimManager::instance();

	char szFullPath[256];
	getFullPath( szFullPath, szFileName );

	TiXmlDocument doc( szFullPath );
	bool bLoaded = doc.LoadFile();
	assert( bLoaded );

	if( bLoaded )
	{
		TiXmlElement* pElement = doc.FirstChildElement();
		TiXmlNode* pLayerNode = pElement->FirstChild();

        // find the right layer to load given the menu name
		while( pLayerNode )
		{
			const char* szName = pLayerNode->FirstChild()->FirstChild()->Value();
			
			if( !strcmp( szName, szMenu ) )
			{
				if( mpLayer )
				{
                    mpCurrScreen->exit();
                    mpCurrScreen = NULL;
					delete mpLayer;
				}
				
				// create new layer
				mpLayer = new CLayer();
				
                //float fScreenWidth = (float)renderGetScreenWidth() * renderGetScreenScale();
                //float fScreenHeight = (float)renderGetScreenHeight() * renderGetScreenScale();
                
                //float fUIScaleX = fScreenWidth / 640.0f;
                //float fUIScaleY = fScreenHeight / 960.0f;
                
                //mpLayer->setScale( fUIScaleX, fUIScaleY );
                
				// get he property of the layer
				TiXmlNode* pPropertyNode = pLayerNode->FirstChild();
				while( pPropertyNode )
				{
					const char* szProperty = pPropertyNode->Value();
					if( !strcmp( szProperty, "filename" ) )
					{
						const char* szFileName = pPropertyNode->FirstChild()->Value();
											
                        // set the correct screen for layer
                        mpCurrScreen = NULL;
                        loadCurrScreen( szMenu, szFileName );
                    }
					else if( !strcmp( szProperty, "enter" ) )
					{
						const char* szFileName = pPropertyNode->FirstChild()->Value();
                        std::vector<CAnimPlayerDB *> animList;
                        pAnimManager->load( szFileName, &animList );
						setAnimPlayerToControl( szFileName, mpLayer, CMenuAnimPlayer::TYPE_ENTER );
                        
                        // check for any particle to add to layer
                        for( size_t i = 0; i < animList.size(); i++ )
                        {
                            const char* szAnimName = animList[i]->getName();
                            if( strstr( szAnimName, ".pex" ) )
                            {
                                // get the position of the particle
                                tVectorAnimFrame animFrame;
                                animList[i]->getFrameAtTime( &animFrame, CAnimPlayerDB::FRAME_POSITION, 0.0f );
                                tVector2 pos = { animFrame.mValue.fX, animFrame.mValue.fY };
                                
                                // get the starting time
                                float fStartTime = 0.0f;
                                for( int j = 0; j < 10000; j++ )
                                {
                                    float fTime = j * 0.01f;
                                    animList[i]->getFrameAtTime( &animFrame, CAnimPlayerDB::FRAME_ACTIVE, fTime );
                                    
                                    if( animFrame.mValue.fX > 0.0f )
                                    {
                                        fStartTime = fTime;
                                        break;
                                    }
                                }
                                
                                // add emitter to layer
                                char szParticleName[256];
                                memset( szParticleName, 0, sizeof( szParticleName ) );
                                const char* szStart = (const char *)( (size_t)strstr( szAnimName, "/" ) + 1 );
                                strncpy( szParticleName, szStart, sizeof( szParticleName ) ); 
                                mpLayer->addEmitter( szParticleName, CMenuAnimPlayer::TYPE_ENTER, &pos, fStartTime );
                            }
                        }
					}
					else if( !strcmp( szProperty, "exit" ) )
					{
						const char* szFileName = pPropertyNode->FirstChild()->Value();
						std::vector<CAnimPlayerDB *> animList;
                        pAnimManager->load( szFileName, &animList );
						setAnimPlayerToControl( szFileName, mpLayer, CMenuAnimPlayer::TYPE_EXIT );
                        
                        // check for any particle to add to layer
                        for( size_t i = 0; i < animList.size(); i++ )
                        {
                            const char* szAnimName = animList[i]->getName();
                            if( strstr( szAnimName, ".pex" ) )
                            {
                                tVectorAnimFrame animFrame;
                                animList[i]->getFrameAtTime( &animFrame, CAnimPlayerDB::FRAME_POSITION, 0.0f );
                                
                                tVector2 pos = { animFrame.mValue.fX, animFrame.mValue.fY };
                                
                                // get the starting time
                                float fStartTime = 0.0f;
                                for( int j = 0; j < 10000; j++ )
                                {
                                    float fTime = j * 0.01f;
                                    animList[i]->getFrameAtTime( &animFrame, CAnimPlayerDB::FRAME_ACTIVE, fTime );
                                    
                                    if( animFrame.mValue.fX > 0.0f )
                                    {
                                        fStartTime = fTime;
                                        break;
                                    }
                                }
                                
                                // add emitter to layer
                                char szParticleName[256];
                                memset( szParticleName, 0, sizeof( szParticleName ) );
                                const char* szStart = (const char *)( (size_t)strstr( szAnimName, "/" ) + 1 );
                                strncpy( szParticleName, szStart, sizeof( szParticleName ) ); 
                                mpLayer->addEmitter( szParticleName, CMenuAnimPlayer::TYPE_EXIT, &pos, fStartTime );
                            }
                        }
					}
					else if( !strcmp( szProperty, "focus" ) )
					{
						const char* szFileName = pPropertyNode->FirstChild()->Value();
                        std::vector<CAnimPlayerDB *> animList;
						pAnimManager->load( szFileName, &animList );
						setAnimPlayerToControl( szFileName, mpLayer, CMenuAnimPlayer::TYPE_FOCUS );
                        
                        // check for any particle to add to layer
                        for( size_t i = 0; i < animList.size(); i++ )
                        {
                            const char* szAnimName = animList[i]->getName();
                            if( strstr( szAnimName, ".pex" ) )
                            {
                                tVectorAnimFrame animFrame;
                                animList[i]->getFrameAtTime( &animFrame, CAnimPlayerDB::FRAME_POSITION, 0.0f );
                                
                                tVector2 pos = { animFrame.mValue.fX, animFrame.mValue.fY };
                                
                                // get the starting time
                                float fStartTime = 0.0f;
                                for( int j = 0; j < 10000; j++ )
                                {
                                    float fTime = j * 0.01f;
                                    animList[i]->getFrameAtTime( &animFrame, CAnimPlayerDB::FRAME_ACTIVE, fTime );
                                    
                                    if( animFrame.mValue.fX > 0.0f )
                                    {
                                        fStartTime = fTime;
                                        break;
                                    }
                                }
                                
                                // add emitter to layer
                                char szParticleName[256];
                                memset( szParticleName, 0, sizeof( szParticleName ) );
                                const char* szStart = (const char *)( (size_t)strstr( szAnimName, "/" ) + 1 );
                                strncpy( szParticleName, szStart, sizeof( szParticleName ) ); 
                                mpLayer->addEmitter( szParticleName, CMenuAnimPlayer::TYPE_FOCUS, &pos, fStartTime );
                            }
                        }
					}
                    else if( !strcmp( szProperty, "normal" ) )
					{
						const char* szFileName = pPropertyNode->FirstChild()->Value();
						std::vector<CAnimPlayerDB *> animList;
                        pAnimManager->load( szFileName, &animList );
						setAnimPlayerToControl( szFileName, mpLayer, CMenuAnimPlayer::TYPE_NORMAL );
                        
                        // check for any particle to add to layer
                        for( size_t i = 0; i < animList.size(); i++ )
                        {
                            const char* szAnimName = animList[i]->getName();
                            if( strstr( szAnimName, ".pex" ) )
                            {
                                tVectorAnimFrame animFrame;
                                animList[i]->getFrameAtTime( &animFrame, CAnimPlayerDB::FRAME_POSITION, 0.0f );
                                
                                tVector2 pos = { animFrame.mValue.fX, animFrame.mValue.fY };
                                
                                // get the starting time
                                float fStartTime = 0.0f;
                                for( int j = 0; j < 10000; j++ )
                                {
                                    float fTime = j * 0.01f;
                                    animList[i]->getFrameAtTime( &animFrame, CAnimPlayerDB::FRAME_ACTIVE, fTime );
                                    
                                    if( animFrame.mValue.fX > 0.0f )
                                    {
                                        fStartTime = fTime;
                                        break;
                                    }
                                }
                                
                                // add emitter to layer
                                char szParticleName[256];
                                memset( szParticleName, 0, sizeof( szParticleName ) );
                                const char* szStart = (const char *)( (size_t)strstr( szAnimName, "/" ) + 1 );
                                strncpy( szParticleName, szStart, sizeof( szParticleName ) ); 
                                mpLayer->addEmitter( szParticleName, CMenuAnimPlayer::TYPE_NORMAL, &pos, fStartTime );
                            }
                        }
					}
                    else if( !strcmp( szProperty, "user0" ) )
                    {
                        const char* szFileName = pPropertyNode->FirstChild()->Value();
						pAnimManager->load( szFileName );
						setAnimPlayerToControl( szFileName, mpLayer, CMenuAnimPlayer::TYPE_USER_0 );
                    }
                    else if( !strcmp( szProperty, "user1" ) )
                    {
                        const char* szFileName = pPropertyNode->FirstChild()->Value();
						pAnimManager->load( szFileName );
						setAnimPlayerToControl( szFileName, mpLayer, CMenuAnimPlayer::TYPE_USER_1 );
                    }
                    else if( !strcmp( szProperty, "user2" ) )
                    {
                        const char* szFileName = pPropertyNode->FirstChild()->Value();
						pAnimManager->load( szFileName );
						setAnimPlayerToControl( szFileName, mpLayer, CMenuAnimPlayer::TYPE_USER_2 );
                    }
                    
					pPropertyNode = pPropertyNode->NextSibling();
				}

				break;
			}

			pLayerNode = pLayerNode->NextSibling();
		}
	
        WTFASSERT2( mpLayer, "no layer: \"%s\"", mszNextMenuFileName );
        mpLayer->setName( szMenu );
	}
    
    assert( mpCurrScreen );
    mpCurrScreen->getLayer()->setAnimType( CMenuAnimPlayer::TYPE_ENTER );
    mpCurrScreen->enter();
    mpCurrScreen->getLayer()->markStartStateTime();
    
}

/*
**
*/
void CMenuManager::setAnimPlayerToControl( const char* szFileName, CLayer* pLayer, int iType )
{
	CMenuAnimManager* pAnimManager = CMenuAnimManager::instance();

	char szFullPath[256];
	getFullPath( szFullPath, szFileName );
	TiXmlDocument animDoc( szFullPath );
	bool bLoaded = animDoc.LoadFile();
	assert( bLoaded );
	
	// set the animation player to the control based on the layer name in the animation file
	if( bLoaded )
	{
		TiXmlElement* pElement = animDoc.FirstChildElement();
		TiXmlNode* pLayerNode = pElement->FirstChild();

		while( pLayerNode )
		{
			const char* szValue = pLayerNode->Value();
			if( !strcmp( szValue, "layer" ) )
			{
				const char* szName = pLayerNode->FirstChild()->FirstChild()->Value();
				
                // rename, changing slash to underscore
                char szRename[256];
                memset( szRename, 0, sizeof( szRename ) );
                strncpy( szRename, szName, sizeof( szRename ) );
                for( unsigned int i = 0; i < strlen( szRename ); i++ )
                {
                    if( szRename[i] == '/' || szRename[i] == '\\' )
                    {
                        szRename[i] = '_';
                    }
                }
                
				// set animation player to control
				CControl* pControl = pLayer->findControl( szRename ); 
                WTFASSERT2( pControl, "%s not found in %s while loading %s", szRename, pLayer->getName(), szFileName );
                
				// filepath for ID
				char szNoExtension[256];
				memset( szNoExtension, 0, sizeof( szNoExtension ) );
				const char* szEnd = strstr( szFileName, "." );
				assert( szEnd );
				memcpy( szNoExtension, szFileName, (size_t)szEnd - (size_t)szFileName );

				char szFullName[256];
				sprintf( szFullName, "%s/%s", szNoExtension, szName );
				
				// animation db
				CAnimPlayerDB* pAnimDB = pAnimManager->getAnimPlayer( szFullName );
				WTFASSERT2( pAnimDB, "animation player %s not found", szFullName );

				// set to animation player
				CMenuAnimPlayer* pAnimPlayer = pControl->getAnimPlayer();
				
				// create new animation player
				if( pAnimPlayer == NULL )
				{
					pAnimPlayer = new CMenuAnimPlayer();
					pControl->setAnimPlayer( pAnimPlayer );
				}

				pAnimPlayer->addAnimDB( pAnimDB, iType );
				pAnimPlayer->setState( CMenuAnimPlayer::STATE_START );
                
                // check for any others in template
                if( strstr( szRename, "template" ) )
                {
                    std::vector<CControl *> controlList;
                    pLayer->findControls( szRename, &controlList, NULL );

                    // get the default anchor point
                    tVectorAnimFrame defaultAnchorPt;
                    tVector2 defaultPos;
                    for( size_t i = 0; i < controlList.size(); i++ )
                    {
                        if( controlList[i]->getAnimPlayer() )
                        {
                            const tVector2 pos = controlList[i]->getParent()->getPosition();
                            pAnimDB->getFrameAtTime( &defaultAnchorPt, CAnimPlayerDB::FRAME_ANCHOR_PT, 0.0f );
                            memcpy( &defaultPos, &pos, sizeof( defaultPos ) );
                            
                            // TODO: fix this for horizontal scroll list
                            // first of the list is -1 in index, so index 0 is dimension increment of the first one
                            defaultPos.fY += controlList[i]->getParent()->getDimension().fY;
                            break;
                        }
                    }
                    
                    // create and add the anim db to the template's player
                    for( size_t i = 0; i < controlList.size(); i++ )
                    {
                        CControl* pTemplateControl = controlList[i];
                        
                        if( pTemplateControl->getAnimPlayer() == NULL )
                        {
                            CMenuAnimPlayer* pTemplateAnimPlayer = new CMenuAnimPlayer();
                            pTemplateControl->setAnimPlayer( pTemplateAnimPlayer );
                            pTemplateAnimPlayer->addAnimDB( pAnimDB, iType );
                            pTemplateAnimPlayer->setState( CMenuAnimPlayer::STATE_START );
                        
                            const tVector2 position = pTemplateControl->getParent()->getPosition();
                            
                            tVector2 offset = 
                            {
                                0.0f,
                                position.fY - defaultPos.fY
                            };
                            pTemplateAnimPlayer->setAnchorOffset( &offset );
                        }
                    }
                }
			}
			
			pLayerNode = pLayerNode->NextSibling();
		}
	}
}

/*
**
*/
void CMenuManager::drawScreen( float fDT )
{
    if( mpCurrScreen == NULL )
    {
        return;
    }
    
    mbFinishedAnimation = true;
	if( mpLayer )
	{
		mpLayer->traverseControls( checkExitedMenu );
        mpLayer->traverseControls( setAppropriateNormalState );
        
		if( mbFinishedAnimation )
		{
            // release current screen
			assert( mpCurrScreen );
            mpCurrScreen->exit();
            mpCurrScreen->releaseLayer();
            mpCurrScreen = NULL;
            mpLayer = NULL;
            
            // load the next menu
            if( strlen( mszNextMenuFileName ) )
            {
                loadMenu( mszNextMenuFileName, "menuinfo.txt" );
                memset( mszNextMenuFileName, 0, sizeof( mszNextMenuFileName ) );
            }
		}
	}
    
    if( mpCurrScreen )
    {
        mpCurrScreen->update( fDT );
        
        if( mpfnUpdate )
        {
            mpfnUpdate();
        }
        
        if( mpCurrScreen )
        {
            mpCurrScreen->draw();
        }
    }
}

/*
**
*/
void CMenuManager::releaseScreen( void )
{
    mpCurrScreen->exit();
    mpCurrScreen->releaseLayer();
    mpCurrScreen = NULL;
    mpLayer = NULL;
}

/*
**
*/
void CMenuManager::releaseLayer( void )
{
	delete mpLayer;
	mpLayer = NULL;
}

/*
**
*/
void CMenuManager::setNextMenuFileName( const char* szFileName )
{
    OUTPUT( "%s : %d szFileName = %s\n",
            __FUNCTION__,
            __LINE__,
            szFileName );
    strncpy( mszNextMenuFileName, szFileName, sizeof( mszNextMenuFileName ) );
}

/*
**
*/
void CMenuManager::checkExitedMenu( CControl* pControl, CLayer* pLayer, void* pUserData )
{
	CMenuAnimPlayer* pAnimPlayer = pControl->getAnimPlayer();
    if( pControl->getState() != CControl::STATE_DISABLED )
    {
        if( pAnimPlayer )
        {	
            // just exit type
            if( pAnimPlayer->getType() == CMenuAnimPlayer::TYPE_EXIT )
            {
                // still playing animation
                if( pAnimPlayer->getTime() < pAnimPlayer->getLastTime( CMenuAnimPlayer::TYPE_EXIT ) )
                {
                    CMenuManager::instance()->setFinishedAllAnimation( false );
                }
            }
            else
            {
                // not exit animation
                if( pAnimPlayer->getType() != CMenuAnimPlayer::TYPE_USER_0 &&
                    pAnimPlayer->getType() != CMenuAnimPlayer::TYPE_USER_1 &&
                    pAnimPlayer->getType() != CMenuAnimPlayer::TYPE_USER_2 )
                {
                    CMenuManager::instance()->setFinishedAllAnimation( false );
                }
            }
        }
    }
}

/*
**
*/
void CMenuManager::setAppropriateNormalState( CControl* pControl, CLayer* pLayer, void* pUserData )
{
    CMenuAnimPlayer* pAnimPlayer = pControl->getAnimPlayer();
	if( pAnimPlayer )
	{
        // done with animation in enter and focus, set state to normal
        if( pAnimPlayer->getType() == CMenuAnimPlayer::TYPE_ENTER )
        {
            float fLastTime = pAnimPlayer->getLastTime( CMenuAnimPlayer::TYPE_ENTER );
            float fAnimTime = pAnimPlayer->getTime();
            if( fAnimTime >= fLastTime )
            {                
                pAnimPlayer->setType( CMenuAnimPlayer::TYPE_NORMAL );
                pAnimPlayer->setState( CMenuAnimPlayer::STATE_START );
                pAnimPlayer->setTime( 0.0f );
            }
        }
        else if( pAnimPlayer->getType() == CMenuAnimPlayer::TYPE_FOCUS )
        {
            // just loop back to normal
            if( pAnimPlayer->getTime() >= pAnimPlayer->getLastTime( CMenuAnimPlayer::TYPE_FOCUS ) )
            {
                pAnimPlayer->setType( CMenuAnimPlayer::TYPE_NORMAL );
                pAnimPlayer->setState( CMenuAnimPlayer::STATE_START );
                pAnimPlayer->setTime( 0.0f );
            }
        }
    }
}

/*
**
*/
void CMenuManager::restart( const char* szStartMenu )
{
    loadMenu( szStartMenu, "menuinfo.txt" );
}

/*
**
*/
void CMenuManager::exitMenu( void )
{
    mpLayer->traverseControls( exitStateForControls );
}

/*
**
*/
void CMenuManager::exitStateForControls( CControl* pControl, CLayer* pLayer, void* pUserData )
{
    if( pControl->getAnimPlayer() )
	{	
		pControl->getAnimPlayer()->setType( CMenuAnimPlayer::TYPE_EXIT );
		pControl->getAnimPlayer()->setState( CMenuAnimPlayer::STATE_START );
	}
}

/*
**
*/
void CMenuManager::loadCurrScreen( const char* szMenu, const char* szFileName )
{
    // look for the appropriate screen
    bool bFound = false;
    for( size_t i = 0; i < mapScreens.size(); i++ )
    {
        CMenuScreen* pScreen = mapScreens[i];
        if( !strcmp( pScreen->getName(), szMenu ) )
        {
            mpCurrScreen = pScreen;
            mpCurrScreen->load( szFileName, mpLayer );
            bFound = true;
            break;
        }
    }
    
    if( !bFound )
    {
        mpCurrScreen = CBlankMenu::instance();
        mpCurrScreen->load( szFileName, mpLayer );
    }
    
    CLayer* pLayer = mpCurrScreen->getLayer();
    pLayer->traverseControls( CMenuManager::setButtonGotoScreen );
}

/*
**
*/
void CMenuManager::setButtonGotoScreen( CControl* pControl, CLayer* pLayer, void* pUserData )
{
    if( pControl->getType() == CControl::TYPE_BUTTON )
    {
        CButton* pButton = static_cast<CButton *>( pControl );
        pButton->setPressedFunc( CMenuManager::gotoNextScreen );
    }
}

/*
**
*/
void CMenuManager::gotoNextScreen( CControl* pControl, void* pUserData )
{
    CButton* pButton = static_cast<CButton *>( pControl );
    const char* szNextScreen = pButton->getNextScreen();
    if( strlen( szNextScreen ) )
    {
        CMenuManager::instance()->setNextMenuFileName( szNextScreen );
        CMenuManager::instance()->exitMenu();
    }
}

/*
**
*/
void CMenuManager::registerScreen( CMenuScreen* pScreen )
{
    mapScreens.push_back( pScreen );
}

/*
**
*/
bool CMenuManager::isInExitState( void )
{
    bool bIsInExit = false;
    mpLayer->traverseControls( checkExitState, NULL, &bIsInExit );
    
    return bIsInExit;
}

/*
**
*/
void CMenuManager::checkExitState( CControl* pControl, CLayer* pLayer, void* pUserData )
{
    bool* pbIsExit = (bool *)pUserData;
    CMenuAnimPlayer* pPlayer = pControl->getAnimPlayer();
    
#if 0
    if( pPlayer && pPlayer->animTypeHasData() )
    {
        int iType = pPlayer->getType();
        float fLastTime = pPlayer->getLastTime( iType );
        float fTime = pPlayer->getTime();
        
        if( fLastTime > fTime )
        {
            *pbIsExit = false;
        }
    }
#endif // #if 0
    
    
    if( pPlayer && pPlayer->getType() == CMenuAnimPlayer::TYPE_EXIT )
    {
        if( pbIsExit )
        {
            *pbIsExit = true;
        }
    }
}

/*
**
*/
CControl* CMenuManager::inputUpdate( float fCursorX,
                                float fCursorY,
                                int iHit,
                                int iNumTouches,
                                int iTouchType )
{
	CControl* pControl = NULL;

    if( mpLayer )
    {
        pControl = mpLayer->inputUpdate( fCursorX,
										 fCursorY,
										 iHit,
										 iNumTouches,
										 iTouchType );
    }

	return pControl;
}
