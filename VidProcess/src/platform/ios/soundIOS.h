//
//  SoundIOS.h
//  CityLights
//
//  Created by Tony Peng on 10/20/11.
//  Copyright (c) 2011 Breaktime Studios. All rights reserved.
//

#ifndef __SOUNDIOS_H__
#define __SOUNDIOS_H__


void initMusicPlatform( void );
void initSoundFXPlatform( void );
void releaseSoundPlatform();
    
void playSoundPlatform( const char* szFileName );
float playMusicPlatform( const char* szFileName );

void setSoundVolume( float fVolume );
void setMusicVolume( float fVolume );
    
void updateSound( float fDT );

void registerSound( const char* szFileName );
    

#endif // __SOUNDIOS_H__
