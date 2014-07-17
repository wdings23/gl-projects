//
//  animplayer.cpp
//  CityBenchmark
//
//  Created by Dingwings on 7/31/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "animplayer.h"

/*
**
*/
CAnimPlayer::CAnimPlayer( void )
{
    memset( maFrames, 0, sizeof( maFrames ) );
    miNumFrames = 0;
}

/*
**
*/
CAnimPlayer::~CAnimPlayer( void )
{
    
}

/*
**
*/
bool CAnimPlayer::addFrame( const tAnimFrame* pFrame )
{
    bool bRet = false;
    assert( miNumFrames < sizeof( maFrames ) / sizeof( *maFrames ) );
    if( miNumFrames < sizeof( maFrames ) / sizeof( *maFrames ) )
    {
        memcpy( &maFrames[miNumFrames++], pFrame, sizeof( tAnimFrame ) );
        bRet = true;
    }
    
    return bRet;
}

/*
**
*/
bool CAnimPlayer::getFrame( tAnimFrame* pAnimFrame, double fTime )
{
    bool bRet = false;
    int iStartFrame = 0;
    for( iStartFrame = 0; iStartFrame < miNumFrames; iStartFrame++ )
    {
        if( maFrames[iStartFrame].mfTime > fTime )
        {
            if( iStartFrame - 1 >= 0 )
            {
                iStartFrame -= 1;
            }
            break;
        }
    }
    
    if( iStartFrame < miNumFrames )
    {
    
        int iNextFrame = iStartFrame + 1;
        if( iNextFrame < miNumFrames )
        {
            tAnimFrame* pStart = &maFrames[iStartFrame];
            tAnimFrame* pEnd = &maFrames[iNextFrame];
            float fDuration = (float)( pEnd->mfTime - pStart->mfTime );
            
            tVector4 diffPos = { 0.0f, 0.0f, 0.0f, 1.0f };
            Vector4Subtract( &diffPos, &pEnd->mPos, &pStart->mPos );
            
            tVector4 diffColor = { 0.0f, 0.0f, 0.0f, 0.0f };
            Vector4Subtract( &diffColor, &pEnd->mColor, &pStart->mColor );
            diffColor.fW = pEnd->mColor.fW - pStart->mColor.fW;
            
            tVector4 diffAngle = { 0.0f, 0.0f, 0.0f, 1.0f };
            Vector4Subtract( &diffAngle, &pEnd->mRotation, &pStart->mRotation );
            
            tVector4 diffScale = { 0.0f, 0.0f, 0.0f, 1.0f };
            Vector4Subtract( &diffScale, &pEnd->mScale, &pStart->mScale );
            
            float fTimeDiff = (float)( fTime - pStart->mfTime );
            assert( fTimeDiff >= 0.0 );
            assert( fDuration > 0.0f );
            float fPct = fTimeDiff / fDuration;
            
            if( fTimeDiff < fDuration )
            {
                pAnimFrame->mPos.fX = pStart->mPos.fX + diffPos.fX * fPct;
                pAnimFrame->mPos.fY = pStart->mPos.fY + diffPos.fY * fPct;
                pAnimFrame->mPos.fZ = pStart->mPos.fZ + diffPos.fZ * fPct;
                
                pAnimFrame->mColor.fX = pStart->mColor.fX + diffColor.fX * fPct;
                pAnimFrame->mColor.fY = pStart->mColor.fY + diffColor.fY * fPct;
                pAnimFrame->mColor.fZ = pStart->mColor.fZ + diffColor.fZ * fPct;
                pAnimFrame->mColor.fW = pStart->mColor.fW + diffColor.fW * fPct;
                
                pAnimFrame->mRotation.fX = pStart->mRotation.fX + diffAngle.fX * fPct;
                pAnimFrame->mRotation.fY = pStart->mRotation.fY + diffAngle.fY * fPct;
                pAnimFrame->mRotation.fZ = pStart->mRotation.fZ + diffAngle.fZ * fPct;
                
                pAnimFrame->mScale.fX = pStart->mScale.fX + diffScale.fX * fPct;
                pAnimFrame->mScale.fY = pStart->mScale.fY + diffScale.fY * fPct;
                pAnimFrame->mScale.fZ = pStart->mScale.fZ + diffScale.fZ * fPct;
                
            }
            else
            {
                memcpy( pAnimFrame, pEnd, sizeof( tAnimFrame ) );
            }
        }
        else
        {
            memcpy( pAnimFrame, &maFrames[iStartFrame], sizeof( tAnimFrame ) );
        }
        
        bRet = true;
    }
    
    return bRet;
}