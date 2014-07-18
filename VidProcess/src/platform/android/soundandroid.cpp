#include "soundandroid.h"
#include "filepathutil.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#define SMALLEST_DECIBEL 1500.0f

#define MAX_SOUND_PLAYERS 64

struct SoundPlayer
{
    SLObjectItf     mObject;
    SLPlayItf       mPlay;
    SLVolumeItf     mVolume;
    
    SLmillibel           mDecibel;
    SLmillisecond        mDuration;
};

typedef struct SoundPlayer tSoundPlayer;

static void playCallBack( SLPlayItf pObject, void* pContext, SLuint32 iState );

static SLObjectItf pEngineObject = NULL;
static SLEngineItf pEngine = NULL;
static SLObjectItf pOutputMixObject = NULL;
static SLEnvironmentalReverbItf pOutputMixEnvironmentReverb = NULL;
static SLSeekItf pSeekInterface = NULL;

static SLmillibel sCurrVol;

static AAssetManager* spAssetManager = NULL;

static const SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
static void createPlayer( tSoundPlayer* pPlayer,
                          AAssetManager* pAssetManager, 
                          const char* szFileName );

// ring buffer
static tSoundPlayer saPlayers[MAX_SOUND_PLAYERS];
static int          siLastPlayer;

// music player
static tSoundPlayer sMusicPlayer;

/*
**
*/
void androidSoundSetAssetManager( AAssetManager* pAssetManager )
{
    printf( "%s : %d SET ASSET MANAGER\n",
            __PRETTY_FUNCTION__,
           __LINE__ );
    spAssetManager = pAssetManager;
}


/*
**
*/
void initSoundPlatform( void )
{
    initOpenSL();
}

/*
**
*/
void releaseSoundPlatform( void )
{
    releaseOpenSL();
}

/*
**
*/
void initSoundFXPlatform( void )
{
    if( pEngine == NULL )
    {
    	initOpenSL();
    }
}

/*
**
*/
void initMusicPlatform( void )
{
	if( pEngine == NULL )
	{
		initOpenSL();
	}
}

/*
**
*/
void setMusicVolume( float fVolume )
{    
    SLmillibel maxVol;
    if( sMusicPlayer.mVolume == NULL )
    {
        return;
    }
    
    assert( sMusicPlayer.mVolume );
    SLresult result = (*(sMusicPlayer.mVolume))->GetMaxVolumeLevel( sMusicPlayer.mVolume, &maxVol );
    
    assert( result == SL_RESULT_SUCCESS );
    
    float fSmallestDecibel = SMALLEST_DECIBEL - (float)sMusicPlayer.mDecibel;
    SLmillibel newVol = (SLmillibel)( (float)maxVol - ( fSmallestDecibel * ( 1.0f - fVolume ) ) );
    
    /*printf( "!!!!!!!! %s : %d SET MUSIC VOLUME %d max volume %d curr volume %d !!!!!!\n",
           __PRETTY_FUNCTION__,
           __LINE__,
           newVol,
           maxVol,
           sCurrVol );
    */
    result = (*(sMusicPlayer.mVolume))->SetVolumeLevel( sMusicPlayer.mVolume, newVol );
    assert( result == SL_RESULT_SUCCESS );
}

/*
**
*/
void setSoundVolume( float fVolume )
{
    sCurrVol = 1.0f - fVolume;
}

/*
**
*/
void playSoundPlatform( const char* szFileName )
{
    printf( "%s : %d szFileName = %s\n",
            __PRETTY_FUNCTION__,
            __LINE__,
           szFileName );
    
    size_t iSize = strlen( szFileName );
    const char* szExtension = strstr( szFileName, "." );
    char szRename[256];
    memset( szRename, 0, sizeof( szRename ) );
    memcpy( szRename, szFileName, (unsigned int)szExtension - (unsigned int)szFileName );
    printf( "NO EXTENSION = %s\n", szRename );
    strncat( szRename, ".wav", sizeof( szRename ) );
    printf( "RENAME = %s\n", szRename );

    // look for a free player
    bool bFound = false;
    while( !bFound )
    {
        if( saPlayers[siLastPlayer].mObject == NULL )
        {
            bFound = true;
            break;
        }
        else
        {
            ++siLastPlayer;
        }
    }
    
    // create the player
    assert( saPlayers[siLastPlayer].mObject == NULL );
    printf( "%s : %d start create player spAssetManager = 0x%08X\n",
           __PRETTY_FUNCTION__,
           __LINE__,
           (unsigned int)spAssetManager );

    createPlayer( &saPlayers[siLastPlayer], spAssetManager, szRename );

    printf( "%s : %d end create player\n",
            __PRETTY_FUNCTION__,
           __LINE__ );

    siLastPlayer = ( siLastPlayer +  1 ) % MAX_SOUND_PLAYERS;
}

/*
**
*/
void stopSound( tSoundPlayer* pPlayer )
{    
    printf( "%s : %d stop player\n", 
            __PRETTY_FUNCTION__,
           __LINE__ );
    
    (*(pPlayer->mPlay))->SetPlayState( pPlayer->mPlay, SL_PLAYSTATE_STOPPED );
    (*(pPlayer->mObject))->Destroy( pPlayer->mObject );
    
    pPlayer->mPlay = NULL;
    pPlayer->mObject = NULL;
    pPlayer->mVolume = NULL;
}

/*
**
*/
void registerSound( const char* szSoundName )
{

}

/*
**
*/
float playMusicPlatform( const char* szFileName )
{
OUTPUT( "PLAY MUSIC %s\n", szFileName );
    
    stopMusic();
    
OUTPUT( "CREATE MUSIC PLAYER\n" );
	createPlayer( &sMusicPlayer, spAssetManager, szFileName );
    
    SLresult result = (*(sMusicPlayer.mVolume))->GetVolumeLevel( sMusicPlayer.mVolume, &sMusicPlayer.mDecibel );
    assert( result == SL_RESULT_SUCCESS );
    
    SLmillisecond duration;
    result = (*(sMusicPlayer.mPlay))->GetDuration( sMusicPlayer.mPlay, &duration );
    
    printf( "%s : %d duration = %d\n",
            __PRETTY_FUNCTION__,
            __LINE__,
           duration );
    
    return ( duration / 1000 );
}

/*
**
*/
void stopMusic( void )
{
OUTPUT( "%s : %d\n",
        __PRETTY_FUNCTION__,
       __LINE__ );

    if( sMusicPlayer.mObject )
    {
        (*(sMusicPlayer.mPlay))->SetPlayState( sMusicPlayer.mPlay, SL_PLAYSTATE_STOPPED );
        (*(sMusicPlayer.mObject))->Destroy( sMusicPlayer.mObject );
        
        sMusicPlayer.mPlay = NULL;
        sMusicPlayer.mObject = NULL;
        sMusicPlayer.mVolume = NULL;
    }
}

/*
**
*/
void updateSound( float fDT )
{
    for( int i = 0; i < MAX_SOUND_PLAYERS; i++ )
    {
        if( saPlayers[i].mObject )
        {
            SLPlayItf pPlayer = saPlayers[i].mPlay;
            
            SLuint32 iState;
            (*pPlayer)->GetPlayState( pPlayer, &iState );
            
            //printf( "%s : %d iState = %d\n",
            //        __PRETTY_FUNCTION__,
            //        __LINE__,
            //       iState );
            
            if( iState == SL_PLAYSTATE_PAUSED )
            {
                stopSound( &saPlayers[i] );
            }
        }
    }
}

/*
**
*/
void initOpenSL( void )
{
    SLresult result;

printf( "******* START INIT OPENSL ******\n" );

    // create engine
    result = slCreateEngine( &pEngineObject, 0, NULL, 0, NULL, NULL );
    assert( result == SL_RESULT_SUCCESS );

    result = (*pEngineObject)->Realize( pEngineObject, SL_BOOLEAN_FALSE );
    assert( result == SL_RESULT_SUCCESS );

    result = (*pEngineObject)->GetInterface( pEngineObject,
                                            SL_IID_ENGINE,
                                            &pEngine );
    assert( result == SL_RESULT_SUCCESS );


    const SLInterfaceID aIDs[1] = { SL_IID_ENVIRONMENTALREVERB };
    const SLboolean aReq[1] = { SL_BOOLEAN_FALSE };
    result = (*pEngine)->CreateOutputMix( pEngine,
                                         &pOutputMixObject,
                                         1,
                                         aIDs,
                                         aReq );
    assert( result == SL_RESULT_SUCCESS );

    (*pOutputMixObject)->Realize( pOutputMixObject, SL_BOOLEAN_FALSE );
    assert( result == SL_RESULT_SUCCESS );

    result = (*pOutputMixObject)->GetInterface( pOutputMixObject,
                                               SL_IID_ENVIRONMENTALREVERB,
                                               &pOutputMixEnvironmentReverb );
    if( result == SL_RESULT_SUCCESS )
    {
        result = (*pOutputMixEnvironmentReverb)->SetEnvironmentalReverbProperties( pOutputMixEnvironmentReverb,
                                                                            &reverbSettings );
    }

printf( "******* END INIT OPENSL ******\n" );
    
    memset( &sMusicPlayer, 0, sizeof( sMusicPlayer ) );
    memset( saPlayers, 0, sizeof( tSoundPlayer ) * MAX_SOUND_PLAYERS );
    siLastPlayer = 0;

}

/*
**
*/
void releaseOpenSL( void )
{
    // destroy file descriptor audio player object, and invalidate all associated interfaces
    for( int i = 0; i < MAX_SOUND_PLAYERS; i++ )
    {
        (*(saPlayers[i].mObject))->Destroy( saPlayers[i].mObject );
        saPlayers[i].mObject = NULL;
        saPlayers[i].mPlay = NULL;
        saPlayers[i].mVolume = NULL;
    }
    
    // destroy engine object, and invalidate all associated interfaces
    if( pEngineObject != NULL )
    {
        (*pEngineObject)->Destroy( pEngineObject );
        pEngineObject = NULL;
        pEngine = NULL;
    }
}

/*
**
*/
static void createPlayer( tSoundPlayer* pPlayer,
                          AAssetManager* pAssetManager, 
                          const char* szFileName )
{
    SLresult result;

    AAsset* pAsset = AAssetManager_open( pAssetManager, szFileName, AASSET_MODE_UNKNOWN );
OUTPUT( "%s : %d pAsset = 0x%8X file = %s\n",
       __PRETTY_FUNCTION__,
       __LINE__,
      (unsigned int)pAsset,
       szFileName );
    if( pAsset == NULL )
    {
        OUTPUT( "!!! CAN NOT FIND %s !!!\n", szFileName );
        return;
    }

    off_t start, length;
    int fd = AAsset_openFileDescriptor(pAsset, &start, &length);

OUTPUT( "%s : %d fd = %d\n",
        __PRETTY_FUNCTION__,
        __LINE__,
       fd );
    AAsset_close( pAsset );

    // configure audio source
    SLDataLocator_AndroidFD loc_fd = { SL_DATALOCATOR_ANDROIDFD, fd, start, length };
    SLDataFormat_MIME format_mime = { SL_DATAFORMAT_MIME, NULL, SL_CONTAINERTYPE_UNSPECIFIED };
    SLDataSource audioSrc = { &loc_fd, &format_mime };

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, pOutputMixObject };
    SLDataSink audioSnk = {&loc_outmix, NULL};

    // create audio player
    const SLInterfaceID ids[2] = {SL_IID_SEEK, SL_IID_VOLUME};
    const SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    result = (*pEngine)->CreateAudioPlayer( pEngine,
                                            &pPlayer->mObject,
                                            &audioSrc,
                                            &audioSnk,
                                            2,
                                            ids,
                                            req );

    OUTPUT( "%s : %d after create audio player\n",
    		__PRETTY_FUNCTION__,
    		__LINE__ );

    if( result != SL_RESULT_SUCCESS )
    {
        OUTPUT( "!!! DID NOT CREATE PLAYER FOR %s\n", szFileName );
        return;
    }
    

    SLObjectItf pPlayerObject = pPlayer->mObject;
    
    // realize the player
    result = (*(pPlayerObject))->Realize( pPlayerObject, SL_BOOLEAN_FALSE );
    WTFASSERT2( result == SL_RESULT_SUCCESS, "didn't realize player for : %s", szFileName );

    // get the play interface
    result = (*(pPlayerObject))->GetInterface( pPlayerObject, SL_IID_PLAY, &pPlayer->mPlay );
    WTFASSERT2( result == SL_RESULT_SUCCESS, "didn't get player interface for : %s", szFileName );

    // register player callback (doesn't even work now)
    result = (*(pPlayer->mPlay))->RegisterCallback( pPlayer->mPlay, playCallBack, NULL );
    WTFASSERT2( result == SL_RESULT_SUCCESS, "didn't create player callback for : %s", szFileName );
    
    // get the volume interface
    result = (*(pPlayerObject))->GetInterface( pPlayerObject, SL_IID_VOLUME, &pPlayer->mVolume );
    WTFASSERT2( result == SL_RESULT_SUCCESS, "didn't get volume interface for : %s", szFileName );
    
    pPlayer->mDecibel = 0;
    result = (*(pPlayer->mVolume))->SetVolumeLevel( pPlayer->mVolume, -0.1f/*pPlayer->mDecibel*/ );
    
    // start playing
    result = (*(pPlayer->mPlay))->SetPlayState( pPlayer->mPlay, SL_PLAYSTATE_PLAYING );
    WTFASSERT2( result == SL_RESULT_SUCCESS, "can't start playing for : %s", szFileName );
    
    result = (*(pPlayer->mPlay))->GetDuration( pPlayer->mPlay, &pPlayer->mDuration );
    WTFASSERT2( result == SL_RESULT_SUCCESS, "can't get play duration for : %s", szFileName );
    
    // seek interface
    result = (*pPlayerObject)->GetInterface( pPlayerObject,
                                             SL_IID_SEEK,
                                             &pSeekInterface );
    
    WTFASSERT2( result == SL_RESULT_SUCCESS, "can't create seek interface for: %s", szFileName );
    
    // loop
    result = (*pSeekInterface)->SetLoop( pSeekInterface, SL_BOOLEAN_TRUE, 1, pPlayer->mDuration );
    
    
    OUTPUT( "%s : %d duration = %d\n",
            __PRETTY_FUNCTION__,
            __LINE__,
           pPlayer->mDuration );
}

/*
**
*/
static void playCallBack( SLPlayItf pObject, void* pContext, SLuint32 iState )
{
    //if( iState == SL_PLAYEVENT_HEADATEND )
    {
        printf( "!!!!!!! %s : %d player end !!!!!!!!!\n",
               __PRETTY_FUNCTION__,
               __LINE__ );
    }
}

