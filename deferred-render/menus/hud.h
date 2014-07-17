#ifndef __HUD_H__
#define __HUD_H__

#include "layer.h"

class CHUD : public CLayer
{
public:
	CHUD( void );
	~CHUD( void );
	
	void enter( void );
	void update( void );

	void setShaderName( const char* szShaderName );

	static CHUD* instance( void );
	
protected:
	CControl*			mpOptionsMenu;
	CButton*			mpOptionsButton;
	CButton*			mpShaderButton;

	static CHUD*		mpInstance;

protected:
	static void showOptionsMenu( CControl* pControl, void* pData );
	static void toggleShaders( CControl* pControl, void* pData );
	static void closeOptionMenu( CControl* pControl, void* pData );
	
};

#endif // __HUD_H__