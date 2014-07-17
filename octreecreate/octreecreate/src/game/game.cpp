#include "game.h"

#include "joint.h"
#include "factory.h"
#include "jobmanager.h"

#include "animhierarchy.h"
#include "animsequence.h"
#include "animmodel.h"
#include "animmodelinstance.h"

#include "camera.h"
#include "shadermanager.h"

#include "gamerender.h"

#include "model.h"
#include "modelinstance.h"

#include "level.h"


CFactory<tVector4>      gVectorFactory;
CFactory<tMatrix44>     gMatrixFactory;
CFactory<tJoint>        gJointFactory;
CFactory<tQuaternion>   gQuaternionFactory;

//static int saiVal[5] = { 0, 1, 2, 3, 4 };
static tAnimHierarchy sAnimHierarchy;
static tAnimSequence sAnimSequence;

static tVector4*    saJointPos;
static tVector4*    saJointScale;
static tQuaternion* saJointRot;
static tMatrix44*   saAnimMatrices;

static tVector4*    saJointRotVec;

static tAnimModel   sAnimModel;
//static tAnimModelInstance sAnimModelInstance;

static tModel           sModel;
static tModelInstance   sModelInstance;

static tLevel           sLevel;

static tVector4         sCharVelocity = { 0.0f, 0.0f, 0.0f, 1.0f };
static tVector4         sCharHeading = { 0.0f, 0.0f, 0.0f, 1.0f };
static bool             sbUpdateAnim = false;

tMatrix44 gIdentityMat;

/*
**
*/
void setupTest( void )
{
    // load hiearchy
    animHierarchyInit( &sAnimHierarchy );
    animHierarchyLoad( &sAnimHierarchy, "boss2.cac", &gJointFactory, &gMatrixFactory );
    
    // load sequence
    animSequenceInit( &sAnimSequence );
    animSequenceLoad( &sAnimSequence,
                      "boss2_walk.anim",
                      &sAnimHierarchy,
                      &gVectorFactory,
                      &gQuaternionFactory );
    
    animModelInit( &sAnimModel );
    animModelLoad( &sAnimModel,
                   &sAnimHierarchy,
                   "boss2.cac" );
    
    //animModelInstanceInit( &sAnimModelInstance );
    //animModelInstanceSet( &sAnimModelInstance, &sAnimModel );
    //animModelInstanceSetAnimSequence( &sAnimModelInstance, &sAnimSequence );
    //animModelInstanceSetupGL( &sAnimModelInstance );
    
    //sAnimModelInstance.mpAnimHierarchy = &sAnimHierarchy;
    
    // animated position, scaling, and rotation
    saJointPos = gVectorFactory.alloc( sAnimSequence.miNumJoints );
    saJointScale = gVectorFactory.alloc( sAnimSequence.miNumJoints );
    saJointRot = gQuaternionFactory.alloc( sAnimSequence.miNumJoints );
    saAnimMatrices = gMatrixFactory.alloc( sAnimSequence.miNumJoints );
    
    saJointRotVec = gVectorFactory.alloc( sAnimSequence.miNumJoints );
    
    Matrix44Identity( &gIdentityMat );
    jobManagerInit( gpJobManager );
    
    for( int i = 0; i < sAnimHierarchy.miNumJoints; i++ )
    {
        Matrix44Identity( &saAnimMatrices[i] );
    }
    
    CShaderManager::instance()->loadProgram( "default", "vs.txt", "fs.txt" );
    CShaderManager::instance()->loadProgram( "debug_cull", "vs.txt", "debug_cull_fs.txt" );
    CShaderManager::instance()->loadProgram( "color", "vs.txt", "color_fs.txt" );
    
    tVector4 pos = { -50.0f, 5.0f, 0.0f, 1.0f };
    tVector4 lookAt = { 80.0f, 5.0f, 0.0f, 1.0f };
    
    //tVector4 pos = { 0.0f, 5.0f, -50.0f, 1.0f };
    //tVector4 lookAt = { 0.0f, 5.0f, 80.0f, 1.0f };
    
    tVector4 up = { 0.0f, 1.0f, 0.0f, 1.0f };
    
    CCamera::instance()->setPosition( &pos );
    CCamera::instance()->setLookAt( &lookAt );
    CCamera::instance()->setUp( &up );
    float fFOV = atanf( /*540.0f / 960.0f*/ 320.0f / 480.0f );
    CCamera::instance()->setFOV( fFOV );
    
    gamerRenderInitAllObjects( &sAnimModel, &sAnimSequence, &sAnimHierarchy );
    
    modelInit( &sModel );
    modelLoad( &sModel, "box.cac" );
    
    modelInstanceInit( &sModelInstance );
    modelInstanceSet( &sModelInstance, &sModel );
    //modelInstanceSetupGL( &sModelInstance );
    
    levelInit( &sLevel );
    levelLoad( &sLevel, "castle_scene.cac" );
    
OUTPUT( "!!! FINISH SETUP !!!\n" );
}

/*
**
*/
void updateGame( float fTime )
{
    CCamera* pCamera = CCamera::instance();
    
    const float fSpeed = 0.05f;

    tVector4 const* pPos = gameRenderGetObjectPosition( 0 );
    tVector4 newPos;
    memcpy( &newPos, pPos, sizeof( tVector4 ) );
    newPos.fZ += sCharVelocity.fX * fSpeed;
    newPos.fX += sCharVelocity.fZ * fSpeed;
    
    if( newPos.fY < 0.0f )
    {
        newPos.fY = 0.0f;
    }
    
    const float fScale = 0.6f;
    tVector4 scale = { fScale, fScale, fScale, 1.0f };
    
    gameRenderSetXForm( 0, &newPos, &scale, &sCharHeading );
    
    float fAnimTime = fTime;
    if( !sbUpdateAnim )
    {
        fAnimTime = 0.0f;
    }
    gamerRenderUpdateAllObjects( fAnimTime );

    tVector4 newCamPos, newCamLookAt;
    memcpy( &newCamPos, &newPos, sizeof( tVector4 ) );
    memcpy( &newCamLookAt, &newCamPos, sizeof( tVector4 ) );
    
    newCamPos.fY += 3.0f;
    newCamPos.fX -= 40.0f;
    newCamLookAt.fX += 100.0f;
    
    pCamera->setPosition( &newCamPos );
    pCamera->setLookAt( &newCamLookAt );
    
    pCamera->update( 320, 480 );
    
    levelUpdate( &sLevel, pCamera );
}

/*
**
*/
static CCamera sDebugCamera;
void drawGame( void )
{
    tVector4 debugCamPos = { 0.0f, 200.0f, 0.0f, 1.0f };
    tVector4 debugCamLookAt = { 0.0f, 0.0f, 0.0f, 1.0f };
    tVector4 debugCamUp = { 0.0f, 0.0f, 1.0f, 1.0f };
    
    sDebugCamera.setPosition( &debugCamPos );
    sDebugCamera.setLookAt( &debugCamLookAt );
    sDebugCamera.setUp( &debugCamUp );
    sDebugCamera.update( 480, 640 );
    
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );
    
    CCamera* pCamera = &sDebugCamera; // CCamera::instance();
    tMatrix44 const* pViewMatrix = pCamera->getViewMatrix();
    tMatrix44 const* pProjMatrix = pCamera->getProjectionMatrix();
    
    tMatrix44 invViewMatrix, invTransViewMatrix;
    
    Matrix44Inverse( &invViewMatrix, pViewMatrix );
    Matrix44Transpose( &invTransViewMatrix, &invViewMatrix );
    
    GLint iShader = CShaderManager::instance()->getShader( "default" );
    
#if 0
    animModelInstanceDraw( &sAnimModelInstance,
                           pViewMatrix,
                           pProjMatrix,
                           iShader );
#endif // #if 0
    
    gameRenderDraw( pViewMatrix,
                    pProjMatrix,
                    iShader );
    
    levelDrawCullDebug( &sLevel, pViewMatrix, pProjMatrix );
    //levelDraw( &sLevel, pViewMatrix, pProjMatrix, iShader );
    
    glDisable( GL_CULL_FACE );
}

/*
**
*/
static tVector2 sStartTouchPos = { -1.0f, -1.0f };
void gameInputUpdate( float fX, float fY, int iTouchType )
{
#if 0
    CCamera* pCamera = CCamera::instance();
    if( iTouchType == TOUCHTYPE_BEGIN )
    {
        sStartTouchPos.fX = fX;
        sStartTouchPos.fY = fY;
    }
    else if( iTouchType == TOUCHTYPE_MOVE )
    {
        tVector4 diff =
        {
            fX - sStartTouchPos.fX,
            fY - sStartTouchPos.fY
        };
        
        tVector4 const* pCamPos = pCamera->getPosition();
        tVector4 const* pCamLookAt = pCamera->getLookAt();
        
        tVector4 newPos =
        {
            pCamPos->fX + diff.fX,
            pCamPos->fY,
            pCamPos->fZ + diff.fY,
            1.0f
        };
        
        tVector4 newLookAt =
        {
            pCamLookAt->fX + diff.fX,
            pCamLookAt->fY,
            pCamLookAt->fZ + diff.fY,
            1.0f
        };
        
        pCamera->setPosition( &newPos );
        pCamera->setLookAt( &newLookAt );
        
        sStartTouchPos.fX = fX;
        sStartTouchPos.fY = fY;
    }
#endif // #if 0
    
    if( iTouchType == TOUCHTYPE_BEGIN )
    {
        sStartTouchPos.fX = fX;
        sStartTouchPos.fY = fY;
        if( fY < 100.0f )
        {
            sCharVelocity.fZ = 1.0f;
            sCharHeading.fX = 3.14159f;
        }
        else if( fY >= 100.0f && fY < 380.0f )
        {
            if( fX > 160.0f )
            {
                sCharVelocity.fX = -1.0f;
                sCharHeading.fY = 3.14159f;
            }
            else
            {
                sCharVelocity.fX = 1.0f;
                sCharHeading.fY = 0.0f;
            }
        }
        else
        {
            sCharVelocity.fZ = -1.0f;
            sCharHeading.fX = 0.0f;
        }
        
        sbUpdateAnim = true;
    }
    else if( iTouchType == TOUCHTYPE_MOVE )
    {
        if( fY < 100.0f )
        {
            sCharVelocity.fZ = 1.0f;
            sCharHeading.fX = 3.14159f;
        }
        else if( fY >= 100.0f && fY < 380.0f )
        {
            if( fX > 160.0f )
            {
                sCharVelocity.fX = -1.0f;
                sCharHeading.fY = 3.14159f;
            }
            else
            {
                sCharVelocity.fX = 1.0f;
                sCharHeading.fY = 0.0f;
            }
        }
        else
        {
            sCharVelocity.fZ = -1.0f;
            sCharHeading.fX = 0.0f;
        }
        
        sbUpdateAnim = true;
    }
    else if( iTouchType == TOUCHTYPE_END )
    {
        memset( &sCharVelocity, 0, sizeof( sCharVelocity ) );
        sbUpdateAnim = false;
    }
    
}
