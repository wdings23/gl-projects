//
//  scrolllist.cpp
//  Game1
//
//  Created by Dingwings on 2/9/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "scrolllist.h"
#include "text.h"
#include "sprite.h"
#include "controller.h"
#include "render.h"
#include "timeutil.h"

#define LIST_SPACING            10.0f
#define VELOCITY_DECAY_PCT      0.95f

#define SCROLL_BACK_TIME    0.25f


/*
**
*/
CScrollList::CScrollList( void )
{
    miType = CControl::TYPE_SCROLLLIST;
    miDisplayType = TYPE_VERTICAL;
    
    Matrix44Identity( &mTotalMatrix );
    miTop = 0;
    
    mVelocity.fX = 0.0f;
    mVelocity.fY = 0.0f;
    
    mPositionOffset.fX = 0.0f;
    mPositionOffset.fY = 0.0f;
    
    miTouchType = TOUCHCONTROL_ENDED;
    mpfnFillData = NULL;
    miStartList = 0;
    miNumEntries = 100;
    
    mfLastTime = 0.0;
    
    memset( &mScrollBackVelocity, 0, sizeof( mScrollBackVelocity ) );
    mfStartScrollBackTime = -1.0f;
    
    mbWrap = false;
    mfVelocityDecay = VELOCITY_DECAY_PCT;
    mpfnAtStart = NULL;
}

/*
**
*/
CScrollList::~CScrollList( void )
{
    
}

/*
**
*/
void CScrollList::draw( double fDT, int iScreenWidth, int iScreenHeight )
{
    if( miState == STATE_DISABLED )
	{
		return;
	}
    
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
        }
	}   // if valid animation player
    
    updatePositions();
    
	tMatrix44 translationMat, rotationMat, scaleMat, anchorMat, transRotMat, transRotScaleMat, animMatrix;
    tVector4 position = { mPosition.fX + fTransX + mOffset.fX, mPosition.fY + fTransY + mOffset.fY, 0.0f, 1.0f };
    tVector4 anchorPt = { mAnchorPt.fX + fAnchorPtX, mAnchorPt.fY + fAnchorPtY };
    
	// transformations
	Matrix44Translate( &translationMat, &position );
	Matrix44RotateZ( &rotationMat, fAngle );
	Matrix44Scale( &scaleMat, fScaleX, fScaleY, fScaleZ );
	Matrix44Translate( &anchorMat, &anchorPt );
    
	// calculate total animation matrix
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
    
	// concat with parent
	Matrix44Identity( &mTotalMatrix );
	Matrix44Multiply( &mTotalMatrix, &animMatrix, &parentMatrix );

    // get the original position, concatenating with parent
    tVector4 currOrigPos =
    {
        parentMatrix.M( 0, 3 ) + mOrigPosition.fX + mOffset.fX,
        parentMatrix.M( 1, 3 ) + mOrigPosition.fY + mOffset.fY,
        0.0f,
        1.0f
    };
    
    // origin is bottom-left
    int iX = (int)( currOrigPos.fX ) - (int)( mDimension.fX * 0.5f );
    int iY = (int)( (float)iScreenHeight - ( currOrigPos.fY + mDimension.fY * 0.5f ) );
    
    // scissor
#if defined( UI_USE_BATCH )
    renderSetToUseScissorBatch( true,
                                iX,
                                iY,
                                (int)mDimension.fX,
                                (int)mDimension.fY );
#else

    glEnable( GL_SCISSOR_TEST );
    glScissor( iX,
               iY,
               (int)mDimension.fX,
               (int)mDimension.fY );
    
#endif // UI_USE_BATCH
    
    for( int i = 0; i < miStartList; i++ )
    {
        mapChildren[i]->setAlpha( fAlpha );
        mapChildren[i]->draw( fDT, iScreenWidth, iScreenHeight );
    }
    
	size_t iNumEntries = miStartList + miNumEntries + 1;
	if( iNumEntries > mapChildren.size() )
	{
		iNumEntries = mapChildren.size();
	}

    // draw all the children
    for( size_t i = miStartList; i < iNumEntries; i++ )
    {
        mapChildren[i]->setAlpha( fAlpha );
        mapChildren[i]->draw( fDT, iScreenWidth, iScreenHeight );
    }
    
#if defined( UI_USE_BATCH )
    renderSetToUseScissorBatch( false, 0, 0, 0, 0 );
#else
    glDisable( GL_SCISSOR_TEST );
#endif // UI_USE_BATCH
}

/*
**
*/
void CScrollList::setTemplate( CControl* pControl )
{
    miStartList = (int)mapChildren.size();
    
    tVector2 dimension = pControl->getDimension();
    if( miDisplayType == TYPE_VERTICAL )
    {
        miMaxEntries = (int)ceil( mDimension.fY / ( dimension.fY + LIST_SPACING ) );
    }
    else
    {
        miMaxEntries = (int)ceil( mDimension.fX / ( dimension.fX + LIST_SPACING ) );
    }
    
    // extras top and bottom
    //miMaxEntries += 2;
    
    // add to children
    tVector2 pos = { 0.0f, -dimension.fY };
    if( miDisplayType == TYPE_HORIZONTAL )
    {
        pos.fX = -dimension.fX;
        pos.fY = 0.0f;
    }
    
    // add children
    for( int i = 0; i < miMaxEntries + 2; i++ )
    {
        // copy template
        CControl* pCopy = pControl->copy();
        pCopy->setPosition( pos );        
        addChild( pCopy );
        
        // update position
        if( miDisplayType == TYPE_VERTICAL )
        {
            pos.fY += ( dimension.fY + LIST_SPACING );
        }
        else
        {
            pos.fX += ( dimension.fX + LIST_SPACING );
        }
    }   // for i = 0 to max entries
}

/*
**
*/
CControl* CScrollList::inputUpdate( float fCursorX,
                                    float fCursorY,
                                    int iNumTouches,
                                    int iTouchType,
                                    CControl* pLastInteract,
                                    CControl* pParent )
{
    if( miState == CControl::STATE_DISABLED ||
        miState == CControl::STATE_INPUT_DISABLED )
    {
        return NULL;
    }
    
    CControl* pRet = NULL;
    const float fUIScale = 0.5f;
    
    // input for children
    for( unsigned int i = 0; i < mapChildren.size(); i++ )
    {
        pRet = mapChildren[i]->inputUpdate( fCursorX, fCursorY, iNumTouches, iTouchType, pLastInteract );
        if( pRet )
        {
            break;
        }
    }
    
    // only if children have no registered input
    if( pRet == NULL )
    {
        if( iTouchType == TOUCHCONTROL_BEGAN )
        {
            mfLastTime = getCurrTime();
        }
        
        {
            float fX = mTotalMatrix.M( 0, 3 );
            float fY = mTotalMatrix.M( 1, 3 );
            
            // out of bounds
            if( fCursorX < fX - mDimension.fX * 0.5f || fCursorX > fX + mDimension.fX * 0.5f ||
                fCursorY < fY - mDimension.fY * 0.5f || fCursorY > fY + mDimension.fY * 0.5f )
            {
                if( iTouchType == TOUCHCONTROL_ENDED )
                {
                    if( mLastTouchPt.fX < 0.0f )
                    {
                        mVelocity.fX = 0.0f; 
                        mVelocity.fY = 0.0f; 
                    }
                    else
                    {
                        mVelocity.fX = ( fCursorX - mLastTouchPt.fX ) * fUIScale;
                        mVelocity.fY = ( fCursorY - mLastTouchPt.fY ) * fUIScale;
                    }
                    
                    mLastTouchPt.fX = -1.0f;
                    mLastTouchPt.fY = -1.0f;
                    
                    //mLastTouchPt.fX = fCursorX;
                    //mLastTouchPt.fY = fCursorY;
                    
                    miTouchType = TOUCHCONTROL_ENDED;
                }
                else
                {
                    mVelocity.fX = 0.0f;
                    mVelocity.fY = 0.0f;
                }
                
                return 0;
            }
            
            miTouchType = iTouchType;
            
            // calculate velocity
            if( iTouchType == TOUCHCONTROL_BEGAN )
            {
                mStartTouchPt.fX = fCursorX;
                mStartTouchPt.fY = fCursorY;
                
                mLastTouchPt.fX = fCursorX;
                mLastTouchPt.fY = fCursorY;
                
                mVelocity.fX = 0.0f;
                mVelocity.fY = 0.0f;
            
                mScrollBackVelocity.fX = 0.0f;
                mScrollBackVelocity.fY = 0.0f;
        
                //memcpy( &mPosition, &mOrigPosition, sizeof( mPosition ) );
                
                pRet = this;
            }
            else if( iTouchType == TOUCHCONTROL_MOVED )
            {
                mVelocity.fX = ( fCursorX - mLastTouchPt.fX ) * fUIScale;
                mVelocity.fY = ( fCursorY - mLastTouchPt.fY ) * fUIScale;
                
                mLastTouchPt.fX = fCursorX;
                mLastTouchPt.fY = fCursorY;
                
                OUTPUT( "%s : %d velocity ( %f, %f )\n",
                        __FUNCTION__,
                        __LINE__,
                        mVelocity.fX,
                        mVelocity.fY );
                
                // don't move if at end
                if( miDisplayType == TYPE_VERTICAL )
                {
                    if( mVelocity.fY > 0.0f )
                    {
                        if( miTop - 1 < 0 )
                        {
                            // move just the list
                            //mPosition.fY += mVelocity.fY;
                            
                            mVelocity.fY = 0.0f;
                        }
                    }
                    else if( mVelocity.fY < 0.0f )
                    {
                        if( miTop + miMaxEntries > miNumEntries )
                        {
                            // move just the list
                            //mPosition.fY += mVelocity.fY;
                            
                            mVelocity.fY = 0.0f;
                        }
                    }
                }
                
                mPositionOffset.fX += mVelocity.fX;
                mPositionOffset.fY += mVelocity.fY;
                
                pRet = this;
            }
            else if( iTouchType == TOUCHCONTROL_ENDED )
            {
                double fCurrTime = getCurrTime();
                double fDuration = fCurrTime - mfLastTime;
                float fSpeed = (float)( 1.0 / fDuration ) * 8.0f;
                
                // check if it's actually scrolling 
                float fStartDX = fCursorX - mStartTouchPt.fX;
                float fStartDY = fCursorY - mStartTouchPt.fY;
                float fLength = sqrtf( fStartDX * fStartDX + fStartDY * fStartDY );
                if( fLength <= 20.0f )
                {
                    fSpeed = 0.0f;
                }
                
                if( mLastTouchPt.fX >= 0.0f && mLastTouchPt.fY >= 0.0f )
                {
                    mVelocity.fX = ( fCursorX - mLastTouchPt.fX );
                    mVelocity.fY = ( fCursorY - mLastTouchPt.fY );
                    
                    float fLength = sqrtf( mVelocity.fX * mVelocity.fX + mVelocity.fY * mVelocity.fY );
                    if( fabs( fLength ) < 5.0f )
                    {
                        mVelocity.fX = 0.0f;
                        mVelocity.fY = 0.0f;
                        
                        fSpeed = 0.0f;
                    }
                    else
                    {
                        mVelocity.fX /= fLength;
                        mVelocity.fY /= fLength;
                    }
                    
                    mVelocity.fX *= fSpeed;
                    mVelocity.fY *= fSpeed;
                }
                else
                {
                    mVelocity.fX = 0.0f;
                    mVelocity.fY = 0.0f;
                }
                
                // don't move if at end
                if( miDisplayType == TYPE_VERTICAL )
                {
                    if( mVelocity.fY > 0.0f )
                    {
                        if( miTop - 1 < 0 )
                        {
                            mVelocity.fY = 0.0f;
                        }
                    }
                    else if( mVelocity.fY < 0.0f )
                    {
                        if( miTop + miMaxEntries > miNumEntries )
                        {
                            mVelocity.fY = 0.0f;
                        }
                    }
                }
                
                mLastTouchPt.fX = -1.0f;
                mLastTouchPt.fY = -1.0f;
                
                if( fabs( mPosition.fX - mOrigPosition.fX ) > 0.001f ||
                    fabs( mPosition.fY - mOrigPosition.fY ) > 0.001f )
                {
                    mScrollBackVelocity.fX = mPosition.fX - mOrigPosition.fX;
                    mScrollBackVelocity.fY = mPosition.fY - mOrigPosition.fY;
                    mfStartScrollBackTime = getCurrTime();
                    
                    //memcpy( &mPosition, &mOrigPosition, sizeof( mPosition ) );
                }
                
                pRet = this;
            }
        }
    }   // if no input register from children
    
    return pRet;
}

/*
**
*/
void CScrollList::updatePositions( void )
{
    tVector2 dimension = mapChildren[miStartList]->getDimension();
    
    // velocity decay
    if( miTouchType == TOUCHCONTROL_ENDED )
    {
        mVelocity.fX *= mfVelocityDecay;
        mVelocity.fY *= mfVelocityDecay;
        
        if( fabs( mVelocity.fX ) < 0.001f )
        {
            mVelocity.fX = 0.0f;
        }
        
        if( fabs( mVelocity.fY ) < 0.001f )
        {
            mVelocity.fY = 0.0f;
        }
    }
    
    // don't move if at end
    if( miDisplayType == TYPE_VERTICAL )
    {
        if( !mbWrap )
        {
            if( mVelocity.fY > 0.0f )
            {
                if( miTop - 1 < 0 )
                {
                    mVelocity.fY = 0.0f;
                }
            }
            else if( mVelocity.fY < 0.0f )
            {
                if( miTop + miMaxEntries > miNumEntries )
                {
                    mVelocity.fY = 0.0f;
                }
            }
        }
    }
    
    mPositionOffset.fX += mVelocity.fX;
    mPositionOffset.fY += mVelocity.fY;
    
    // snap back to original place and update data when passed the child's dimension
    if( miDisplayType == TYPE_VERTICAL )
    {
        if( fabs( mPositionOffset.fY ) >= dimension.fY + LIST_SPACING )
        {
            // update the top
            if( mPositionOffset.fY < 0.0f )
            {
                updateChildren( 1 );
            }
            else if( mPositionOffset.fY > 0.0f )
            {
                updateChildren( -1 );
            }
            
            if( mpfnAtStart )
            {
                mpfnAtStart( miTop );
            }
            
            // back at start
            mPositionOffset.fY = 0.0f;
        }   // if offset passed dimension
    }   // if vertical
    else if( miDisplayType == TYPE_HORIZONTAL )
    {
        if( fabs( mPositionOffset.fX ) >= dimension.fX + LIST_SPACING )
        {
            // update the top
            if( mPositionOffset.fX < 0.0f )
            {
                --miTop;
                if( miTop < 0 )
                {
                    miTop = 0;
                }
                
                // disable appropriate controls
                for( int i = -1; i < miMaxEntries + 1; i++ )
                {
                    if( miTop + i < 0 && miTop + i >= miNumEntries )
                    {
                        mapChildren[i+1+miStartList]->setState( CControl::STATE_DISABLED );
                    }
                    else
                    {
                        mapChildren[i+1+miStartList]->setState( CControl::STATE_NORMAL );
                    }
                }
                
                // fill out the controls
                if( mpfnFillData )
                {
                    for( int i = -1; i < miMaxEntries + 1; i++ )
                    {
                        if( miTop + i >= 0 && miTop + i < miNumEntries )
                        {
                            mpfnFillData( mapChildren[miStartList+i+1], miTop + i );
                        }
                    }
                }
            }
            else if( mPositionOffset.fX > 0.0f )
            {
                ++miTop;
                
                // disable appropriate controls
                for( int i = -1; i < miMaxEntries + 1; i++ )
                {
                    if( miTop + i < 0 && miTop + i >= miNumEntries )
                    {
                        mapChildren[i+1+miStartList]->setState( CControl::STATE_DISABLED );
                    }
                    else
                    {
                        mapChildren[i+1+miStartList]->setState( CControl::STATE_NORMAL );
                    }
                }
                
                // fill out the controls
                if( mpfnFillData )
                {
                    for( int i = -1; i < miMaxEntries + 1; i++ )
                    {
                        if( miTop + i >= 0 && miTop + i < miNumEntries )
                        {
                            mpfnFillData( mapChildren[miStartList+i+1], miTop + i );
                        }
                    }
                }
            }
            
            // back at start
            mPositionOffset.fX = 0.0f;
        }   // if offset passed dimension
    }   // if horizontal
    
    // start position
    tVector2 pos = { 0.0f, -mDimension.fY * 0.5f };
    pos.fY += ( -( dimension.fY * 0.5f + LIST_SPACING ) + mPositionOffset.fY );
    
    if( miDisplayType == TYPE_HORIZONTAL )
    {
        pos.fX = -dimension.fX + mVelocity.fX;
        pos.fY = 0.0f;
    }
    
    // update position
    for( size_t i = miStartList; i < mapChildren.size(); i++ )
    {
        if( i == miStartList + 1 )
        {
            pos.fY += LIST_SPACING;
        }
        
        mapChildren[i]->setPosition( pos );
        
        // update position
        if( miDisplayType == TYPE_VERTICAL )
        {
            pos.fY += ( dimension.fY + LIST_SPACING );
        }
        else
        {
            pos.fX += ( dimension.fX + LIST_SPACING );
        }
    }   // for i = 0 to max entries

    // scroll back to original position
    if( mfStartScrollBackTime > 0.0f )
    {
        double fCurrTime = getCurrTime();
        double fTimeDiff = fCurrTime - mfStartScrollBackTime;
        float fPct = 1.0f - (float)( fTimeDiff / (double)SCROLL_BACK_TIME );
        if( fPct < 0.0f )
        {
            fPct = 0.0f;
            memcpy( &mPosition, &mOrigPosition, sizeof( mPosition ) );
            
            mfStartScrollBackTime = -1.0f;
        }
        else
        {
            mPosition.fX = mOrigPosition.fX + mScrollBackVelocity.fX * fPct;
            mPosition.fY = mOrigPosition.fY + mScrollBackVelocity.fY * fPct;
        }
    }
}

/*
**
*/
void CScrollList::startScrollBack( void )
{
    mfStartScrollBackTime = getCurrTime();
}

/*
**
*/
void CScrollList::setFillDataFunc( void (*pfnFillData)( CControl* pControl, int iIndex ), void* pUserData )
{
    mpfnFillData = pfnFillData;
    mpUserData = pUserData;
    int iStart = -1;
    int iEnd = miMaxEntries + 1;
    
    if( miNumEntries > 0 )
    {
        for( int i = iStart; i < iEnd; i++ )
        {
            if( miTop + i >= 0 && miTop + i < miNumEntries )
            {
                mapChildren[miStartList+i+1]->setState( CControl::STATE_NORMAL );
                mpfnFillData( mapChildren[miStartList+i+1], miTop + i );
            }
            else
            {
                mapChildren[miStartList+i+1]->setState( CControl::STATE_DISABLED );
            }
        }
    }
}

/*
**
*/
void CScrollList::setNumEntries( int iNumEntries )
{
    for( size_t i = miStartList; i < mapChildren.size(); i++ )
    {
        mapChildren[i]->setState( CControl::STATE_NORMAL );
    }
    
    miNumEntries = iNumEntries;
    
    if( iNumEntries < miMaxEntries )
    {
        for( size_t i = miStartList + iNumEntries + 1; i < mapChildren.size(); i++ )
        {
            mapChildren[i]->setState( CControl::STATE_DISABLED );
        }
    }
}

/*
**
*/
void CScrollList::refresh( void )
{
    if( mpfnFillData )
    {
        for( int i = -1; i < miMaxEntries + 1; i++ )
        {
            if( miTop + i >= 0 && miTop + i < miNumEntries )
            {
                mpfnFillData( mapChildren[miStartList+i+1], miTop + i );
            }
        }
    }
}

/*
**
*/
void CScrollList::updateChildren( int iOffset )
{
    if( mbWrap )
    {
        if( iOffset > 0 )
        {
            miTop = ( miTop + 1 ) % miNumEntries;
        }
        else
        {
            miTop = ( ( miNumEntries + miTop ) - 1 ) % miNumEntries;
        }
    }
    else
    {
        if( iOffset > 0 )
        {
            ++miTop;
        }
        else
        {
            --miTop;
            if( miTop < 0 )
            {
                miTop = 0;
            }
        
        }
    }
    
    // disable appropriate controls
    for( int i = -1; i < miMaxEntries + 1; i++ )
    {
        int iIndex = i + 1 + miStartList;
        
        if( mbWrap )
        {
            if( iIndex >= 0 && iIndex < miNumEntries )
            {
                mapChildren[iIndex]->setState( CControl::STATE_NORMAL );
            }
        }
        else
        {
            if( miTop + i >= 0 && miTop + i < miNumEntries )
            {
                mapChildren[iIndex]->setState( CControl::STATE_NORMAL );
            }
            else
            {
                mapChildren[iIndex]->setState( CControl::STATE_DISABLED );
            }
        }
    }
    
    // fill out the controls
    if( mpfnFillData )
    {
        for( int i = -1; i < miMaxEntries + 1; i++ )
        {
            int iIndex = miStartList + i + 1;
            
            int iSlotIndex = miTop + i;
            if( mbWrap )
            {
                iSlotIndex = ( miTop + i ) % miNumEntries;
            }
            
            if( miTop + i >= 0 && miTop + i < miNumEntries )
            {
                mpfnFillData( mapChildren[iIndex], iSlotIndex );
            }
        }
    }
}