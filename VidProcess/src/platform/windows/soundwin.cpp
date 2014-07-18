#if WIN32
#include "soundwin.h"

/*
**
*/
void initMusicPlatform( void )
{
}

/*
**
*/
void initSoundFXPlatform( void )
{
}

/*
**
*/
void releaseSoundPlatform( void )
{
}

/*
**
*/
void playSoundPlatform( const char* szFileName )
{
}

/*
**
*/
float playMusicPlatform( const char* szFileName )
{
	return 0.0f;
}

/*
**
*/
void setSoundVolume( float fVolume )
{
}

/*
**
*/
void setMusicVolume( float fVolume )
{
}

/*
**
*/
void updateSound( float fDT )
{
}

/*
**
*/
void registerSound( const char* szFileName )
{
}
#endif // WIN32
