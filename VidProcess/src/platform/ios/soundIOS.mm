//
//  SoundIOS.c
//  CityLights
//
//  Created by Tony Peng on 10/20/11.
//  Copyright (c) 2011 Breaktime Studios. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "hashutil.h"
#include "FilePathUtil.h"
#include "SoundIOS.h"
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <AVFoundation/AVFoundation.h>
#include <AudioToolbox/AudioToolbox.h>

#define MAX_AUDIOFILE_ID    100

struct SoundInfo
{
#if defined( DEBUG )
    char            mszName[64];
#endif // DEBUG
    
    unsigned long        miHash;
    ALuint          miSource;
    ALuint          miBuffer;
};

typedef struct SoundInfo tSoundInfo;

#if !TARGET_IPHONE_SIMULATOR
static ALCcontext* spContext;
static ALCdevice* spDevice;
#endif // TARGET_IPHONE_SIMULATOR

static AudioFileID saAudioFileIDs[MAX_AUDIOFILE_ID];

tSoundInfo saSoundInfo[MAX_AUDIOFILE_ID];
static int siCurrAudio; 
static float sfFXVolume;

AVAudioPlayer* spAudioPlayer = NULL;

static void getSound( const char* szFileName, ALuint* piSound, ALuint* piBuffer );
static void interruptionListenerCallback( void* pUserData, UInt32 interruptionState );

/*
**
*/
void* CDloadWaveAudioData(CFURLRef inFileURL, ALsizei *outDataSize, ALenum *outDataFormat, ALsizei*     outSampleRate)
{
    OSStatus                                                err = noErr;    
    UInt64                                                  fileDataSize = 0;
    AudioStreamBasicDescription             theFileFormat;
    UInt32                                                  thePropertySize = sizeof(theFileFormat);
    AudioFileID                                             afid = 0;
    void*                                                   theData = NULL;
    
    // Open a file with ExtAudioFileOpen()
    err = AudioFileOpenURL(inFileURL, kAudioFileReadPermission, 0, &afid);
    if(err)
    {
        // Dispose the ExtAudioFileRef, it is no longer needed
        if (afid) AudioFileClose(afid);
            return theData;
    }
    
    // Get the audio data format
    err = AudioFileGetProperty(afid, kAudioFilePropertyDataFormat, &thePropertySize, &theFileFormat);
    if(err)
    {
        // Dispose the ExtAudioFileRef, it is no longer needed
        if (afid) AudioFileClose(afid);
            return theData;
    }
    
    if (theFileFormat.mChannelsPerFrame > 2)
    {
        
        // Dispose the ExtAudioFileRef, it is no longer needed
        if (afid) AudioFileClose(afid);
            return theData;
    }
    
    if ((theFileFormat.mFormatID != kAudioFormatLinearPCM) || (!TestAudioFormatNativeEndian(theFileFormat)))
    {
        
        // Dispose the ExtAudioFileRef, it is no longer needed
        if (afid) AudioFileClose(afid);
            return theData;
    }
    
    if ((theFileFormat.mBitsPerChannel != 8) && (theFileFormat.mBitsPerChannel != 16))
    {
        
        // Dispose the ExtAudioFileRef, it is no longer needed
        if (afid) AudioFileClose(afid);
            return theData;
    }
    
    thePropertySize = sizeof(fileDataSize);
    err = AudioFileGetProperty(afid, kAudioFilePropertyAudioDataByteCount, &thePropertySize, &fileDataSize);
    if(err)
    {
        // Dispose the ExtAudioFileRef, it is no longer needed
        if (afid) AudioFileClose(afid);
            return theData;
    }
    
    // Read all the data into memory
    UInt32          dataSize = (UInt32)fileDataSize;
    theData = malloc(dataSize);
    if (theData)
    {
            AudioFileReadBytes(afid, false, 0, &dataSize, theData);
            if(err == noErr)
            {
                // success
                *outDataSize = (ALsizei)dataSize;
                //This fix was added by me, however, 8 bit sounds have a clipping sound at the end so aren't really usable (SO)
                if (theFileFormat.mBitsPerChannel == 16) { 
                    *outDataFormat = (theFileFormat.mChannelsPerFrame > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
                } else {
                    *outDataFormat = (theFileFormat.mChannelsPerFrame > 1) ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;   
                }       
                *outSampleRate = (ALsizei)theFileFormat.mSampleRate;
            }
            else 
            { 
                // failure
                free (theData);
                theData = NULL; // make sure to return NULL
                
                // Dispose the ExtAudioFileRef, it is no longer needed
                if (afid) AudioFileClose(afid);
                    return theData;
            }       
        }
    
    return theData;
}

/*
**
*/
void* CDloadCafAudioData(CFURLRef inFileURL, ALsizei *outDataSize, ALenum *outDataFormat, ALsizei* outSampleRate)
{
    OSStatus                                                status = noErr;
    BOOL                                                    abort = NO;
    SInt64                                                  theFileLengthInFrames = 0;
    AudioStreamBasicDescription             theFileFormat;
    UInt32                                                  thePropertySize = sizeof(theFileFormat);
    ExtAudioFileRef                                 extRef = NULL;
    void*                                                   theData = NULL;
    AudioStreamBasicDescription             theOutputFormat;
    UInt32                                                  dataSize = 0;
    
    // Open a file with ExtAudioFileOpen()
    status = ExtAudioFileOpenURL(inFileURL, &extRef);
    if (status != noErr)
    {
        abort = YES;
    }
    
    if (abort)
        goto Exit;
 
    // Get the audio data format
    status = ExtAudioFileGetProperty(extRef, kExtAudioFileProperty_FileDataFormat, &thePropertySize, &theFileFormat);
    if (status != noErr)
    {
            
        abort = YES;
    }
    
    if (abort)
        goto Exit;
    
    if (theFileFormat.mChannelsPerFrame > 2)
    {
        abort = YES;
    }
    
    if (abort)
        goto Exit;
    
    // Set the client format to 16 bit signed integer (native-endian) data
    // Maintain the channel count and sample rate of the original source format
    theOutputFormat.mSampleRate = theFileFormat.mSampleRate;
    theOutputFormat.mChannelsPerFrame = theFileFormat.mChannelsPerFrame;
    
    theOutputFormat.mFormatID = kAudioFormatLinearPCM;
    theOutputFormat.mBytesPerPacket = 2 * theOutputFormat.mChannelsPerFrame;
    theOutputFormat.mFramesPerPacket = 1;
    theOutputFormat.mBytesPerFrame = 2 * theOutputFormat.mChannelsPerFrame;
    theOutputFormat.mBitsPerChannel = 16;
    theOutputFormat.mFormatFlags = kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked | kAudioFormatFlagIsSignedInteger;
    
    // Set the desired client (output) data format
    status = ExtAudioFileSetProperty(extRef, kExtAudioFileProperty_ClientDataFormat, sizeof(theOutputFormat), &theOutputFormat);
    if (status != noErr)
    {
        abort = YES;
    }
    
    if (abort)
        goto Exit;
    
    // Get the total frame count
    thePropertySize = sizeof(theFileLengthInFrames);
    status = ExtAudioFileGetProperty(extRef, kExtAudioFileProperty_FileLengthFrames, &thePropertySize, &theFileLengthInFrames);
    if (status != noErr)
    {
        
        abort = YES;
    }
    
    if (abort)
        goto Exit;
    
    // Read all the data into memory
    dataSize = (UInt32) theFileLengthInFrames * theOutputFormat.mBytesPerFrame;
    theData = malloc(dataSize);
    if (theData)
    {
        AudioBufferList         theDataBuffer;
        theDataBuffer.mNumberBuffers = 1;
        theDataBuffer.mBuffers[0].mDataByteSize = dataSize;
        theDataBuffer.mBuffers[0].mNumberChannels = theOutputFormat.mChannelsPerFrame;
        theDataBuffer.mBuffers[0].mData = theData;
        
        // Read the data into an AudioBufferList
        status = ExtAudioFileRead(extRef, (UInt32*)&theFileLengthInFrames, &theDataBuffer);
        if(status == noErr)
        {
            // success
            *outDataSize = (ALsizei)dataSize;
            *outDataFormat = (theOutputFormat.mChannelsPerFrame > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
            *outSampleRate = (ALsizei)theOutputFormat.mSampleRate;
        }
        else
        {
                // failure
                free (theData);
                theData = NULL; // make sure to return NULL
                abort = YES;
        }
    }
    
    if (abort)
        goto Exit;
    
    Exit:
    // Dispose the ExtAudioFileRef, it is no longer needed
    if (extRef) ExtAudioFileDispose(extRef);
        return theData;
}

/*
**
*/
void* GetOpenALAudioData(CFURLRef inFileURL, ALsizei *outDataSize, ALenum *outDataFormat, ALsizei*	outSampleRate) {
	
	CFStringRef extension = CFURLCopyPathExtension(inFileURL);
	CFComparisonResult isWavFile = (CFComparisonResult)0;
	if (extension != NULL) {
		isWavFile = CFStringCompare (extension,(CFStringRef)@"wav", kCFCompareCaseInsensitive);
		CFRelease(extension);
	}	
	
	if (isWavFile == kCFCompareEqualTo) {
		return CDloadWaveAudioData(inFileURL, outDataSize, outDataFormat, outSampleRate);	
	} else {
		return CDloadCafAudioData(inFileURL, outDataSize, outDataFormat, outSampleRate);		
	}
}

void playSoundPlatform( const char* szFileName )
{
    // check if other audio is playing
    bool bMuteSelf = false;
    //UInt32 otherAudioIsPlaying;
    //UInt32 propertySize = sizeof( otherAudioIsPlaying );
    //AudioSessionGetProperty( kAudioSessionProperty_OtherAudioIsPlaying, &propertySize, &otherAudioIsPlaying );
    
    /*if( otherAudioIsPlaying )
    {
        bMuteSelf = true;
    }*/
    
    if( !bMuteSelf )
    {
        char szFullPath[512];
        getFullPath( szFullPath, szFileName );
        
        // find sound id
        bool bFound = false;
        unsigned long iHash = hash( szFileName );
        for( int i = 0; i < siCurrAudio; i++ )
        {
            if( saSoundInfo[i].miHash == iHash )
            {
                bFound = true;
                break;
            }
        }
        
        // register sound
        if( !bFound )
        {
            registerSound( szFileName );
        }
        
        ALuint iBuffer;
        ALuint iSource; 
        getSound( szFileName, &iSource, &iBuffer );
        
        alSourcei( iSource, AL_BUFFER, iBuffer );
        alSourcef( iSource, AL_PITCH, 1.0f );
        alSourcef( iSource, AL_GAIN, sfFXVolume );
        
        alSourcePlay( iSource );
    }
}

float playMusicPlatform( const char* szFileName )
{
#if !TARGET_IPHONE_SIMULATOR
    
    char szFullPath[512];
    getFullPath( szFullPath, szFileName );
    
    NSString* filename = [[NSString alloc] initWithCString:szFullPath encoding:NSUTF8StringEncoding];
    NSURL* pURL = [NSURL fileURLWithPath:filename];
  
    // check if other audio is playing
    bool bMuteSelf = false;
    UInt32 otherAudioIsPlaying;
    UInt32 propertySize = sizeof( otherAudioIsPlaying );
    AudioSessionGetProperty( kAudioSessionProperty_OtherAudioIsPlaying, &propertySize, &otherAudioIsPlaying );
    if( otherAudioIsPlaying )
    {
        bMuteSelf = true;
    }
    
    // allow mute button to work
    if( !bMuteSelf )
    {
        NSError* error;
        spAudioPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:pURL error:&error];
        spAudioPlayer.numberOfLoops = -1;
    
        SInt32 ambient = kAudioSessionCategory_AmbientSound;
        AudioSessionSetProperty( kAudioSessionProperty_AudioCategory, sizeof( ambient ), &ambient );
        
        [spAudioPlayer play];
    }
#endif // !TARGET_IPHONE_SIMULATOR
    
    float fDuration = spAudioPlayer.duration;  
    return fDuration;
}

void setSoundVolume( float fVolume )
{
    sfFXVolume = fVolume;
}

void setMusicVolume( float fVolume )
{
    spAudioPlayer.volume = fVolume;
}

void updateSound( float fDT )
{
    
}

/*
**
*/
void initMusicPlatform( void )
{
#if !TARGET_IPHONE_SIMULATOR
    AudioSessionInitialize( NULL, NULL, interruptionListenerCallback, NULL );
    
    // allow other music to play
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryAmbient error:nil];
#endif // !TARGET_IPHONE_SIMULATOR
}

/*
**
*/
void initSoundFXPlatform( void )
{
#if !TARGET_IPHONE_SIMULATOR
    spDevice = alcOpenDevice( NULL );
    if( spDevice )
    {
        spContext = alcCreateContext( spDevice, NULL );
        alcMakeContextCurrent( spContext );
    }
#endif // !TARGET_IPHONE_SIMULATOR
    
    memset( saAudioFileIDs, 0, sizeof( saAudioFileIDs ) );
    siCurrAudio = 0;
    
    sfFXVolume = 1.0f;
}

static AudioFileID openSoundFile( const char* szFileName )
{
    NSString* filename = [[NSString alloc] initWithCString:szFileName encoding:NSUTF8StringEncoding];
    NSURL* pURL = [NSURL fileURLWithPath:filename];
    
    AudioFileID audioID;
    CFURLRef cfPtr = (__bridge CFURLRef)pURL;
    OSStatus result = AudioFileOpenURL( cfPtr,
                                        kAudioFileReadPermission, 
                                        0, 
                                        &audioID );
    
    if( result != 0 )
    {
        WTFASSERT2( result == 0, "can't find sound %s", szFileName );
    }
    
    return audioID;
}

void releaseSoundPlatform()
{
    for( int i = 0; i < MAX_AUDIOFILE_ID; i++ )
    {
        if( saSoundInfo[i].miHash != 0 )
        {
            alDeleteSources( 1, &(saSoundInfo[i].miSource) );
            alDeleteBuffers( 1, &(saSoundInfo[i].miBuffer) );
        }
    }
}

static UInt32 getSoundSize( AudioFileID* pID )
{
    UInt64 iOutSize = 0;
    UInt32 iPropSize = sizeof( iOutSize );
    
    OSStatus result = AudioFileGetProperty( *pID, 
                                            kAudioFilePropertyAudioDataByteCount, 
                                            &iPropSize, 
                                            &iOutSize );
    
    if( result != 0 )
    {
        assert( result == 0 );
    }
    
    return (UInt32)iOutSize;
}

void registerSound( const char* szFileName )
{
    // see if it's already registered
    bool bFound = false;
    int iHash = hash( szFileName );
    for( int i = 0; i < siCurrAudio; i++ )
    {
        if( saSoundInfo[i].miHash == iHash )
        {
            bFound = true;
            break;
        }
    }
    
    if( bFound )
    {
        return;
    }
    
    assert( siCurrAudio < MAX_AUDIOFILE_ID );
    
    char szFullPath[512];
    getFullPath( szFullPath, szFileName );
    
    AudioFileID audioID = openSoundFile( szFullPath );
    UInt32 iSoundSize = getSoundSize( &audioID );
    unsigned char* pOutData = (unsigned char *)malloc( iSoundSize );
    
    OSStatus result = AudioFileReadBytes( audioID, false, 0, &iSoundSize, pOutData );
    
    if( result != 0 )
    {
        assert( result == 0 );
    }
    
    // close
    AudioFileClose( audioID );
    
    // save the info
    tSoundInfo* pInfo = &saSoundInfo[siCurrAudio];
    pInfo->miHash = hash( szFileName );
    
#if defined( DEBUG )
    strncpy( pInfo->mszName, szFileName, sizeof( pInfo->mszName ) );
#endif // DEBUG
    
    // still has sound resource, delete it
    if( pInfo->miSource != 0 )
    {
        alDeleteSources( 1, &pInfo->miSource );
        alDeleteBuffers( 1, &pInfo->miBuffer );
    }
    
    // buffer for the data
    alGenBuffers( 1, &pInfo->miBuffer );
    
    NSString* filename = [[NSString alloc] initWithCString:szFullPath encoding:NSUTF8StringEncoding];
    NSURL* pURL = [NSURL fileURLWithPath:filename];
    
    ALsizei iDataSize;
    ALenum format;
    ALsizei iSampleRate;
    
    CFURLRef cfPtr = (__bridge CFURLRef)pURL;
    
    void* pALAudioData = GetOpenALAudioData( cfPtr, &iDataSize, &format, &iSampleRate );
    WTFASSERT2( pALAudioData, "can't load sound file: %s", szFileName );
    
#if !defined( FINAL )
    const char* szConvFileName = [filename UTF8String];
    WTFASSERT2( pALAudioData != NULL, "%s is not valid", szConvFileName );
#endif // FINAL
    
    alBufferData( pInfo->miBuffer, format, pALAudioData, iSoundSize, iSampleRate );
    
    // generate sources
    alGenSources( 1, &pInfo->miSource );
    alSourcei( pInfo->miBuffer, AL_BUFFER, pInfo->miSource );
    
    // property
    alSourcef( pInfo->miSource, AL_PITCH, 1.0f );
    alSourcef( pInfo->miSource, AL_GAIN, 1.0f );
    
    free( pALAudioData );
    free( pOutData );
    
    siCurrAudio = ( siCurrAudio + 1 ) % MAX_AUDIOFILE_ID;
}

static void getSound( const char* szFileName, ALuint* piSound, ALuint* piBuffer )
{
    unsigned long iHash = hash( szFileName );
    for( int i = 0; i < MAX_AUDIOFILE_ID; i++ )
    {
        if( iHash == saSoundInfo[i].miHash )
        {
            *piSound = saSoundInfo[i].miSource;
            *piBuffer = saSoundInfo[i].miBuffer;
            break;
        }
    }
}

static void interruptionListenerCallback( void* pUserData, UInt32 interruptionState )
{
#if !TARGET_IPHONE_SIMULATOR
    if( interruptionState == kAudioSessionBeginInterruption )
    {
        alcMakeContextCurrent( NULL );
        alcSuspendContext( spContext );
        
        AudioSessionSetActive( NO );
    }
    else if( interruptionState == kAudioSessionEndInterruption )
    {
        AudioSessionSetActive( YES );
        
        alcMakeContextCurrent( spContext );
        alcProcessContext( spContext );
    }
#endif // TARGET_IPHONE_SIMULATOR
}
