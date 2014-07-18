#ifndef __SOUNDANDROID_H__
#define __SOUNDANDROID_H__

#include <android/asset_manager.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void initOpenSL( void );
void releaseOpenSL();

void initSoundPlatform( void );
void initSoundFXPlatform( void );
void releaseSoundPlatform( void );
void registerSound( const char* szSoundName );
    
void initMusicPlatform( void );
void setMusicVolume( float fVolume );
void playSoundPlatform( const char* szFileName ); 
float playMusicPlatform( const char* szFileName );

void setSoundVolume( float fVolume );
    
void stopSound( void );
void stopMusic( void );
void updateSound( float fDT );
    
void androidSoundSetAssetManager( AAssetManager* pAssetManager );
    
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __SOUNDANDROID_H__
