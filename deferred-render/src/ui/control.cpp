#include "control.h"
#include <algorithm>

/*
**
*/
CControl::CControl( void )
{
	memset( &mPosition, 0, sizeof( mPosition ) );
	memset( &mDimension, 0, sizeof( mDimension ) );

	maUV[0].fX = 0.0f; maUV[0].fY = 0.0f;
	maUV[1].fX = 1.0f; maUV[1].fY = 0.0f;
	maUV[2].fX = 0.0f; maUV[2].fY = 1.0f;
	maUV[3].fX = 1.0f; maUV[3].fY = 1.0f;

	miShaderProgram = 0;
	mpParent = NULL;

	memset( &mAnchorPt, 0, sizeof( mAnchorPt ) );
	mpAnimPlayer = NULL;

	mColor.fX = mColor.fY = mColor.fZ = mColor.fW = 1.0f;
	
	memset( mszName, 0, sizeof( mszName ) );
	miState = STATE_NORMAL;
    miType = TYPE_CONTROL;
    
    Matrix44Identity( &mTotalMatrix );
    
    memset( &mOffset, 0, sizeof( mOffset ) );
}

/*
**
*/
CControl::~CControl( void )
{
	memset( &mPosition, 0, sizeof( mPosition ) );
	memset( &mDimension, 0, sizeof( mDimension ) );
	
	for( unsigned int i = 0; i < mapChildren.size(); i++ )
	{
		delete mapChildren[i];
	}

	if( mpAnimPlayer )
	{
		delete mpAnimPlayer;
		mpAnimPlayer = NULL;
	}
}

/*
**
*/
void CControl::setState( int iState )
{
	miState = iState;

	for( unsigned int i = 0; i < mapChildren.size(); i++ )
	{
		mapChildren[i]->setState( iState );
	}
}

/*
**
*/
void CControl::setPosition( const tVector2& position )
{
	memcpy( &mPosition, &position, sizeof( mPosition ) );
}

/*
**
*/
void CControl::setDimension( const tVector2& dimension )
{
	memcpy( &mDimension, &dimension, sizeof( mDimension ) );
}

/*
**
*/
void CControl::setOrigPosition( const tVector2& position )
{
    memcpy( &mOrigPosition, &position, sizeof( mOrigPosition ) );
}

/*
**
*/
void CControl::setName( const char* szName )
{
	strncpy( mszName, szName, sizeof( mszName ) );
}

/*
**
*/
CControl* CControl::inputUpdate( float fCursorX, 
						   float fCursorY, 
						   int iNumTouches,
						   int iTouchType,
                           CControl* pLastInteract,
						   CControl* pParent )
{
	if( miState == STATE_DISABLED )
    {
        return 0;
    }
    
    updateMatrix( 0.0 );
    
    CControl* pRet = NULL;
	if( pParent )
	{
		for( int i = pParent->getNumChildren() - 1; i >= 0; i-- )
		{
			CControl* pChild  = pParent->getChild( i );
			pRet = pChild->inputUpdate( fCursorX, fCursorY, iNumTouches, iTouchType, pLastInteract, this );
			if( pRet )
			{
				break;
			}
		}
	}
	else
	{
		for( int i = (int)mapChildren.size() - 1; i >= 0; i-- )
		{
			CControl* pChild = mapChildren[i];
			pRet = pChild->inputUpdate( fCursorX, fCursorY, iNumTouches, iTouchType, pLastInteract, this );
			if( pRet )
			{
				break;
			}
		}
	}

	return pRet;
}

/*
**
*/
void CControl::addChild( CControl* pControl )
{
	mapChildren.push_back( pControl );
	pControl->setParent( this );
}

/*
**
*/
void CControl::setShader( unsigned int iShader )
{
	miShaderProgram = iShader;
	for( unsigned int i = 0; i < mapChildren.size(); i++ )
	{
		mapChildren[i]->setShader( iShader );
	}

    miShaderPos = glGetAttribLocation( miShaderProgram, "position" );
    miShaderColor = glGetAttribLocation( miShaderProgram, "color" );
    miShaderUV = glGetAttribLocation( miShaderProgram, "textureUV" );
}

/*
**
*/
void CControl::draw( double fDT, int iScreenWidth, int iScreenHeight )
{    
    if( miState == STATE_DISABLED )
	{
		return;
	}
    
    updateMatrix( fDT );
	for( unsigned int i = 0; i < mapChildren.size(); i++ )
	{
        CControl* pChild = mapChildren[i];
        
		//mapChildren[i]->setAlpha( fAlpha );
		pChild->draw( fDT, iScreenWidth, iScreenHeight );
	}
    
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
void CControl::reverseChildren( void )
{
	std::reverse( mapChildren.begin(), mapChildren.end() );
}

/*
**
*/
void CControl::setAnimState( int iState )
{
    if( mpAnimPlayer == NULL )
    {
        return;
    }
    
    mpAnimPlayer->setState( iState );

    for( unsigned int i = 0; i < mapChildren.size(); i++ )
    {
        mapChildren[i]->setAnimState( iState );
    }
}

/*
**
*/
CControl* CControl::copy( void )
{
    CControl* pDestination = new CControl();
    
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

/*
**
*/
void CControl::copyControl( CControl* pDestination )
{
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
}

/*
**
*/
CControl* CControl::find( const char* szName )
{
    for( size_t i = 0; i < mapChildren.size(); i++ )
    {
        const char* szChildName = mapChildren[i]->getName();
        if( !strcmp( szChildName, szName ) )
        {
            return mapChildren[i];
        }
        
        CControl* pControl = mapChildren[i]->find( szName );
        if( pControl )
        {
            return pControl;
        }
    }
    
    return NULL;
}

/*
**
*/
void CControl::setAlpha( float fAlpha )
{
    mColor.fW = fAlpha;
    for( size_t i = 0; i < mapChildren.size(); i++ )
    {
        mapChildren[i]->setAlpha( fAlpha );
    }
}

/*
**
*/
void CControl::setScreenPos( tVector2& screenPos )
{
    memcpy( &mScreenPos, &screenPos, sizeof( mScreenPos ) );
}

/*
**
*/
tVector2& CControl::getScreenPos( void )
{
    tVector2 screenPos = { 0.0f, 0.0f };
    CControl* pParent = mpParent;
    while( pParent )
    {
        tVector2 parentPos = pParent->getPosition();
        screenPos.fX += parentPos.fX;
        screenPos.fY += parentPos.fY;
        
        pParent = pParent->getParent();
    }
    
    mScreenPos.fX = screenPos.fX + mPosition.fX;
    mScreenPos.fY = screenPos.fY + mPosition.fY;
    
    return mScreenPos;
}

/*
**
*/
void CControl::screenToLocal( tVector2* pScreenPos, tVector2* pLocalPos )
{
    tVector2 screenPos = { 0.0f, 0.0f };
    CControl* pParent = mpParent;
    while( pParent )
    {
        tVector2 parentPos = pParent->getPosition();
        screenPos.fX += parentPos.fX;
        screenPos.fY += parentPos.fY;
        
        pParent = pParent->getParent();
    }
    
    pLocalPos->fX = pScreenPos->fX - screenPos.fX;
    pLocalPos->fY = pScreenPos->fY - screenPos.fY;
}

/*
**
*/
void getChildBottomRight( CControl* pControl, tVector2* pBottomRight, tVector2* pParentPos )
{
    float fX = pParentPos->fX + pControl->getPosition().fX;
    float fY = pParentPos->fY + pControl->getPosition().fY;
    
    float fBottomRightX = fX + pControl->getDimension().fX * 0.5f;
    float fBottomRightY = fY + pControl->getDimension().fY * 0.5f;
    
    tVector2 pos = { fX, fY };
    if( fBottomRightX > pBottomRight->fX )
    {
        pBottomRight->fX = fBottomRightX;
    }
    
    if( fBottomRightY > pBottomRight->fY )
    {
        pBottomRight->fY = fBottomRightY;
    }
    
    for( int i = 0; i < (int)pControl->getNumChildren(); i++ )
    {
        CControl* pChild = pControl->getChild( i );
        getChildBottomRight( pChild, pBottomRight, &pos );
    }
}

/*
**
*/
void CControl::getBounds( tVector2* pBounds )
{
    pBounds->fX = mDimension.fX;
    pBounds->fY = mDimension.fY;
    
    tVector2 bottomRight = { mPosition.fX + mDimension.fX * 0.5f, mPosition.fY + mDimension.fY * 0.5f };
    
    for( size_t i = 0; i < mapChildren.size(); i++ )
    {
        tVector2 parentPos = { mPosition.fX, mPosition.fY };
        getChildBottomRight( mapChildren[i], &bottomRight, &parentPos );
    }
}

/*
**
*/
void CControl::updateMatrix( double fDT )
{
    float fScaleX = 1.0f;
	float fScaleY = 1.0f;
	float fScaleZ = 1.0f;
	float fAngle = 0.0f;
	
	float fTransX = 0.0f;
	float fTransY = 0.0f;
	
	float fAnchorPtX = 0.0f;
	float fAnchorPtY = 0.0f;
	
	float fAlpha = 1.0f;
	
	// get animation to apply with the total matrix
	if( mpAnimPlayer )
	{
        bool bUpdateAnimation = false;
    
        // normal animation
        if( mpAnimPlayer->getType() == CMenuAnimPlayer::TYPE_NORMAL &&
           mpAnimPlayer->animTypeHasData() )
        {
            mpAnimPlayer->setLoopType( true, CMenuAnimPlayer::TYPE_NORMAL );
        }
        
		// play animation 
		if( mpAnimPlayer->animTypeHasData() &&
           ( mpAnimPlayer->getState() == CMenuAnimPlayer::STATE_START || mpAnimPlayer->getState() == CMenuAnimPlayer::STATE_PLAYING ) )
		{
            mpAnimPlayer->update( fDT );
            bUpdateAnimation = true;
		}
        else if( mpAnimPlayer->animTypeHasData() &&
                mpAnimPlayer->getState() == CMenuAnimPlayer::STATE_STOP )
        {
            // just play the start of the animation
            bUpdateAnimation = true;
        }
		else
		{
			if( mpAnimPlayer->getType() == CMenuAnimPlayer::TYPE_EXIT )
			{
                // hold last frame of animation
				if( mpAnimPlayer->getState() == CMenuAnimPlayer::STATE_STOP )
                {
                    mpAnimPlayer->setTime( mpAnimPlayer->getLastTime( CMenuAnimPlayer::TYPE_EXIT ) );
                    bUpdateAnimation = true;
                }
			}
		}
        
        // update animation
        if( bUpdateAnimation && mpAnimPlayer->animTypeHasData() )
        {
            tVector4 scale, translation, anchorPt, color;
            
            mpAnimPlayer->getScaling( &scale );
            mpAnimPlayer->getRotation( &fAngle );
            mpAnimPlayer->getTranslation( &translation );
            mpAnimPlayer->getAnchorPoint( &anchorPt );
            mpAnimPlayer->getColor( &color );
            
            tVector2* pAnchorOffset = mpAnimPlayer->getAnchorOffset();
            anchorPt.fX += pAnchorOffset->fX;
            anchorPt.fY += pAnchorOffset->fY;
            
            translation.fX += pAnchorOffset->fX;
            translation.fY += pAnchorOffset->fY;
            
            //mpAnimPlayer->getActive( &bActive );
            
            fScaleX = scale.fX;
            fScaleY = scale.fY;
            fScaleZ = scale.fZ;
            
            float fHalfStageWidth = mpAnimPlayer->getStageWidth() * 0.5f;
            float fHalfStageHeight = mpAnimPlayer->getStageHeight() * 0.5f;
            
            // offset to the center of the stage then apply translation from the center
            fTransX = ( fHalfStageWidth - mPosition.fX ) + ( translation.fX - fHalfStageWidth );
            fTransY = ( fHalfStageHeight - mPosition.fY ) + ( translation.fY - fHalfStageHeight );
            
            fAnchorPtX = mPosition.fX - anchorPt.fX;
            fAnchorPtY = mPosition.fY - anchorPt.fY;
            
            fAlpha = color.fX * 0.01f;
            
            // animation alpha
            setAlpha( fAlpha );

        }
	}   // if valid animation player
    
    // position with animation translation
	tMatrix44 translationMat, rotationMat, scaleMat, anchorMat, transRotMat, transRotScaleMat, animMatrix;
	tVector4 position = { mPosition.fX + fTransX + mOffset.fX, mPosition.fY + fTransY + mOffset.fY, 0.0f, 1.0f };
	
	// anchor point from animation
	tVector4 anchorPt = { mAnchorPt.fX + fAnchorPtX, mAnchorPt.fY + fAnchorPtY };
	Matrix44Translate( &anchorMat, &anchorPt );
	
	// translate, rotate, scale
	Matrix44Translate( &translationMat, &position );
	Matrix44RotateZ( &rotationMat, fAngle );
	Matrix44Scale( &scaleMat, fScaleX, fScaleY, fScaleZ );
	
	// total animation matrix
	Matrix44Multiply( &transRotMat, &translationMat, &rotationMat );
	Matrix44Multiply( &transRotScaleMat, &transRotMat, &scaleMat );
	Matrix44Multiply( &animMatrix, &transRotScaleMat, &anchorMat );
	
	tMatrix44 parentMatrix;
	if( mpParent )
	{
		parentMatrix = mpParent->getMatrix();
	}
	else
	{
		Matrix44Identity( &parentMatrix );
	}
	
	// concat with parent matrix
	Matrix44Identity( &mTotalMatrix );
	Matrix44Multiply( &mTotalMatrix, &animMatrix, &parentMatrix );
    
    // update emitter
    if( mpAnimPlayer )
    {
        CEmitter* pEmitter = mpAnimPlayer->getEmitter();
        if( pEmitter )
        {
            pEmitter->setFullScreen( true );
            
            tVector4 sourcePos =
            {
                mTotalMatrix.M( 0, 3 ) * 0.5f,
                320.0f - mTotalMatrix.M( 1, 3 ) * 0.5f,
                0.0f,
                1.0f
            };
            
            pEmitter->setSourcePosition( &sourcePos );
            pEmitter->update( fDT );
        }
    }
}

/*
**
*/
void CControl::traverse( CControl* pControl,  void (*pFunc)( CControl* pControl, void* pUserData ), void* pUserData )
{
    for( size_t i = 0; i < mapChildren.size(); i++ )
    {
        CControl* pChild = mapChildren[i];
        pFunc( pChild, pUserData );
        
        pChild->traverse( pChild, pFunc, pUserData );
    }
}

/*
**
*/
tVector2 CControl::getAbsolutePosition( void )
{
	CControl* pParent = mpParent;
	tVector2 pos =
	{
		mPosition.fX,
		mPosition.fY
	};

	while( pParent )
	{
		tVector2 parentPos = pParent->getPosition();
		pos.fX += parentPos.fX;
		pos.fY += parentPos.fY;

		pParent = pParent->getParent();
	}

	return pos;
}