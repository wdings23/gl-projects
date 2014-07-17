#include "button.h"
#include "controller.h"

/*
**
*/
CButton::CButton( void )
{
	mpPressFunc = NULL;
    miType = CControl::TYPE_BUTTON;
    
    memset( mszNextScreen, 0, sizeof( mszNextScreen ) );
    mpUserData = NULL;
}

/*
**
*/
CButton::~CButton( void )
{

}

/*
**
*/
void CButton::draw( double fTime, int iScreenWidth, int iScreenHeight )
{
    CControl::draw( fTime, iScreenWidth, iScreenHeight );
}


/*
**
*/
CControl* CButton::inputUpdate( float fCursorX,
						  float fCursorY,
						  int iNumTouches,
						  int iTouchType,
                          CControl* pLastInteract,
						  CControl* pParent )
{
    // only can interact on normal animation
    bool bCanInteract = true;
    if( mpAnimPlayer )
    {
        if( mpAnimPlayer->getType() != CMenuAnimPlayer::TYPE_NORMAL )
        {
            bCanInteract = false;
        }
    }
    
    if( bCanInteract )
    {
        bCanInteract = ( miState == CControl::STATE_NORMAL );
    }
    
    // save the first touch to check if the user has moved
    if( iTouchType == TOUCHCONTROL_BEGAN )
    {
        mTouchDownPos.fX = fCursorX;
        mTouchDownPos.fY = fCursorY;
    }
    
    // check if touch has been moved
    if( iTouchType == TOUCHCONTROL_ENDED )
    {
        tVector2 diff =
        {
            fCursorX - mTouchDownPos.fX,
            fCursorY - mTouchDownPos.fY
        };
        
        float fLength = diff.fX * diff.fX + diff.fY * diff.fY;
        if( fLength >= 300.0f )
        {
            bCanInteract = false;
        }
    }
    
	CControl* pRet = NULL;
	if( bCanInteract && intersect( fCursorX, fCursorY ) )
	{
		pRet = this;
		//OUTPUT( "button %s pressed\n", mszName );
        
        // focus animation
        if( iTouchType == TOUCHCONTROL_ENDED )
        {
            if( pLastInteract == this )
            {
                if( mpAnimPlayer && mpAnimPlayer->animTypeHasData( CMenuAnimPlayer::TYPE_FOCUS ) )
                {
                    mpAnimPlayer->setType( CMenuAnimPlayer::TYPE_FOCUS );
                    mpAnimPlayer->setState( CMenuAnimPlayer::STATE_START );
                    mpAnimPlayer->setFinishFunc( animFinished, this );
                    
                    CAnimPlayerDB const* pAnimDB = mpAnimPlayer->getAnimDB( CMenuAnimPlayer::TYPE_FOCUS );
                    const char* szSound = ((CAnimPlayerDB *)pAnimDB)->getSound();
                    if( szSound[0] == '\0' )
                    {
                
                    }
                }
                else
                {
                    if( mpPressFunc )
                    {
                        mpPressFunc( this, mpUserData );
                    }
                }
            }
        }
        
	}	// if intersect

	return pRet;
}

/*
**
*/
int CButton::intersect( float fX, float fY )
{
	tMatrix44 translation;
	tVector4 position = { mPosition.fX + mOffset.fX, mPosition.fY + mOffset.fY, 0.0f, 1.0f };
	Matrix44Translate( &translation, &position );
	
	tMatrix44 parentMatrix;
	if( mpParent )
	{
		parentMatrix = mpParent->getMatrix();
	}
	else
	{
		Matrix44Identity( &parentMatrix );
	}

	Matrix44Identity( &mTotalMatrix );
	Matrix44Multiply( &mTotalMatrix, &translation, &parentMatrix );
	
	tVector2 newPos = 
	{
		mTotalMatrix.M( 0, 3 ),
		mTotalMatrix.M( 1, 3 )
	};
    
    if( mpAnimPlayer )
    {
        tVector4 translation = { 0.0f, 0.0f, 0.0f, 1.0f };
        
        if( mpAnimPlayer->animTypeHasData() )
        {
            tVector4 anchorPt = { 0.0f, 0.0f, 0.0f, 1.0f };
            
            mpAnimPlayer->getTranslation( &translation );
            mpAnimPlayer->getAnchorPoint( &anchorPt );
            
            float fTransX = translation.fX - anchorPt.fX;
            float fTransY = translation.fY - anchorPt.fY;
            
            newPos.fX += fTransX;
            newPos.fY += fTransY;
        }
    }
    
	int iRet = 0;
	if( fX + BUTTON_TOUCHRANGE_X >= newPos.fX - mDimension.fX * 0.5f && fX - BUTTON_TOUCHRANGE_X <= newPos.fX + mDimension.fX * 0.5f &&
		fY + BUTTON_TOUCHRANGE_Y >= newPos.fY - mDimension.fY * 0.5f && fY - BUTTON_TOUCHRANGE_Y <= newPos.fY + mDimension.fY * 0.5f )
	{
		iRet = 1;

	}
	
	return iRet;
}

/*
**
*/
void CButton::animFinished( void* pData )
{
    CButton* pButton = (CButton *)pData;
    if( pButton->mpPressFunc )
    {
        pButton->mpPressFunc( pButton, pButton->getUserData() );
        //pButton->setPressedFunc( NULL );
    }
}

/*
**
*/
void CButton::setNextScreen( const char* szNextScreen )
{
    strncpy( mszNextScreen, szNextScreen, sizeof( mszNextScreen ) );
}

/*
**
*/
CButton* CButton::copy( void )
{
    CButton* pDestination = new CButton();
    
    memcpy( &pDestination->mszName, mszName, sizeof( pDestination->mszName ) );
    
    memcpy( &pDestination->mPosition, &mPosition, sizeof( pDestination->mPosition ) );
    memcpy( &pDestination->mDimension, &mDimension, sizeof( pDestination->mDimension ) );
    memcpy( &pDestination->maUV, maUV, sizeof( pDestination->maUV ) );
    memcpy( &pDestination->mColor, &mColor, sizeof( pDestination->mColor ) );
    memcpy( &pDestination->mAnchorPt, &mAnchorPt, sizeof( pDestination->mAnchorPt ) );
    memcpy( &pDestination->mTotalMatrix, &mTotalMatrix, sizeof( pDestination->mTotalMatrix ) );
    
    pDestination->miType = miType;
    pDestination->miShaderProgram = miShaderProgram;
    pDestination->miState = miState;
    
    if( mpAnimPlayer )
    {
        CMenuAnimPlayer* pAnimPlayer = new CMenuAnimPlayer();
        mpAnimPlayer->copy( pAnimPlayer );
    }
    
    for( size_t i = 0; i < mapChildren.size(); i++ )
    {
        CControl* pChild = mapChildren[i];
        
        CControl* pCopyChild = pChild->copy();
        pDestination->addChild( pCopyChild );
    }
    
    return pDestination;
}

