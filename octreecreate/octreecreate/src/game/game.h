//
//  main.h
//  animtest
//
//  Created by Tony Peng on 6/24/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#ifndef __GAME_H__
#define __GAME_H__

enum
{
    TOUCHTYPE_BEGIN = 0,
    TOUCHTYPE_MOVE,
    TOUCHTYPE_END,
    
    NUM_TOUCHTYPES,
};

void setupTest( void );
void updateGame( float fTime );
void drawGame( void );
void gameInputUpdate( float fX, float fY, int iTouchType );

#endif // __GAME_H__
