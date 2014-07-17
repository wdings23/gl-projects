//
//  ViewController.m
//  worldviewer
//
//  Created by Dingwings on 5/7/14.
//  Copyright (c) 2014 Dingwings. All rights reserved.
//

#import "ViewController.h"

#include "render.h"
#include "game.h"
#include "timeutil.h"
#include "hud.h"

@interface ViewController () {
    float           _retinaScale;
    int             _screenWidth;
    int             _screenHeight;
    double          _lastTime;
    float           _totalTime;
    float           _lastDistance;
    
    CGame*          _game;
    UITouch*        _maTouches[5];
}

@property (strong, nonatomic) EAGLContext *context;
@property (strong, nonatomic) GLKBaseEffect *effect;

- (void)setupGL;
- (void)tearDownGL;

- (BOOL)loadShaders;
- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file;
- (BOOL)linkProgram:(GLuint)prog;
- (BOOL)validateProgram:(GLuint)prog;
@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
 
    self.view.multipleTouchEnabled = YES;
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }
    
    // 60 fps
    self.preferredFramesPerSecond = 60;
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    
    CGFloat scale = [[UIScreen mainScreen] scale];
    CGRect screenBounds = [[UIScreen mainScreen] bounds];
    
    renderSetScreenWidth( (int)screenBounds.size.width );
    renderSetScreenHeight( (int)screenBounds.size.height );
    renderSetScreenScale( (float)scale );
    
    _retinaScale = scale;

    [self setupGL];
    
    _game = CGame::instance();
    _game->setSceneFileName( "scene.cac" );
    _game->init();
    
    _lastTime = 0.0;
    _lastDistance = -1.0f;
    
    memset( _maTouches, 0, sizeof( _maTouches ) );
}

- (void)dealloc
{    
    [self tearDownGL];
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil)) {
        self.view = nil;
        
        [self tearDownGL];
        
        if ([EAGLContext currentContext] == self.context) {
            [EAGLContext setCurrentContext:nil];
        }
        self.context = nil;
    }

    // Dispose of any resources that can be recreated.
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];
    
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    if( _lastTime == 0.0 )
    {
        _lastTime = getTime();
    }
    
    double fDT = ( getTime() - _lastTime ) * 0.001;
    
    _game->update( fDT );
    
    _lastTime = getTime();
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    glClearColor(0.65f, 0.65f, 0.65f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    _game->draw();
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	// get all the touches
	int iTouch = 0;
    for (UITouch *pTouch in touches)
	{
        // save touch
        for( int i = 0; i < sizeof( _maTouches ) / sizeof( *_maTouches ); i++ )
        {
            if( _maTouches[i] == NULL )
            {
                _maTouches[i] = pTouch;
                break;
            }
        }
        
        ++iTouch;
    }
    
    // get the positions
    tVector2 aPos[5];
    int iNumTouches = 0;
    for( int i = 0; i < sizeof( _maTouches ) / sizeof( *_maTouches ); i++ )
    {
        if( _maTouches[i] )
        {
            CGPoint touchPt = [_maTouches[i] locationInView:self.view];
            
            aPos[iNumTouches].fX = (float)( touchPt.x * _retinaScale );
            aPos[iNumTouches].fY = (float)( touchPt.y * _retinaScale );
            ++iNumTouches;
        }
    }
    
    OUTPUT( "!!! TOUCH BEGAN NUM TOUCHES: %d !!!\n", iNumTouches );
    
    if( iNumTouches == 1 )
    {
        _game->inputUpdate( aPos[0].fX, aPos[0].fY, TOUCHTYPE_BEGAN );
        CHUD::instance()->inputUpdate( aPos[0].fX, aPos[0].fY, 0, 1, TOUCHTYPE_BEGAN );
    }
    else
    {
        _lastDistance = -1.0f;
    }
}


- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    tVector2 aPos[5];
    int iNumTouches = 0;
    
	// get all the touches
    int iCount = 0;
    for( int i = 0; i < sizeof( _maTouches ) / sizeof( *_maTouches ); i++ )
    {
        if( _maTouches[i] )
        {
            CGPoint touchPt = [_maTouches[i] locationInView:self.view];
            aPos[iCount].fX = (float)( touchPt.x * _retinaScale );
            aPos[iCount].fY = (float)( touchPt.y * _retinaScale );
            ++iCount;
        }
    }
    
    for( int i = 0; i < sizeof( _maTouches ) / sizeof( *_maTouches ); i++ )
    {
        if( _maTouches[i] )
        {
            ++iNumTouches;
        }
    }
    
    OUTPUT( "MOVING NUM TOUCHES: %d\n", iNumTouches );
    
    if( iNumTouches == 1 )
    {
        _game->inputUpdate( aPos[0].fX, aPos[0].fY, TOUCHTYPE_MOVED );
        CHUD::instance()->inputUpdate( aPos[0].fX, aPos[0].fY, 0, 1, TOUCHTYPE_MOVED );
    }
    else
    {
        tVector2 diff =
        {
            aPos[0].fX - aPos[1].fX,
            aPos[0].fY - aPos[1].fY
        };
        
        float fDistance = sqrtf( diff.fX * diff.fX + diff.fY * diff.fY );
        if( _lastDistance < 0.0f )
        {
            _lastDistance = fDistance;
        }
        
        float fDiff = fDistance - _lastDistance;
        _game->zoomCamera( fDiff * 0.01f );
        
        _lastDistance = fDistance;
    }
}

-(void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    tVector2 aPos[5];
    int iNumTouches = 0;
    int aiToDelete[5] = { -1, -1, -1, -1, -1 };
    
    
    // get all the touches
	for (UITouch *pTouch in touches)
	{
        // get position
        for( int i = 0; i < sizeof( _maTouches ) / sizeof( *_maTouches ); i++ )
        {
            if( _maTouches[i] == pTouch )
            {
                CGPoint touchPt = [_maTouches[i] locationInView:self.view];
                aPos[i].fX = (float)( touchPt.x * _retinaScale );
                aPos[i].fY = (float)( touchPt.y * _retinaScale );
                
                aiToDelete[i] = i;
            }
        }
    }
    
    // get position
    for( int i = 0; i < sizeof( _maTouches ) / sizeof( *_maTouches ); i++ )
    {
        if( _maTouches[i] )
        {
            ++iNumTouches;
        }
        
        if( aiToDelete[i] != -1 )
        {
            _maTouches[i] = NULL;
        }
    }
    
    OUTPUT( "!!! TOUCH ENDED NUM TOUCHES: %d !!!\n", iNumTouches );
    if( iNumTouches == 1 )
    {
        _game->inputUpdate( aPos[0].fX, aPos[0].fY, TOUCHTYPE_ENDED );
        CHUD::instance()->inputUpdate( aPos[0].fX, aPos[0].fY, 0, 1, TOUCHTYPE_ENDED );
        
        _game->clearInputPos();
    }
    else
    {
        _game->clearInputPos();
    }
}

@end
