#include "hud.h"
#include "game.h"
#include "render.h"

/*
**
*/
CHUD* CHUD::mpInstance = NULL;
CHUD* CHUD::instance( void )
{
	if( mpInstance == NULL )
	{
		mpInstance = new CHUD();
	}

	return mpInstance;
}

/*
**
*/
CHUD::CHUD( void )
{

}

/*
**
*/
CHUD::~CHUD( void )
{
	
}

/*
**
*/
void CHUD::enter( void )
{
	mpOptionsMenu = findControl( "option_menu" );
	WTFASSERT2( mpOptionsMenu, "can't find option_menu" );
	mpOptionsMenu->setState( CControl::STATE_DISABLED );

	mpOptionsButton = static_cast<CButton *>( findControl( "options" ) );
	WTFASSERT2( mpOptionsMenu, "can't find options" );
	mpOptionsButton->setPressedFunc( showOptionsMenu );

	mpShaderButton = static_cast<CButton *>( findControl( "option_menu_shader" ) );
	WTFASSERT2( mpShaderButton, "can't find option_menu_shader" );
	mpShaderButton->setPressedFunc( toggleShaders );

	CButton* pButton = static_cast<CButton *>( findControl( "option_menu_close" ) );
	pButton->setPressedFunc( closeOptionMenu );
}

/*
**
*/
void CHUD::update( void )
{
	
}

/*
**
*/
void CHUD::showOptionsMenu( CControl* pControl, void* pData )
{
	CHUD::instance()->findControl( "option_menu" )->setState( CControl::STATE_NORMAL );
}

/*
**
*/
void CHUD::toggleShaders( CControl* pControl, void* pData )
{
	CGame::instance()->toggleShader();
}

/*
**
*/
void CHUD::closeOptionMenu( CControl* pControl, void* pData )
{
	CHUD::instance()->findControl( "option_menu" )->setState( CControl::STATE_DISABLED );
}

/*
**
*/
void CHUD::setShaderName( const char* szShaderName )
{
	CText* pText = static_cast<CText *>( findControl( "option_menu_shader_text" ) );
	WTFASSERT2( pText, "can't find option_menu_shader_text" );
	pText->setText( szShaderName );
}