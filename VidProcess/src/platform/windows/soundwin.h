#ifndef __SOUNDWIN_H__
#define __SOUNDWIN_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void initMusicPlatform( void );
void initSoundFXPlatform( void );
void releaseSoundPlatform();
    
void playSoundPlatform( const char* szFileName );
float playMusicPlatform( const char* szFileName );

void setSoundVolume( float fVolume );
void setMusicVolume( float fVolume );
    
void updateSound( float fDT );

void registerSound( const char* szFileName );
    
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __SOUNDWIN_H__