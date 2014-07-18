//
//  ViewController.h
//  VidProcess
//
//  Created by Dingwings on 6/7/14.
//  Copyright (c) 2014 Dingwings. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>
#import <AVFoundation/AVCaptureSession.h>
#import <AVFoundation/AVCaptureDevice.h>
#import <AVFoundation/AVFoundation.h>

#include <pthread.h>
#include <vector>
#include <string>
#include "app.h"

enum
{
    STATE_DEFAULT = 0,
    STATE_RECORDING,
    STATE_EXPORTING,
    STATE_PLAYING,
    
    NUM_STATES,
};

@interface ViewController : UIViewController <GLKViewDelegate,
                                              AVCaptureVideoDataOutputSampleBufferDelegate,
                                              UIScrollViewDelegate,
                                              UITabBarDelegate,
                                              AVPlayerItemOutputPullDelegate>
{
    GLKView*                    mpGLKView;
    EAGLContext*                mpContext;
    
    AVCaptureSession*           mpCaptureSession;
    AVAssetWriter*              mpAssetWriter;
    AVAssetWriterInputPixelBufferAdaptor* mpAssetWriterAdaptor;
    AVAssetWriterInput*         mpAssetWriterInput;
    AVCaptureMovieFileOutput*   mpAssetOutput;
    
    AVPlayerItemVideoOutput*    mpVideoOutput;
    AVPlayer*                   mpVideoPlayer;
    AVPlayerItem*               mpPlayerItem;
    
    CVPixelBufferRef            mPixelBuffer;
    
    dispatch_queue_t            mVidOutQueue;
    
    CGFloat                     mfWidth;
    CGFloat                     mfHeight;
    
    int                         miVideoWidth;
    int                         miVideoHeight;
    
    double                      mfLastTime;
    int                         miScrollItemType;
    
    int                         miFrameIndex;
    
    unsigned char*              macFrameBuffer;
    unsigned char*              macPixelBufferCopy;
    
    pthread_mutex_t             mMutex;
    bool                        mbRefreshTexture;
    
    float                       mfViewScale;

    std::vector<tKeyFrame>      maKeyFrames;
    
    bool                        mbSaveToAlbum;
    std::string                 mExportFileName;
    int                         miMovieIndex;
    
    bool                        mbFilterShowing;
    
    int                         miPrevState;
    int                         miState;
    float                       mfMovieSeconds;
}

@property (weak, nonatomic) IBOutlet UIScrollView *itemScrollView;
@property (weak, nonatomic) IBOutlet UITabBar *tabBar;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *saveButton;
@property (weak, nonatomic) IBOutlet UISlider *playerSlider;
@property (weak, nonatomic) IBOutlet UIButton *recordButton;
@property (weak, nonatomic) IBOutlet UIButton *playButton;
@property (weak, nonatomic) IBOutlet UIButton *stopButton;
@property (weak, nonatomic) IBOutlet UINavigationBar *navigationBar;
@property (weak, nonatomic) IBOutlet UIButton *filterButton;

@end
