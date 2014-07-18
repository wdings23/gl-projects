//
//  ViewController.m
//  VidProcess
//
//  Created by Dingwings on 6/7/14.
//  Copyright (c) 2014 Dingwings. All rights reserved.
//

#import "ViewController.h"
#import <MobileCoreServices/MobileCoreServices.h>
#import <AssetsLibrary/ALAssetsLibrary.h>
#import <UIKit/UIKit.h>
#import <AVFoundation/AVPlayerItemOutput.h>

#include "app.h"
#include "timeutil.h"
#include "tga.h"
#include "shadermanager.h"

#include "lodepng.h"
#include "filepathutil.h"

#include "tinyxml.h"

//#define EXPORT_WITH_SHADER
#define ONE_FRAME_DURATION 0.03
#define MOVIE_FRAME_PER_SECONDS 20.0f

enum
{
    TAB_ITEM_FILTERS = 0,
    TAB_ITEM_EXPORT,
    
    NUM_TAB_ITEMS,
};

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
    
    mfWidth = [UIScreen mainScreen].bounds.size.width;
    mfHeight = [UIScreen mainScreen].bounds.size.height;
    
    miVideoWidth = 640;
    miVideoHeight = 480;
    
    float fMovieRatio = (float)miVideoHeight / (float)miVideoWidth;

    mfViewScale = 1.0f;
    
    // x coordinate
    float fPaddingWidth = mfWidth * 0.1f;
    float fViewWidth = mfWidth - fPaddingWidth;
    float fX = fPaddingWidth * 0.5f;
    
    // y coordinate
    const float fPaddingHeight = 10.0f;
    float fY = _navigationBar.frame.size.height + fPaddingHeight * 0.5f;
    float fViewHeight = fViewWidth * fMovieRatio;
    
    // glk view
    mpContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    mpGLKView = [[GLKView alloc] initWithFrame:CGRectMake( fX, fY, fViewWidth, fViewHeight ) ];
    
    mpGLKView.context = mpContext;
    mpGLKView.delegate = self;
    
    [EAGLContext setCurrentContext:mpContext];
    
    // add as sub view
    [self.view addSubview:mpGLKView];
    
    [self setupGL];
    [self setupCaptureSession];
    
    // display link to refresh screen
    CADisplayLink* pDisplayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector( drawFrame: )];
    pDisplayLink.frameInterval = 0;
    [pDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    
    // video output for playback
    NSDictionary* pixBufferAttrib = @{(id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA)};
    mpVideoOutput = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:pixBufferAttrib];
    mVidOutQueue = dispatch_queue_create( "videoout", NULL );
    [mpVideoOutput setDelegate:self queue:mVidOutQueue];
    
    // movie player
    mpVideoPlayer = [[AVPlayer alloc] init];
    
    CApp::instance()->init();
    mfLastTime = 0.0;
    
    _itemScrollView.showsHorizontalScrollIndicator = YES;
    _itemScrollView.scrollEnabled = YES;
    _itemScrollView.userInteractionEnabled = YES;
    _itemScrollView.directionalLockEnabled = YES;
    _itemScrollView.delegate = self;
    
    miScrollItemType = -1;
    
    if( macFrameBuffer == NULL )
    {
        macFrameBuffer = (unsigned char *)malloc( 1024 * 1024 * 4 * sizeof( char ) );
    }
    
    if( macPixelBufferCopy == NULL )
    {
        macPixelBufferCopy = (unsigned char *)malloc( 1024 * 1024 * 4 * sizeof( char ) );
    }
    
    pthread_mutex_init( &mMutex, NULL );
    
    mbRefreshTexture = false;
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(playMovie:) name:@"StartMovie" object:nil];
    
    mbSaveToAlbum = false;
    mbFilterShowing = false;
    
    miMovieIndex = -1;
    miState = STATE_DEFAULT;
    
    [_playerSlider setMinimumValue:0.0f];
    [_playerSlider setMaximumValue:1.0f];
    [_playerSlider setValue:0.0f animated:YES];
    
    mfMovieSeconds = 0.0f;
}


- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    glClearColor( 1.0f, 0.0f, 0.0f, 1.0f );
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
    
    double fCurrTime = getTime();
    if( mfLastTime == 0.0 )
    {
        mfLastTime = fCurrTime;
    }
    
    double fDT = fCurrTime - mfLastTime;
    mfLastTime = fCurrTime;
    
    if( mbRefreshTexture )
    {
        // set new texture
        CApp::instance()->setTextureData( macPixelBufferCopy, miVideoWidth, miVideoHeight );
        mbRefreshTexture = false;
    }
    
    CApp::instance()->update( fDT );
    CApp::instance()->draw();
    
    // recording
    if( miState == STATE_RECORDING &&
        miPrevState == STATE_RECORDING )
    {
        // append the current frame
        [self exportVideoFrame:macPixelBufferCopy];
        
    }
    
    if( miState == STATE_EXPORTING && miPrevState == STATE_EXPORTING )
    {
        // save to album
        glFlush();
        CApp::instance()->getFrameBuffer( macFrameBuffer, miVideoWidth, miVideoHeight );
        [self exportVideoFrame:macFrameBuffer];
    }

    // stop exporting frame
    if( ( miState != STATE_RECORDING && miPrevState == STATE_RECORDING ) ||
        ( miState != STATE_EXPORTING && miPrevState == STATE_EXPORTING ) )
    {
        [self endExportVideo];
        [self setState:STATE_DEFAULT];
        
        if( mbSaveToAlbum )
        {
            NSArray* pPaths = NSSearchPathForDirectoriesInDomains( NSDocumentDirectory, NSUserDomainMask, YES );
            NSString* pDocDir = [pPaths objectAtIndex:0];
            NSString* pFileName = [pDocDir stringByAppendingPathComponent:[NSString stringWithFormat:@"temp.mov" ] ];
            
            //UISaveVideoAtPathToSavedPhotosAlbum( pFileName, nil, nil, nil );
            mbSaveToAlbum = false;
        }
    }
}

- (void)drawFrame:(CADisplayLink *)sender
{
    [mpGLKView display];
    
    CMTime outputItemTime = [mpPlayerItem currentTime];
    CMTime duration = [mpPlayerItem duration];
    
    // capture the pixel buffer from video outpu
    if( [mpVideoOutput hasNewPixelBufferForItemTime:outputItemTime] )
    {
        // set slider's position
        Float64 currTime = CMTimeGetSeconds( outputItemTime );
        Float64 fDuration = CMTimeGetSeconds( duration );
        
        float fPct = currTime / fDuration;
        _playerSlider.value = fPct;
        
        [_playerSlider setValue:fPct animated:YES];
        
OUTPUT( "currTime %f of %f \n", (float)currTime, fDuration );
        
        // get shader at current time
        bool bFoundShader = false;
        std::vector<tKeyFrame>::iterator iter = maKeyFrames.begin();
        for( ; iter != maKeyFrames.end(); ++iter )
        {
            tKeyFrame keyFrame = *iter;
            if( keyFrame.mfTime >= (double)currTime * 1000.0 )
            {
                tShaderProgram const* pShader = NULL;
                if( iter != maKeyFrames.begin() )
                {
                    tKeyFrame prevKey = *( iter - 1 );
                    pShader = CShaderManager::instance()->getShaderProgram( prevKey.mszShaderName );
                    bFoundShader = true;
                }
                else
                {
                    pShader = CShaderManager::instance()->getShaderProgram( keyFrame.mszShaderName );
                    bFoundShader = true;
                }
                
OUTPUT( "SHADER: %s\n", pShader->mszName );
                
                CApp::instance()->setShader( pShader );
                break;
            }
            
        }   // for iter = key frames begin() to end()
        
        // last key frame shader
        if( !bFoundShader )
        {
            tKeyFrame lastKeyFrame = *( maKeyFrames.end() - 1 );
            tShaderProgram const* pShader = CShaderManager::instance()->getShaderProgram( lastKeyFrame.mszShaderName );
            CApp::instance()->setShader( pShader );
        }
        
        CVPixelBufferRef pixelBuffer = [mpVideoOutput copyPixelBufferForItemTime:outputItemTime itemTimeForDisplay:NULL];
    
        CVPixelBufferLockBaseAddress( pixelBuffer, 0 );
        
        // Get information of the image
        uint8_t* pAddress = (uint8_t *)CVPixelBufferGetBaseAddress( pixelBuffer );
        size_t iWidth = CVPixelBufferGetWidth( pixelBuffer );
        size_t iHeight = CVPixelBufferGetHeight( pixelBuffer );
        
        // copy the pixel buffer
        memcpy( macPixelBufferCopy, pAddress, iWidth * iHeight * 4 );
        
        // Unlock the image buffer
        CVPixelBufferUnlockBaseAddress( pixelBuffer, 0 );
        CVBufferRelease( pixelBuffer );
    }
}

- (void)setupGL
{
    glDisable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
}

- (void)setupCaptureSession
{
    // capture device
    AVCaptureDevice* pDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    NSArray* devices = [AVCaptureDevice devices];
    for( AVCaptureDevice* device in devices )
    {
        if( [device hasMediaType:AVMediaTypeVideo] )
        {
            if( [device position] == AVCaptureDevicePositionBack )
            {
                pDevice = device;
                break;
            }
        }
    }
    
    // device input
    AVCaptureDeviceInput* pDeviceInput = [AVCaptureDeviceInput deviceInputWithDevice:pDevice error:nil];
    
    // data output
    AVCaptureVideoDataOutput* pDataOutput = [[AVCaptureVideoDataOutput alloc] init];
    [pDataOutput setAlwaysDiscardsLateVideoFrames:YES];
    
    // set capture delegate in thread
    dispatch_queue_t queue = dispatch_queue_create( "vidprocess", NULL );
    [pDataOutput setSampleBufferDelegate:self queue:queue];
    
    [pDataOutput setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:kCVPixelFormatType_32BGRA]
                                                            forKey:(id)kCVPixelBufferPixelFormatTypeKey]];
    
    // capture session
    mpCaptureSession = [[AVCaptureSession alloc] init];
    [mpCaptureSession beginConfiguration];
    
    if( [mpCaptureSession canSetSessionPreset:AVCaptureSessionPreset640x480] )
    {
        [mpCaptureSession setSessionPreset:AVCaptureSessionPreset640x480];
    }
    
    // add input
    if( [mpCaptureSession canAddInput:pDeviceInput] )
    {
        [mpCaptureSession addInput:pDeviceInput];
        printf( "ADDED INPUT\n" );
    }
    
    // set capture session's output to rgba
    if( [mpCaptureSession canAddOutput:pDataOutput] )
    {
        [mpCaptureSession addOutput:pDataOutput];
        printf( "ADDED OUTPUT\n" );
    }
    
    // commit and start
    [mpCaptureSession commitConfiguration];
    [mpCaptureSession startRunning];

}

// for sample buffer
// this gets called after successful frame capture with valid data buffer
- (void)captureOutput:(AVCaptureOutput *)captureOutput
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection
{
    // just started recording
    if( ( miState == STATE_RECORDING && miPrevState != STATE_RECORDING ) ||
        ( miState == STATE_EXPORTING && miPrevState != STATE_EXPORTING ) )
    {
        char szMovieName[64];
        if( mbSaveToAlbum )
        {
            strncpy( szMovieName, "temp.mov", sizeof( szMovieName ) );
        }
        else
        {
            CApp::getValidFileName( szMovieName, "mov" );
        }
        
OUTPUT( "SAVE MOVIE: %s\n", szMovieName );
        
        NSString* movieName = [NSString stringWithUTF8String:szMovieName];
        
        NSArray* pPaths = NSSearchPathForDirectoriesInDomains( NSDocumentDirectory, NSUserDomainMask, YES );
        NSString* pDocDir = [pPaths objectAtIndex:0];
        NSString* pFileName = [pDocDir stringByAppendingPathComponent:movieName];
        
        CApp::instance()->setMovieName( szMovieName );
        
        // remove the previous movie
        NSError* error;
        [[NSFileManager defaultManager] removeItemAtPath:pFileName error:&error];
        
        [self setupAssetWriter:[NSURL fileURLWithPath:pFileName]];
        [self startExportVideo];
    }
    
    size_t iWidth = 0;
    size_t iHeight = 0;
    
    // copy current frame's image
    if( miState != STATE_PLAYING &&
        miState != STATE_EXPORTING )
    {        
        // image buffer ref
        CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer( sampleBuffer );
        
        // begin data process
        CVPixelBufferLockBaseAddress( imageBuffer, 0 );
        
        uint8_t* pAddress = (uint8_t *)CVPixelBufferGetBaseAddress( imageBuffer );
        iWidth = CVPixelBufferGetWidth( imageBuffer );
        iHeight = CVPixelBufferGetHeight( imageBuffer );
        size_t iDataSize = CVPixelBufferGetDataSize( imageBuffer );
        
        if( macPixelBufferCopy == NULL )
        {
            macPixelBufferCopy = (unsigned char *)malloc( 1024 * 1024 * 4 * sizeof( char ) );
        }
        
        memcpy( macPixelBufferCopy, pAddress, sizeof( char ) * iDataSize );
        
        // end data process
        CVPixelBufferUnlockBaseAddress( imageBuffer, 0 );
    
        miVideoWidth = (int)iWidth;
        miVideoHeight = (int)iHeight;
    }
    
    // just started recording
    if( ( miState == STATE_RECORDING && miPrevState != STATE_RECORDING ) ||
        ( miState == STATE_EXPORTING && miPrevState != STATE_EXPORTING ) )
    {
        // save first frame as png
        if( !mbSaveToAlbum )
        {
            // save in concurrent thread
            dispatch_async( dispatch_queue_create( NULL, DISPATCH_QUEUE_CONCURRENT ), ^
            {
OUTPUT( "START SAVING FRAME PNG\n" );
                // save the first frame
                char szFullPath[256];
                char szPNGName[64];
                CApp::getValidFileName( szPNGName, "png" );
                getWritePath( szFullPath, szPNGName );
                LodePNG_encode32_file( szFullPath, macPixelBufferCopy, (unsigned int)iWidth, (unsigned int)iHeight );
OUTPUT( "END SAVING FRAME PNG\n" );
            } );
        }
        
        // update state
        if( miState == STATE_RECORDING )
        {
            [self setState:STATE_RECORDING];
        }
        else
        {
            [self setState:STATE_EXPORTING];
        }
    }
    
    mbRefreshTexture = true;
    
}

- (void)buttonPressed:(id)sender
{
    UIButton *button = (UIButton *)sender;
    int iTag = (int)button.tag;
    
    CApp::instance()->setShaderFromIndex( iTag );
    OUTPUT( "button pressed: %d\n", iTag );
}

- (void)setupAssetWriter:(NSURL *)url
{
    NSError* error;
    
    NSLog( @"URL = %@", url );
    
    mpAssetWriter = [[AVAssetWriter alloc] initWithURL:url fileType:AVFileTypeQuickTimeMovie error:&error];
    
    // writer input
    NSDictionary* pVideoSettings = [NSDictionary dictionaryWithObjectsAndKeys:
                                    AVVideoCodecH264, AVVideoCodecKey,
                                    [NSNumber numberWithInt:miVideoWidth], AVVideoWidthKey,
                                    [NSNumber numberWithInt:miVideoHeight], AVVideoHeightKey,
                                    nil];
    mpAssetWriterInput = [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo
                                                        outputSettings:pVideoSettings];
    
    // input adaptor
    NSMutableDictionary *attributes = [[NSMutableDictionary alloc] init];
    [attributes setObject:[NSNumber numberWithUnsignedInt:kCVPixelFormatType_32ARGB] forKey:(NSString*)kCVPixelBufferPixelFormatTypeKey];
    [attributes setObject:[NSNumber numberWithUnsignedInt:miVideoWidth] forKey:(NSString*)kCVPixelBufferWidthKey];
    [attributes setObject:[NSNumber numberWithUnsignedInt:miVideoHeight] forKey:(NSString*)kCVPixelBufferHeightKey];
    
    mpAssetWriterAdaptor = [AVAssetWriterInputPixelBufferAdaptor
                          assetWriterInputPixelBufferAdaptorWithAssetWriterInput:mpAssetWriterInput
                          sourcePixelBufferAttributes:attributes];


    
    
    // add input to the writer
    if( [mpAssetWriter canAddInput:mpAssetWriterInput] )
    {
        [mpAssetWriter addInput:mpAssetWriterInput];
        mpAssetWriterInput.expectsMediaDataInRealTime = YES;
    }
    
    [mpAssetWriter startWriting];
    [mpAssetWriter startSessionAtSourceTime:kCMTimeZero];
}

- (void)startExportVideo
{
OUTPUT( "!!! 0 START EXPORT VIDEO !!!\n" );
    
    NSDictionary *options = [NSDictionary dictionaryWithObjectsAndKeys:
                             [NSNumber numberWithBool:YES], kCVPixelBufferCGImageCompatibilityKey,
                             [NSNumber numberWithBool:YES], kCVPixelBufferCGBitmapContextCompatibilityKey,
                             nil];

    CVReturn status = CVPixelBufferCreate( kCFAllocatorDefault,
                                           miVideoWidth,
                                           miVideoHeight,
                                           kCVPixelFormatType_32BGRA,
                                           (__bridge CFDictionaryRef) options,
                                           &mPixelBuffer );
    assert( status == 0 );
    
    BOOL result = NO;
    
    result = [mpAssetWriterAdaptor appendPixelBuffer:mPixelBuffer withPresentationTime:kCMTimeZero];
    
    miFrameIndex = 0;
    
OUTPUT( "!!! 1 START EXPORT VIDEO !!!\n" );
}

- (void)endExportVideo
{
    printf( "STOP RECORDING\n" );

    [mpAssetWriterInput markAsFinished];
    [mpAssetWriter finishWritingWithCompletionHandler:^{printf( "FINISHED WRITING\n" ); }];
    
    CVPixelBufferRelease( mPixelBuffer );
    CVPixelBufferPoolRelease( mpAssetWriterAdaptor.pixelBufferPool );

    if( miState == STATE_EXPORTING )
    {
        UIAlertView *message = [[UIAlertView alloc] initWithTitle:@""
                                                          message:@"Finished Exporting"
                                                         delegate:self
                                                cancelButtonTitle:@"OK"
                                                otherButtonTitles:nil];
        [message show];
    }
    
    miState = STATE_DEFAULT;
}

- (void)exportVideoFrame:(void *)pOrigData
{
    int iFPS = (int)MOVIE_FRAME_PER_SECONDS;
    
    // wait for ready state
    for( int i = 0; i < 100; i++ )
    {
        if( mpAssetWriterAdaptor.assetWriterInput.readyForMoreMediaData )
        {
            break;
        }
        else
        {
            [NSThread sleepForTimeInterval:0.001];
        }
    }
    
    if( !mpAssetWriterAdaptor.assetWriterInput.readyForMoreMediaData )
    {
        return;
    }
    
    CMTime frameTime = CMTimeMake( 1, iFPS );
    CMTime lastTime = CMTimeMake( miFrameIndex, iFPS );             // current frame / frames per second
    CMTime presentTime = CMTimeAdd( lastTime, frameTime );
    
float fSecond = CMTimeGetSeconds( presentTime );
OUTPUT( "EXPORTING FRAME AT %f SEC\n", fSecond );
    
    // copy data over
    CVPixelBufferLockBaseAddress( mPixelBuffer, 0 );
    
    void* pPixelBufferData = CVPixelBufferGetBaseAddress( mPixelBuffer );
    
    memcpy( pPixelBufferData, pOrigData, sizeof( char ) * miVideoWidth * miVideoHeight * 4 );
    
    CVPixelBufferUnlockBaseAddress( mPixelBuffer, 0 );
    BOOL result = NO;
    
    // append to buffer to write out
    for( int i = 0; i < 100; i++ )
    {
        result = [mpAssetWriterAdaptor appendPixelBuffer:mPixelBuffer withPresentationTime:presentTime];
        
        if( result == NO )
        {
            [NSThread sleepForTimeInterval:0.001];
        }
        else
        {
            break;
        }
    }
        
    assert( result == YES );

    ++miFrameIndex;
    
}

- (void)setupPlayback:(NSURL *)url
{
    // remove old item
    [[mpVideoPlayer currentItem] removeOutput:mpVideoOutput];
    
    // item and asset
    AVAsset* pAsset = [AVURLAsset URLAssetWithURL:url options:nil];
    
    // play
    [pAsset loadValuesAsynchronouslyForKeys:[NSArray arrayWithObjects:@"duration", @"tracks", nil] completionHandler:^
     {
         NSError* error = nil;
         AVKeyValueStatus status = [pAsset statusOfValueForKey:@"duration" error:&error];
         
         if( error != nil )
         {
             NSDictionary *userInfo = [error userInfo];
             NSString *error = [userInfo objectForKey:@"NSUnderlyingError"];
             NSLog(@"The error is: %@", error);
         }
         
         if( status == AVKeyValueStatusLoaded )
         {
             dispatch_async( dispatch_get_main_queue(), ^
                            {
                               // add video output for item
                               mpPlayerItem = [AVPlayerItem playerItemWithAsset:pAsset];
                               mfMovieSeconds = CMTimeGetSeconds( mpPlayerItem.duration );
                               [mpPlayerItem addOutput:mpVideoOutput];
                               
                               // item for the player and play
                               [mpVideoPlayer replaceCurrentItemWithPlayerItem:mpPlayerItem];
                               [mpVideoOutput requestNotificationOfMediaDataChangeWithAdvanceInterval:ONE_FRAME_DURATION];
                               //[mpVideoPlayer play];
                                
                                mpVideoPlayer.actionAtItemEnd = AVPlayerActionAtItemEndNone;
                                [[NSNotificationCenter defaultCenter] addObserver:self
                                                                         selector:@selector( movieFinished: )
                                                                             name:AVPlayerItemDidPlayToEndTimeNotification
                                                                           object:[mpVideoPlayer currentItem]];
                           });
         }
     }
    ];
}

- (void)playMovie:(NSNotification *)notification
{
    self.navigationItem.title = @"Playing Movie";
    
    NSDictionary* pData = [notification userInfo];
    if( pData != nil )
    {
        // movie index
        NSNumber* pNum = [pData objectForKey:@"CellIndex"];
        miMovieIndex = [pNum intValue];
        
        // pause at the start
        [self playMovieAtIndex:miMovieIndex];
        [mpVideoPlayer pause];
        
        // set button title
        UIImage* pImage = [UIImage imageNamed:@"arrow.png"];
        [_playButton setImage:pImage forState:UIControlStateNormal];
        
        [self setState:STATE_PLAYING];
    }
}

- (void)movieFinished:(NSNotification *)notification
{
    self.navigationItem.title = @"";
    
    UIImage* pImage = [UIImage imageNamed:@"arrow.png"];
    [_playButton setImage:pImage forState:UIControlStateNormal];
    
    [_playerSlider setValue:0.0f animated:YES];
    
    [self setState:STATE_DEFAULT];
}

- (void)loadKeyFrames:(int)iMovieIndex
{
    maKeyFrames.clear();
    
    std::vector<std::string> aFileNames;
    getAllFilesInDirectory( "", aFileNames, "nfo" );
    
    char szFullPath[256];
    getWritePathWithDirectory( szFullPath, "", aFileNames[iMovieIndex].c_str() );
    
    TiXmlDocument doc( szFullPath );
    bool bLoaded = doc.LoadFile();
    if( bLoaded )
    {
        TiXmlNode* pNode = doc.FirstChild()->FirstChild( "keyframe" );
        while( pNode )
        {
            TiXmlNode* pChild = pNode->FirstChild();
            
            tKeyFrame keyFrame;
            memset( &keyFrame, 0, sizeof( tKeyFrame ) );
            
            while( pChild )
            {
                const char* szValue = pChild->Value();
                if( !strcmp( szValue, "time" ) )
                {
                    keyFrame.mfTime = atof( pChild->FirstChild()->Value() );
                }
                else if( !strcmp( szValue, "shader" ) )
                {
                    const char* szShader = pChild->FirstChild()->Value();
                    strncpy( keyFrame.mszShaderName, szShader, sizeof( keyFrame.mszShaderName ) );
                }
                else if( !strcmp( szValue, "movie" ) )
                {
                    const char* szMovie = pChild->FirstChild()->Value();
                    strncpy( keyFrame.mszMovieName, szMovie, sizeof( keyFrame.mszMovieName ) );
                }
            
                pChild = pChild->NextSibling();
            }
            
            if( strlen( keyFrame.mszShaderName ) )
            {
                maKeyFrames.push_back( keyFrame );
            }
            
            pNode = pNode->NextSibling( "keyframe" );
        }
    }
}

- (void)playMovieAtIndex:(int)iIndex
{
    self.navigationItem.title = @"Playing";
    
    // load the animation keyframes
    [self loadKeyFrames:iIndex];
    
    // get all the movies
    std::vector<std::string> aFileNames;
    getAllFilesInDirectory( "", aFileNames, "mov" );
    const char* szFileName = aFileNames[iIndex].c_str();
    char szFullPath[256];
    getWritePath( szFullPath, szFileName );
    
    // movie path
    NSString* movieName = [NSString stringWithUTF8String:szFileName];
    NSArray* pPaths = NSSearchPathForDirectoriesInDomains( NSDocumentDirectory, NSUserDomainMask, YES );
    NSString* pDocDir = [pPaths objectAtIndex:0];
    NSString* pFileName = [pDocDir stringByAppendingPathComponent:movieName];
    NSURL* pURL = [NSURL fileURLWithPath:pFileName];
    
    // play
    [self setupPlayback:pURL];
}

- (IBAction)exportButtonPressed:(id)sender
{
    if( miMovieIndex >= 0 )
    {
        mbSaveToAlbum = true;
        
        self.navigationItem.title = @"Saving Movie To Album";
        
        // load the animation keyframes
        [self loadKeyFrames:miMovieIndex];
        
        // get all the movies
        std::vector<std::string> aFileNames;
        getAllFilesInDirectory( "", aFileNames, "mov" );
        const char* szFileName = aFileNames[miMovieIndex].c_str();
        char szFullPath[256];
        getWritePath( szFullPath, szFileName );
        
        // movie path
        NSString* movieName = [NSString stringWithUTF8String:szFileName];
        NSArray* pPaths = NSSearchPathForDirectoriesInDomains( NSDocumentDirectory, NSUserDomainMask, YES );
        NSString* pDocDir = [pPaths objectAtIndex:0];
        NSString* pFileName = [pDocDir stringByAppendingPathComponent:movieName];
        NSURL* pURL = [NSURL fileURLWithPath:pFileName];
        
        // play
        [self setupPlayback:pURL];
        
        [self setState:STATE_EXPORTING];
    }
    else
    {
        UIAlertView *message = [[UIAlertView alloc] initWithTitle:@"Warning"
                                                          message:@"Load a movie first"
                                                         delegate:self
                                                cancelButtonTitle:@"OK"
                                                otherButtonTitles:nil];
        [message show];
    }
}

- (void)setState:(int)iState
{
    miPrevState = miState;
    miState = iState;
}

- (IBAction)filtersButtonPressed:(id)sender
{
    mbFilterShowing = !mbFilterShowing;
    
    if( mbFilterShowing )
    {
        _itemScrollView.alpha = 0.0;
    }
    else
    {
        _itemScrollView.alpha = 1.0;
        
        // delete previous items
        for( UIView* pSubView in [_itemScrollView subviews] )
        {
            [pSubView removeFromSuperview];
        }
        
        _itemScrollView.backgroundColor = [UIColor grayColor];
        
        // add items in category
        
        float fViewSize = 0.0f;
        int iNumShaders = CShaderManager::instance()->getNumShaderPrograms();
        for( int i = 0; i < iNumShaders; i++ )
        {
            UIButton* pButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
            pButton.backgroundColor = [UIColor whiteColor];
            
            tShaderProgram const* pShader = CShaderManager::instance()->getShaderFromIndex( i );
            NSString* pString = [[NSString alloc] initWithUTF8String:pShader->mszText];
            
            [pButton setTitle:pString forState:UIControlStateNormal];
            [pButton addTarget:self action:@selector(buttonPressed:) forControlEvents:UIControlEventTouchDown];
            pButton.tag = i;
            
            // button dimensions
            float fButtonSize = 80.0f * mfViewScale;
            float fButtonY = -fButtonSize * 0.65f;
            float fSpacing = 20.0f * mfViewScale;
            
            pButton.frame = CGRectMake( (CGFloat)( (float)i * ( fButtonSize + fSpacing ) ), fButtonY, fButtonSize, fButtonSize );
            [_itemScrollView addSubview:pButton];
            
            fViewSize += ( fButtonSize + fSpacing );
        }
            
        _itemScrollView.contentSize = CGSizeMake( fViewSize, 10 );
    }
}

- (IBAction)recordButtonPressed:(id)sender
{
    // record, mbRecording will be set during the frame capture steps
    if( miState == STATE_RECORDING )
    {
        CApp::instance()->stopRecording();
        self.navigationItem.title = @"";
        [self setState:STATE_DEFAULT];
    }
    else
    {
        CApp::instance()->startRecording();
        self.navigationItem.title = @"Recording Movie";
        [self setState:STATE_RECORDING];
    }
}

- (IBAction)stopButtonPressed:(id)sender
{
    if( miState == STATE_RECORDING )
    {
        CApp::instance()->stopRecording();
        self.navigationItem.title = @"";
        
        [self setState:STATE_DEFAULT];
    }
    else if( miState == STATE_PLAYING )
    {
        [mpVideoPlayer seekToTime:CMTimeMakeWithSeconds( ceil( mfMovieSeconds ), MOVIE_FRAME_PER_SECONDS )
                  toleranceBefore:kCMTimeZero
                   toleranceAfter:kCMTimeZero];
        
        [mpVideoPlayer play];
        [_playerSlider setValue:0.0f animated:YES];
        //miMovieIndex = -1;
    }
}

- (IBAction)playButtonPressed:(id)sender
{
    // pause
    if( miState == STATE_PLAYING )
    {
        if( [mpVideoPlayer rate] != 0.0 )
        {
            [mpVideoPlayer pause];
            
            UIImage* pImage = [UIImage imageNamed:@"arrow.png"];
            [_playButton setImage:pImage forState:UIControlStateNormal];
        }
        else
        {
            [mpVideoPlayer play];
            
            UIImage* pImage = [UIImage imageNamed:@"pause.png"];
            [_playButton setImage:pImage forState:UIControlStateNormal];
        }
    }
    else
    {
        if( miMovieIndex >= 0 )
        {
            [self playMovieAtIndex:miMovieIndex];
            [self setState:STATE_PLAYING];
        }
        else
        {
            UIAlertView *message = [[UIAlertView alloc] initWithTitle:@"Warning"
                                                              message:@"Load a movie first"
                                                             delegate:self
                                                    cancelButtonTitle:@"OK"
                                                    otherButtonTitles:nil];
            [message show];
        }
    }
}

- (IBAction)sliderValueChanged:(id)sender
{
    if( miMovieIndex >= 0 )
    {
        float fPct = [_playerSlider value];
        float fSeconds = fPct * mfMovieSeconds;
        
OUTPUT( "MOVIE SEEK SECONDS %f\n", fSeconds );
        
        [mpVideoPlayer seekToTime:CMTimeMakeWithSeconds( (Float64)fSeconds, MOVIE_FRAME_PER_SECONDS )
                  toleranceBefore:kCMTimeZero
                   toleranceAfter:kCMTimeZero];
    }
}

@end
