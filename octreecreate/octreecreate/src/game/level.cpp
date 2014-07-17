//
//  level.cpp
//  animtest
//
//  Created by Dingwings on 8/9/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

/**********************
 
 On demand load:
    Initialize models and model instances, don't load the data yet. They just have names(id) in the entries for
    loading from the level file. Add the model instances to the oct-grid, which is loaded from file(will generate the oct-grid 
    data off-line). While going around the level, any model instances within the visible oct-nodes will need to be loaded. 
    The models pointed by the model instances will be loaded first, then the model instances' data can then to be set with
    the loaded models.
    
    Load from grid data first and load model and model instances as needed in the oct nodes.
 
***********************/

#include "level.h"
#include "fileutil.h"
#include "tinyxml.h"
#include "factory.h"
#include "animhierarchy.h"
#include "jobmanager.h"
#include "gamerender.h"
#include "shadermanager.h"
#include "octgrid.h"

#define NUM_VISIBLE_PER_ALLOC 1000

static void loadModels( tModel** apModels, TiXmlNode* pNode );
static void loadModelInstances( tModel** apModels, int iNumModels, tModelInstance* aModelInstances, TiXmlNode* pRoot );
static void loadModelNames( tModel** apModels, TiXmlNode* pNode );

static void parseVector( const char* szVector, tVector4* pV );

static void loadXForms( tModelInstance* pModelInstance, int iDepth, TiXmlNode* pNode );
static void getNumXForms( TiXmlNode* pNode, int* piNumDepths );

static void loadAnimModelXForms( tAnimModelInstance* pModelInstance, int iDepth, TiXmlNode* pNode );

static void loadAnimModels( tAnimModel** apAnimModels,
                            tAnimHierarchy** apAnimHierarchies,
                            TiXmlNode* pNode );

static void loadAnimModelInstances( tAnimModel** apAnimModels,
                                   int iNumAnimModels,
                                   tAnimModelInstance* aAnimModelInstances,
                                   tAnimHierarchy** apAnimHierarchies,
                                   TiXmlNode* pRoot );

static bool nodeIntersect( tOctNode const* pOctNode, tVector4 const* pPt0, tVector4 const* pPt1, tVector4 const* pPt2 );
static bool isModelInOctNode( tOctNode const* pOctNode, void* pObject);
static bool isAnimModelInOctNode( tOctNode const* pOctNode, void* pObject );

CFactory<tModel>                gModelFactory;
CFactory<tAnimModel>            gAnimModelFactory;
CFactory<tAnimHierarchy>        gGameAnimHierarchyFactory;
CFactory<tJoint>                gGameJointFactory;
CFactory<tMatrix44>             gGameMatrixFactory;
CFactory<tAnimSequence>         gGameAnimSequenceFactory;
CFactory<tQuaternion>           gGameQuaternionFactory;
CFactory<tVector4>              gGameVectorFactory;

/*
**
*/
void levelInit( tLevel* pLevel )
{
    memset( pLevel, 0, sizeof( tLevel ) );
    
    pLevel->miNumVisibleAlloc = NUM_VISIBLE_PER_ALLOC;
    pLevel->mapVisible = (tModelInstance **)malloc( sizeof( tModelInstance* ) * pLevel->miNumVisibleAlloc );

    pLevel->miNumVisibleAnimModelAlloc = NUM_VISIBLE_PER_ALLOC;
    pLevel->mapVisibleAnimModels = (tAnimModelInstance **)malloc( sizeof( tAnimModelInstance* ) * pLevel->miNumVisibleAnimModelAlloc );
    
}

/*
**
*/
void levelRelease( tLevel* pLevel )
{
    free( pLevel->maModelInstances );
    free( pLevel->mapVisible );
    free( pLevel->mapModels );
    
    free( pLevel->mapAnimModels );
    free( pLevel->maAnimModelInstances );

    free( pLevel->mapVisibleAnimModels );
    
    free( pLevel->mapAnimHierarchies );
    free( pLevel->mapAnimModelInstanceHierarchies );
    
    memset( pLevel, 0, sizeof( tLevel ) );
}

/*
**
*/
void levelLoad( tLevel* pLevel, const char* szFileName )
{
    char szFullPath[256];
    getFullPath( szFullPath, szFileName );
    TiXmlDocument doc( szFullPath );
    
    bool bLoaded = doc.LoadFile();
    if( bLoaded )
    {
        // get the number of models
        int iNumModels = 0;
        TiXmlNode* pMeshNode = doc.FirstChild()->FirstChild( "model" );
        while( pMeshNode )
        {
            ++iNumModels;
            pMeshNode = pMeshNode->NextSibling( "model" );
        }
        
        int iNumAnimModels = 0;
        TiXmlNode* pAnimModelNode = doc.FirstChild()->FirstChild( "animmodel" );
        while( pAnimModelNode )
        {
            ++iNumAnimModels;
            pAnimModelNode = pAnimModelNode->NextSibling( "animmodel" );
        }
        
        int iNumAnimModelInstances = 0;
        int iNumModelInstances = 0;
        
        TiXmlNode* pModelInstanceNode = doc.FirstChild()->FirstChild( "instance" );
        while( pModelInstanceNode )
        {
            // check if it's a animation model
            TiXmlNode* pHierarchy = pModelInstanceNode->FirstChild( "animmodel" );
            if( pHierarchy )
            {
                ++iNumAnimModelInstances;
            }
            else
            {
                ++iNumModelInstances;
            }
            
            pModelInstanceNode = pModelInstanceNode->NextSibling( "instance" );
        }
        
        // add to factory
        tModel* aModels = gModelFactory.alloc( iNumModels );
        
        // save the models
        pLevel->miNumModels = iNumModels;
        pLevel->mapModels = (tModel **)malloc( sizeof( tModel* ) * iNumModels );
        memset( pLevel->mapModels, 0, sizeof( tModel* ) * iNumModels );
        for( int i = 0; i < iNumModels; i++ )
        {
            pLevel->mapModels[i] = &aModels[i];
            
        }   // for i = 0 to num models
        
        // model instances
        pLevel->miNumModelInstances = iNumModelInstances;
        pLevel->maModelInstances = (tModelInstance *)malloc( sizeof( tModelInstance ) * iNumModelInstances );
        
        // load the models
        TiXmlNode* pRootNode = doc.FirstChild();
        
#if defined( ONDEMAND_LOAD )
        loadModelNames( pLevel->mapModels, pRootNode );
#else
        loadModels( pLevel->mapModels, pRootNode );
        loadModelInstances( pLevel->mapModels, iNumModels, pLevel->maModelInstances, pRootNode );
#endif // #if ONDEMAND_LOAD
        
        // animated models
        tAnimModel* aAnimModels = gAnimModelFactory.alloc( iNumAnimModels );
        pLevel->miNumAnimModels = iNumAnimModels;
        pLevel->mapAnimModels = (tAnimModel **)malloc( sizeof( tAnimModel* ) * iNumAnimModels );
        memset( pLevel->mapAnimModels, 0, sizeof( tAnimModel* ) * iNumAnimModels );
        
        tAnimHierarchy* aAnimHierarchies = gGameAnimHierarchyFactory.alloc( iNumAnimModels );
        pLevel->mapAnimHierarchies = (tAnimHierarchy **)malloc( sizeof( tAnimHierarchy* ) * iNumAnimModels );
        memset( pLevel->mapAnimHierarchies, 0, sizeof( tAnimHierarchy* ) * iNumAnimModels );
        
        for( int i = 0; i < iNumAnimModels; i++ )
        {
            pLevel->mapAnimModels[i] = &aAnimModels[i];
            pLevel->mapAnimHierarchies[i] = &aAnimHierarchies[i];
        }
        
        pLevel->miNumAnimModelInstances = iNumAnimModelInstances;
        pLevel->maAnimModelInstances = (tAnimModelInstance *)malloc( sizeof( tAnimModelInstance ) * iNumAnimModelInstances );
        
        tAnimHierarchy* aAnimInstanceHierarchy = gGameAnimHierarchyFactory.alloc( iNumAnimModelInstances );
        pLevel->mapAnimModelInstanceHierarchies = (tAnimHierarchy **)malloc( sizeof( tAnimHierarchy* ) * iNumAnimModelInstances );
        memset( pLevel->mapAnimModelInstanceHierarchies, 0, sizeof( tAnimHierarchy* ) * iNumAnimModelInstances );
        
        for( int i = 0; i < iNumAnimModelInstances; i++ )
        {
            pLevel->mapAnimModelInstanceHierarchies[i] = &aAnimInstanceHierarchy[i];
        }
        
        loadAnimModels( pLevel->mapAnimModels, pLevel->mapAnimHierarchies,  pRootNode );
        loadAnimModelInstances( pLevel->mapAnimModels,
                                iNumAnimModels,
                                pLevel->maAnimModelInstances,
                                pLevel->mapAnimModelInstanceHierarchies,
                                pRootNode );
        
        
        octGridInit( &pLevel->mGrid, 100.0f, 10.0f );
        
        // add model instance to grid
        for( int i = 0; i < iNumModelInstances; i++ )
        {
            tModelInstance* pModelInstance = &pLevel->maModelInstances[i];
            octGridAddObject( &pLevel->mGrid,
                              pModelInstance,
                              isModelInOctNode );
        }
        
        /*for( int i = 0; i < iNumAnimModelInstances; i++ )
        {
            tAnimModelInstance* pAnimModelInstance = &pLevel->maAnimModelInstances[i];
            octGridAddObject( &pLevel->mGrid,
                              pAnimModelInstance,
                              isAnimModelInOctNode );
        }*/
    }
    else
    {
        OUTPUT( "error loading %s : %s\n", szFileName, doc.ErrorDesc() );
        WTFASSERT2( bLoaded, "can't load %s", szFileName );
    }

    octGridSave( &pLevel->mGrid, "level_grid.txt" );
}

// TODO: allocObject checks the current level DB, if not loaded,
//       load. Don't pre-alloc everything in the beginning

/*
**
*/
static void* allocObject( const char* szName, void* pUserData )
{
    void* pRet = NULL;
    tLevel* pLevel = (tLevel *)pUserData;
    
    // find the model instance with the name
    for( int i = 0; i < pLevel->miNumModelInstances; i++ )
    {
        // check if already alloc'd
        tModelInstance* pModelInstance = &pLevel->maModelInstances[i];
        if( !strcmp( pModelInstance->mszName, szName ) )
        {
            pRet = pModelInstance;
            break;
        }
        
    }   // for i = 0 to num model instances
    
    
    
    return pRet;
}

/*
**
*/
void levelLoadOnDemand( tLevel* pLevel, const char* szFileName )
{
    octGridLoad( &pLevel->mGrid, "level_grid.txt", allocObject, pLevel );
}

static bool checkModelFromCamera( void* pLeft, void* pRight )
{
    tVector4 const* pCamPos = CCamera::instance()->getPosition();
    
    tModelInstance* pLeftModel = (tModelInstance *)pLeft;
    tModelInstance* pRightModel = (tModelInstance *)pRight;
    
    tMatrix44 const* pXFormMatLeft = &pLeftModel->mXFormMat;
    tMatrix44 const* pXFormMatRight = &pRightModel->mXFormMat;
    
    tVector4 leftPos =
    {
        pXFormMatLeft->M( 0, 3 ),
        pXFormMatLeft->M( 1, 3 ),
        pXFormMatLeft->M( 2, 3 ),
        1.0f
    };
    
    tVector4 rightPos =
    {
        pXFormMatRight->M( 0, 3 ),
        pXFormMatRight->M( 1, 3 ),
        pXFormMatRight->M( 2, 3 ),
        1.0f
        
    };
    
    tVector4 diffLeft, diffRight;
    Vector4Subtract( &diffLeft, pCamPos, &leftPos );
    Vector4Subtract( &diffRight, pCamPos, &rightPos );
    
    float fLengthLeft = Vector4Dot( &diffLeft, &diffLeft );
    float fLengthRight = Vector4Dot( &diffRight, &diffRight );
    
    return (fLengthLeft < fLengthRight);
}

/*
**
*/
void levelUpdate( tLevel* pLevel, CCamera const* pCamera )
{
    // get the visible nodes
    pLevel->maVisibleNodes.clear();
    octGridGetVisibleNodes( &pLevel->mGrid,
                            pCamera,
                            pLevel->maVisibleNodes );
    
    std::vector<void *>* paVisibleModels = &pLevel->maVisibleModels;
    paVisibleModels->clear();
    
    // all the models in the node
    std::vector<tOctNode const*>::iterator iter = pLevel->maVisibleNodes.begin();
    for( ; iter != pLevel->maVisibleNodes.end(); iter++ )
    {
        tOctNode const* pNode = *iter;
        for( int i = 0; i < pNode->miNumObjects; i++ )
        {            
            tModelInstance* pModelInstance = (tModelInstance *)pNode->mapObjects[i];
            if( !pModelInstance->mbDrawn )
            {
                tVector4 modelPos =
                {
                    pModelInstance->mXFormMat.M( 0, 3 ),
                    pModelInstance->mXFormMat.M( 1, 3 ),
                    pModelInstance->mXFormMat.M( 2, 3 ),
                    1.0f
                };
                
                // check if it's in the frustum
                if( pCamera->sphereInFrustum( &modelPos, pModelInstance->mfRadius ) )
                {
                    paVisibleModels->push_back( pNode->mapObjects[i] );
                    pModelInstance->mbDrawn = true;
                }
            }
        
        }   // for i = 0 to num objects in node
        
    }   // traverse visible list
    
    tAnimModelInstance** apVisibleAnimModel = pLevel->mapVisibleAnimModels;
    int iNumVisibleAnimModelAlloc = pLevel->miNumVisibleAnimModelAlloc;
    
    // update model transform
    std::vector<void *>::iterator visibleIter = paVisibleModels->begin();
    for( ; visibleIter != paVisibleModels->end(); visibleIter++ )
    {
        tModelInstance* pModelInstance = (tModelInstance *)*visibleIter;
        modelInstanceUpdateXForm( pModelInstance );
        
    }   // for iter = start to end
    
    OUTPUT( "# visible models = %d\n", (int)paVisibleModels->size() );
    
    // sort models by distance from camera
    std::sort( paVisibleModels->begin(), paVisibleModels->end(), checkModelFromCamera );
    
    pLevel->miNumVisible = (int)paVisibleModels->size();
    
    // anim model
    int iNumVisibleAnimModels = 0;
    
    // TODO: CACHE THIS
    int iNumAnimModelInstances = pLevel->miNumAnimModelInstances;
    tVector4** aaScales = (tVector4 **)malloc( sizeof( tVector4* ) * iNumAnimModelInstances );
    tVector4** aaPositions = (tVector4 **)malloc( sizeof( tVector4* ) * iNumAnimModelInstances );
    tVector4** aaRotationVecs = (tVector4 **)malloc( sizeof( tVector4* ) * iNumAnimModelInstances );
    tQuaternion** aaRotations = (tQuaternion **)malloc( sizeof( tQuaternion* ) * iNumAnimModelInstances );
    tMatrix44** aaAnimMatrices = (tMatrix44 **)malloc( sizeof( tMatrix44* ) * iNumAnimModelInstances );
    
    for( int i = 0; i < iNumAnimModelInstances; i++ )
    {
        int iNumJoints = pLevel->mapAnimHierarchies[i]->miNumJoints;
        
        aaScales[i] = (tVector4 *)malloc( sizeof( tVector4 ) * iNumJoints );
        aaPositions[i] = (tVector4 *)malloc( sizeof( tVector4 ) * iNumJoints );
        aaRotationVecs[i] = (tVector4 *)malloc( sizeof( tVector4 ) * iNumJoints );
        aaRotations[i] = (tQuaternion *)malloc( sizeof( tQuaternion ) * iNumJoints );
        aaAnimMatrices[i] = (tMatrix44 *)malloc( sizeof( tMatrix44 ) * iNumJoints );
    
        for( int j = 0; j < iNumJoints; j++ )
        {
            Matrix44Identity( &aaAnimMatrices[i][j] );
        }
    }
    
    static float sfAnimTime = 0.0f;
    
    for( int i = 0; i < iNumAnimModelInstances; i++ )
    {
        tAnimModelInstance* pAnimModelInstance = &pLevel->maAnimModelInstances[i];
        tAnimHierarchy const* pAnimModelHierarchy = pAnimModelInstance->mpAnimModel->mpAnimHierarchy;
        tAnimSequence const* pAnimSequence = pAnimModelInstance->mpAnimSequence;
        
        tVector4 pos =
        {
            pAnimModelInstance->mXFormMat.M( 0, 3 ),
            pAnimModelInstance->mXFormMat.M( 1, 3 ),
            pAnimModelInstance->mXFormMat.M( 2, 3 ),
            1.0f
        };
        
        tVector4* paPositions = aaPositions[i];
        tVector4* paScalings = aaScales[i];
        tVector4* paRotationVecs = aaRotationVecs[i];
        tQuaternion* paRotations = aaRotations[i];
        tMatrix44* paAnimMatrices = aaAnimMatrices[i];
        
#if 0
        OUTPUT( "paPositions = 0x%8X\n", (unsigned int)paPositions );
        OUTPUT( "paScalings = 0x%8X\n", (unsigned int)paScalings );
        OUTPUT( "paRotationVecs = 0x%8X\n", (unsigned int)paRotationVecs );
        OUTPUT( "paRotations = 0x%8X\n", (unsigned int)paRotations );
        OUTPUT( "paAnimMatrices = 0x%8X\n", (unsigned int)paAnimMatrices );
#endif // 3if 0
        
        if( pCamera->sphereInFrustum( &pos, pAnimModelInstance->mfRadius * pAnimModelInstance->mXFormMat.M( 0, 0 ) ) )
        {
            gameRenderAnimModelJob( pAnimSequence,
                                    pAnimModelHierarchy,
                                    pLevel->mapAnimModelInstanceHierarchies[i],
                                    paScalings,
                                    paPositions,
                                    paRotations,
                                    paRotationVecs,
                                    paAnimMatrices,
                                    sfAnimTime += 0.01f );
        }
        
        if( iNumVisibleAnimModels >= iNumVisibleAnimModelAlloc )
        {
            pLevel->miNumVisibleAnimModelAlloc += NUM_VISIBLE_PER_ALLOC;
            pLevel->mapVisibleAnimModels = (tAnimModelInstance **)realloc( pLevel->mapVisibleAnimModels, sizeof( tAnimModelInstance* ) * pLevel->miNumVisibleAnimModelAlloc );
            apVisibleAnimModel = pLevel->mapVisibleAnimModels;
            
            iNumVisibleAnimModelAlloc = pLevel->miNumVisibleAnimModelAlloc;
        }
        
        apVisibleAnimModel[iNumVisibleAnimModels++] = pAnimModelInstance;
        pLevel->miNumVisibleAnimModels = iNumVisibleAnimModels;
        
    } // for i = 0 to num anim model instances
    
    jobManagerWait( gpJobManager );

    for( int i = 0; i < iNumVisibleAnimModels; i++ )
    {
        animModelInstanceUpdate( apVisibleAnimModel[i] );
    }
    
    jobManagerWait( gpJobManager );
    
    for( int i = 0; i < iNumVisibleAnimModels; i++ )
    {
        animModelInstanceUpdateXForm( apVisibleAnimModel[i] );
    }
    
    for( int i = 0; i < iNumVisibleAnimModels; i++ )
    {
        animModelInstanceCopyVBData( apVisibleAnimModel[i] );
    }
    
    for( int i = 0; i < iNumAnimModelInstances; i++ )
    {
        free( aaScales[i] );
        free( aaPositions[i] );
        free( aaRotationVecs[i] );
        free( aaRotations[i] );
    }
    
    free( aaScales );
    free( aaPositions );
    free( aaRotationVecs );
    free( aaRotations );
}

/*
**
*/
void levelDraw( tLevel* pLevel,
                tMatrix44 const* pViewMatrix,
                tMatrix44 const* pProjectionMatrix,
                int iShader )
{
#if 0
    tModelInstance** apVisible = pLevel->mapVisible;
    for( int i = 0; i < pLevel->miNumVisible; i++ )
    {
        modelInstanceDraw( *apVisible, pViewMatrix, pProjectionMatrix, iShader );
        ++apVisible;
        
    }   // for i = 0 to num visible
#endif // #if 0
    
    std::vector<void *>* paVisibleModels = &pLevel->maVisibleModels;
    
    std::vector<void *>::iterator iter = paVisibleModels->begin();
    for( ; iter != paVisibleModels->end(); iter++ )
    {
        tModelInstance* pModelInstance = (tModelInstance *)*iter;
        modelInstanceDraw( pModelInstance, pViewMatrix, pProjectionMatrix, iShader );
        pModelInstance->mbDrawn = false;
    }
    
    tAnimModelInstance** apVisibleAnimModels = pLevel->mapVisibleAnimModels;
    for( int i = 0; i < pLevel->miNumVisibleAnimModels; i++ )
    {
        animModelInstanceDraw( *apVisibleAnimModels, pViewMatrix, pProjectionMatrix, iShader );
        ++apVisibleAnimModels;
    }
}

/*
**
*/
void levelDrawCullDebug( tLevel* pLevel,
                         tMatrix44 const* pViewMatrix,
                         tMatrix44 const* pProjMatrix )
{
    std::vector<void *>* paVisibleModels = &pLevel->maVisibleModels;
    
    int iDrawShader = CShaderManager::instance()->getShader( "default" );
    int iCullShader = CShaderManager::instance()->getShader( "debug_cull" );
    
    for( int i = 0; i < pLevel->miNumModelInstances; i++ )
    {
        tModelInstance* pModelInstance = &pLevel->maModelInstances[i];
        std::vector<void *>::iterator it = std::find( paVisibleModels->begin(), paVisibleModels->end(), pModelInstance );
        if( it != paVisibleModels->end() )
        {
            modelInstanceDraw( pModelInstance, pViewMatrix, pProjMatrix, iDrawShader );
        }
        else
        {
            modelInstanceDraw( pModelInstance, pViewMatrix, pProjMatrix, iCullShader );
        }
        
        // reset
        pModelInstance->mbDrawn = false;
        
    }   // for i = 0 to num model instances

    // draw frustum
    CCamera const* pCamera = CCamera::instance();
    
    tVector4 aTopLeftRay[] =
    {
        { pCamera->mNearTopLeft.fX, pCamera->mNearTopLeft.fY, pCamera->mNearTopLeft.fZ, 1.0f },
        { pCamera->mFarTopLeft.fX, pCamera->mFarTopLeft.fY, pCamera->mFarTopLeft.fZ, 1.0f },
    };
    
    tVector4 aTopRightRay[] =
    {
        { pCamera->mNearTopRight.fX, pCamera->mNearTopRight.fY, pCamera->mNearTopRight.fZ, 1.0f },
        { pCamera->mFarTopRight.fX, pCamera->mFarTopRight.fY, pCamera->mFarTopRight.fZ, 1.0f },
    };
    
    tVector4 aBottomLeftRay[] =
    {
        { pCamera->mNearBottomLeft.fX, pCamera->mNearBottomLeft.fY, pCamera->mNearBottomLeft.fZ, 1.0f },
        { pCamera->mFarBottomLeft.fX, pCamera->mFarBottomLeft.fY, pCamera->mFarBottomLeft.fZ, 1.0f },
    };
    
    tVector4 aBottomRightRay[] =
    {
        { pCamera->mNearBottomRight.fX, pCamera->mNearBottomRight.fY, pCamera->mNearBottomRight.fZ, 1.0f },
        { pCamera->mFarBottomRight.fX, pCamera->mFarBottomRight.fY, pCamera->mFarBottomRight.fZ, 1.0f },
    };
    
    int iColorShader = CShaderManager::instance()->getShader( "color" );
    
    glUseProgram( iColorShader );
    
    GLint iViewProj = glGetUniformLocation( iColorShader, "viewProjMat" );
    WTFASSERT2( iViewProj >= 0, "invalid view proj matrix" );
    
    GLint iModelColor = glGetUniformLocation( iColorShader, "modelColor" );
    
    tMatrix44 transpose, viewProjMatrix;
    Matrix44Transpose( &transpose, pViewMatrix );
    Matrix44Multiply( &viewProjMatrix, &transpose, pProjMatrix );
    
    glUniformMatrix4fv( iViewProj, 1, GL_FALSE, viewProjMatrix.afEntries );
    glUniform4f( iModelColor, 0.0f, 1.0f, 0.0f, 1.0f );
    
    GLint iPos = glGetAttribLocation( iCullShader, "position" );
    GLint iNorm = glGetAttribLocation( iCullShader, "normal" );
    
    WTFASSERT2( iPos >= 0, "invalid position shader semantic" );
    WTFASSERT2( iNorm >= 0, "invalid normal shader semantic" );
    
    //glBufferData( GL_ARRAY_BUFFER, sizeof( tInterleaveVert ) * pModelInstance->miNumVBOVerts, pModelInstance->maVBOVerts, GL_STATIC_DRAW );
    glDisable( GL_CULL_FACE );
    //glDisable( GL_DEPTH_TEST );
    
    glLineWidth( 2.0f );
    glEnableVertexAttribArray( iPos );
    
    glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aTopLeftRay );
    glDrawArrays( GL_LINES, 0, 2 );
    
    glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aTopRightRay );
    glDrawArrays( GL_LINES, 0, 2 );
    
    glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aBottomLeftRay );
    glDrawArrays( GL_LINES, 0, 2 );
    
    glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aBottomRightRay );
    glDrawArrays( GL_LINES, 0, 2 );
    
    // draw grid
    tVector4 const* aCenters = pLevel->mGrid.maNodeCenters;
    float fHalfNodeSize = pLevel->mGrid.maNodes[0].mfSize * 0.5f;
    int iNumNodesPerDimension = pLevel->mGrid.miNumNodesInDimension;

    for( int iZ = 0; iZ < iNumNodesPerDimension; iZ++ )
    {
        for( int iY = iNumNodesPerDimension / 2; iY <  iNumNodesPerDimension / 2 + 1; iY++ ) // iNumNodesPerDimension; iY++ )
        {
            for( int iX = 0; iX < iNumNodesPerDimension; iX++ )
            {
                int iIndex = iZ * iNumNodesPerDimension * iNumNodesPerDimension + iY * iNumNodesPerDimension + iX;
                
                tVector4 const* pCenter = &aCenters[iIndex];
                tVector4 topLeftBack = { pCenter->fX - fHalfNodeSize, pCenter->fY + fHalfNodeSize, pCenter->fZ + fHalfNodeSize, 1.0f };
                tVector4 bottomRightFront = { pCenter->fX + fHalfNodeSize, pCenter->fY - fHalfNodeSize, pCenter->fZ - fHalfNodeSize, 1.0f };
                
                tOctNode const* pNode = &pLevel->mGrid.maNodes[iIndex];
                std::vector<tOctNode const *>::iterator it = std::find( pLevel->maVisibleNodes.begin(),
                                                                        pLevel->maVisibleNodes.end(),
                                                                        pNode );
                
                bool bVisible = ( it != pLevel->maVisibleNodes.end() );
                if( bVisible )
                {
                    glUniform4f( iModelColor, 0.0f, 0.0f, 0.0f, 0.5f );
                }
                else
                {
                    glUniform4f( iModelColor, 1.0f, 0.0f, 0.0f, 0.5f );
                }
                
                // front
                tVector4 aRay0[] =
                {
                    { topLeftBack.fX, topLeftBack.fY, bottomRightFront.fZ, 1.0f },
                    { topLeftBack.fX, bottomRightFront.fY, bottomRightFront.fZ, 1.0f },
                };
                
                tVector4 aRay1[] =
                {
                    { topLeftBack.fX, bottomRightFront.fY, bottomRightFront.fZ, 1.0f },
                    { bottomRightFront.fX, bottomRightFront.fY, bottomRightFront.fZ, 1.0f },
                };
                
                tVector4 aRay2[] =
                {
                    { bottomRightFront.fX, bottomRightFront.fY, bottomRightFront.fZ, 1.0f },
                    { bottomRightFront.fX, topLeftBack.fY, bottomRightFront.fZ, 1.0f },
                };
                
                tVector4 aRay3[] =
                {
                    { bottomRightFront.fX, topLeftBack.fY, bottomRightFront.fZ, 1.0f },
                    { topLeftBack.fX, topLeftBack.fY, bottomRightFront.fZ, 1.0f },
                };
                
                glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aRay0 );
                glDrawArrays( GL_LINES, 0, 2 );
                
                glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aRay1 );
                glDrawArrays( GL_LINES, 0, 2 );
                
                glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aRay2 );
                glDrawArrays( GL_LINES, 0, 2 );
                
                glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aRay3 );
                glDrawArrays( GL_LINES, 0, 2 );
                
                // back
                tVector4 aRay4[] =
                {
                    { topLeftBack.fX, topLeftBack.fY, topLeftBack.fZ, 1.0f },
                    { topLeftBack.fX, bottomRightFront.fY, topLeftBack.fZ, 1.0f },
                };
                
                tVector4 aRay5[] =
                {
                    { topLeftBack.fX, bottomRightFront.fY, topLeftBack.fZ, 1.0f },
                    { bottomRightFront.fX, bottomRightFront.fY, topLeftBack.fZ, 1.0f },
                };
                
                tVector4 aRay6[] =
                {
                    { bottomRightFront.fX, bottomRightFront.fY, topLeftBack.fZ, 1.0f },
                    { bottomRightFront.fX, topLeftBack.fY, topLeftBack.fZ, 1.0f },
                };
                
                tVector4 aRay7[] =
                {
                    { bottomRightFront.fX, topLeftBack.fY, topLeftBack.fZ, 1.0f },
                    { topLeftBack.fX, topLeftBack.fY, topLeftBack.fZ, 1.0f },
                };
                
                glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aRay4 );
                glDrawArrays( GL_LINES, 0, 2 );
                
                glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aRay5 );
                glDrawArrays( GL_LINES, 0, 2 );
                
                glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aRay6 );
                glDrawArrays( GL_LINES, 0, 2 );
                
                glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aRay7 );
                glDrawArrays( GL_LINES, 0, 2 );
                
                // top
                tVector4 aRay8[] =
                {
                    { topLeftBack.fX, topLeftBack.fY, bottomRightFront.fZ, 1.0f },
                    { topLeftBack.fX, topLeftBack.fY, topLeftBack.fZ, 1.0f },
                };
                
                tVector4 aRay9[] =
                {
                    { topLeftBack.fX, topLeftBack.fY, topLeftBack.fZ, 1.0f },
                    { bottomRightFront.fX, topLeftBack.fY, topLeftBack.fZ, 1.0f },
                };
                
                tVector4 aRay10[] =
                {
                    { bottomRightFront.fX, topLeftBack.fY, topLeftBack.fZ, 1.0f },
                    { bottomRightFront.fX, topLeftBack.fY, bottomRightFront.fZ, 1.0f },
                };
                
                tVector4 aRay11[] =
                {
                    { bottomRightFront.fX, topLeftBack.fY, bottomRightFront.fZ, 1.0f },
                    { topLeftBack.fX, topLeftBack.fY, bottomRightFront.fZ, 1.0f },
                };
                
                glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aRay8 );
                glDrawArrays( GL_LINES, 0, 2 );
                
                glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aRay9 );
                glDrawArrays( GL_LINES, 0, 2 );
                
                glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aRay10 );
                glDrawArrays( GL_LINES, 0, 2 );
                
                glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aRay11 );
                glDrawArrays( GL_LINES, 0, 2 );
                
                // bottom
                tVector4 aRay12[] =
                {
                    { topLeftBack.fX, bottomRightFront.fY, bottomRightFront.fZ, 1.0f },
                    { topLeftBack.fX, bottomRightFront.fY, topLeftBack.fZ, 1.0f },
                };
                
                tVector4 aRay13[] =
                {
                    { topLeftBack.fX, bottomRightFront.fY, topLeftBack.fZ, 1.0f },
                    { bottomRightFront.fX, bottomRightFront.fY, topLeftBack.fZ, 1.0f },
                };
                
                tVector4 aRay14[] =
                {
                    { bottomRightFront.fX, bottomRightFront.fY, topLeftBack.fZ, 1.0f },
                    { bottomRightFront.fX, bottomRightFront.fY, bottomRightFront.fZ, 1.0f },
                };
                
                tVector4 aRay15[] =
                {
                    { bottomRightFront.fX, bottomRightFront.fY, bottomRightFront.fZ, 1.0f },
                    { topLeftBack.fX, bottomRightFront.fY, bottomRightFront.fZ, 1.0f },
                };
                
                glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aRay12 );
                glDrawArrays( GL_LINES, 0, 2 );
                
                glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aRay13 );
                glDrawArrays( GL_LINES, 0, 2 );
                
                glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aRay14 );
                glDrawArrays( GL_LINES, 0, 2 );
                
                glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, aRay15 );
                glDrawArrays( GL_LINES, 0, 2 );
                
            }   // for x = 0 to num nodes
            
        }   // for y = 0 to num nodes
        
    }   // for z = 0 to num nodes

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
}

/*
**
*/
static void loadModels( tModel** apModels, TiXmlNode* pNode )
{
    int iModel = 0;
    TiXmlNode* pModelNode = pNode->FirstChild( "model" );
    while( pModelNode )
    {
        const char* szFileName = pModelNode->FirstChild( "file" )->FirstChild()->Value();
        
        tModel* pModel = apModels[iModel];
        modelInit( pModel );
        modelLoad( pModel, szFileName );
        
        const char* szModelName = pModelNode->FirstChild( "name" )->FirstChild()->Value();
        strncpy( pModel->mszName, szModelName, 256 );
        
        pModelNode = pModelNode->NextSibling( "model" );
        ++iModel;
    }
}

/*
**
*/
static void loadModelNames( tModel** apModels, TiXmlNode* pNode )
{
    int iModel = 0;
    TiXmlNode* pModelNode = pNode->FirstChild( "model" );
    while( pModelNode )
    {
        tModel* pModel = apModels[iModel];
        modelInit( pModel );
        
        // model name
        const char* szModelName = pModelNode->FirstChild( "name" )->FirstChild()->Value();
        strncpy( pModel->mszName, szModelName, 256 );
        
        pModelNode = pModelNode->NextSibling( "model" );
        ++iModel;
    }
}

/*
**
*/
static void loadModelInstances( tModel** apModels, int iNumModels, tModelInstance* aModelInstances, TiXmlNode* pRoot )
{
    int iModelInstance = 0;
    
    TiXmlNode* pInstanceNode = pRoot->FirstChild( "instance" );
    while( pInstanceNode )
    {
        // not a model instance
        if( pInstanceNode->FirstChild( "model" ) == NULL )
        {
            pInstanceNode = pInstanceNode->NextSibling( "instance" );
            continue;
        }
        
        // look for the model this one is instance of
        const char* szModelName = pInstanceNode->FirstChild( "model" )->FirstChild()->Value();
        tModel* pFoundModel = NULL;
        for( int i = 0; i < iNumModels; i++ )
        {
            tModel* pModel = apModels[i];
            if( !strcmp( pModel->mszName, szModelName ) )
            {
                pFoundModel = pModel;
                break;
            }
        }   // for i = 0 to num models
        
        const char* szName = pInstanceNode->FirstChild( "name" )->FirstChild()->Value();
        
        // initialize the model instance
        WTFASSERT2( pFoundModel, "can't find model %s for instance", szModelName );
        tModelInstance* pModelInstance = &aModelInstances[iModelInstance];
        modelInstanceInit( pModelInstance );
        modelInstanceSet( pModelInstance, pFoundModel );
        modelInstanceSetupGL( pModelInstance );
        
        strncpy( pModelInstance->mszName, szName, sizeof( pModelInstance->mszName ) );
        
        TiXmlNode* pXFormNode = pInstanceNode->FirstChild( "xform" );
        
        int iNumXForms = 0;
        getNumXForms( pXFormNode, &iNumXForms );
        
        pModelInstance->maPositions = (tVector4 *)malloc( sizeof( tVector4 ) * iNumXForms );
        pModelInstance->maRotations = (tVector4 *)malloc( sizeof( tVector4 ) * iNumXForms );
        pModelInstance->maScalings = (tVector4 *)malloc( sizeof( tVector4 ) * iNumXForms );
        pModelInstance->maScalePivot = (tVector4 *)malloc( sizeof( tVector4 ) * iNumXForms );
        pModelInstance->maRotatePivot = (tVector4 *)malloc( sizeof( tVector4 ) * iNumXForms );
        pModelInstance->miNumXForms = iNumXForms;
        
        loadXForms( pModelInstance, 0, pXFormNode );
        
        modelInstanceUpdateXForm( pModelInstance );
        
        pInstanceNode = pInstanceNode->NextSibling( "instance" );
        ++iModelInstance;
    }
}

/*
**
*/
static void loadAnimModels( tAnimModel** apAnimModels,
                            tAnimHierarchy** apAnimHierarchies,
                            TiXmlNode* pNode )
{
    int iModel = 0;
    TiXmlNode* pModelNode = pNode->FirstChild( "animmodel" );
    while( pModelNode )
    {
        const char* szFileName = pModelNode->FirstChild( "file" )->FirstChild()->Value();
        
        tAnimModel* pAnimModel = apAnimModels[iModel];
        tAnimHierarchy* pAnimHierarchy = apAnimHierarchies[iModel];
        
        animHierarchyLoad( pAnimHierarchy, szFileName, &gGameJointFactory, &gGameMatrixFactory );
        
        animModelInit( pAnimModel );
        animModelLoad( pAnimModel, pAnimHierarchy, szFileName );
        
        const char* szModelName = pModelNode->FirstChild( "name" )->FirstChild()->Value();
        strncpy( pAnimModel->mszName, szModelName, 256 );
        
        pModelNode = pModelNode->NextSibling( "animmodel" );
        ++iModel;
    }
}

/*
**
*/
static void loadAnimModelInstances( tAnimModel** apAnimModels,
                                    int iNumAnimModels,
                                    tAnimModelInstance* aAnimModelInstances,
                                    tAnimHierarchy** apAnimHierarchies,
                                    TiXmlNode* pRoot )
{
    int iModelInstance = 0;
    
    TiXmlNode* pInstanceNode = pRoot->FirstChild( "instance" );
    while( pInstanceNode )
    {
        // not a model instance
        if( pInstanceNode->FirstChild( "animmodel" ) == NULL )
        {
            pInstanceNode = pInstanceNode->NextSibling( "instance" );
            continue;
        }
        
        // look for the model this one is instance of
        const char* szModelName = pInstanceNode->FirstChild( "animmodel" )->FirstChild()->Value();
        tAnimModel* pFoundModel = NULL;
        for( int i = 0; i < iNumAnimModels; i++ )
        {
            tAnimModel* pModel = apAnimModels[i];
            if( !strcmp( pModel->mszName, szModelName ) )
            {
                pFoundModel = pModel;
                break;
            }
        }   // for i = 0 to num models
        
        const char* szName = pInstanceNode->FirstChild( "name" )->FirstChild()->Value();
        
        // initialize the model instance
        WTFASSERT2( pFoundModel, "can't find model %s for instance", szModelName );
        tAnimModelInstance* pModelInstance = &aAnimModelInstances[iModelInstance];
        animModelInstanceInit( pModelInstance );
        animModelInstanceSet( pModelInstance, pFoundModel );
        animModelInstanceSetupGL( pModelInstance );
        
        // hierarchy for this instance
        tAnimHierarchy* pAnimHierarchy = apAnimHierarchies[iModelInstance];
        animHierarchyCopy( pAnimHierarchy,
                           (tAnimHierarchy const *)pFoundModel->mpAnimHierarchy,
                           &gGameJointFactory,
                           &gGameMatrixFactory );
        
        pModelInstance->mpAnimHierarchy = pAnimHierarchy;
        
        strncpy( pModelInstance->mszName, szName, sizeof( pModelInstance->mszName ) );
        
        TiXmlNode* pXFormNode = pInstanceNode->FirstChild( "xform" );
        
        // transformation
        int iNumXForms = 0;
        getNumXForms( pXFormNode, &iNumXForms );
        
        pModelInstance->maPositions = (tVector4 *)malloc( sizeof( tVector4 ) * iNumXForms );
        pModelInstance->maRotations = (tVector4 *)malloc( sizeof( tVector4 ) * iNumXForms );
        pModelInstance->maScalings = (tVector4 *)malloc( sizeof( tVector4 ) * iNumXForms );
        pModelInstance->maScalePivot = (tVector4 *)malloc( sizeof( tVector4 ) * iNumXForms );
        pModelInstance->maRotatePivot = (tVector4 *)malloc( sizeof( tVector4 ) * iNumXForms );
        pModelInstance->miNumXForms = iNumXForms;
        pModelInstance->mbPreXform = true;
        
        loadAnimModelXForms( pModelInstance, 0, pXFormNode );
        
        animModelInstanceUpdateXForm( pModelInstance );
        
        // check for animation sequence
        char szFullPath[256];
        char szAnimSequenceName[256];
        snprintf( szAnimSequenceName, sizeof( szAnimSequenceName ), "%s.anm", szModelName );
        getFullPath( szFullPath, szAnimSequenceName );
        FILE* fp = fopen( szFullPath, "rb" );
        if( fp )
        {
            fclose( fp );
            
            tAnimSequence* pAnimSequence = gGameAnimSequenceFactory.alloc( 1 );
            animSequenceInit( pAnimSequence );
            animSequenceLoad( pAnimSequence, szAnimSequenceName,
                              pModelInstance->mpAnimHierarchy,
                              &gGameVectorFactory,
                              &gGameQuaternionFactory );
            animModelInstanceSetAnimSequence( pModelInstance, pAnimSequence );
        }
        
        pInstanceNode = pInstanceNode->NextSibling( "instance" );
        ++iModelInstance;
    }
}

/*
**
*/
static void parseVector( const char* szVector, tVector4* pV )
{
    char szCopy[256];
    strncpy( szCopy, szVector, sizeof( szCopy ) );
    
    char* szX = strtok( szCopy, "," );
    char* szY = strtok( NULL, "," );
    char* szZ = strtok( NULL, "," );
    
    pV->fX = (float)atof( szX );
    pV->fY = (float)atof( szY );
    pV->fZ = (float)atof( szZ );
    pV->fW = 1.0f;
}

/*
**
*/
static void loadXForms( tModelInstance* pModelInstance, int iDepth, TiXmlNode* pNode )
{
    const char* szTranslation = pNode->FirstChild( "translation" )->FirstChild()->Value();
    const char* szScaling = pNode->FirstChild( "scaling" )->FirstChild()->Value();
    const char* szRotation = pNode->FirstChild( "rotation" )->FirstChild()->Value();
    const char* szScalePivot = pNode->FirstChild( "scalePivot" )->FirstChild()->Value();
    const char* szRotatePivot = pNode->FirstChild( "rotatePivot" )->FirstChild()->Value();
    
    parseVector( szTranslation, &pModelInstance->maPositions[iDepth] );
    parseVector( szScaling, &pModelInstance->maScalings[iDepth] );
    parseVector( szRotation, &pModelInstance->maRotations[iDepth] );
    parseVector( szScalePivot, &pModelInstance->maScalePivot[iDepth] );
    parseVector( szRotatePivot, &pModelInstance->maRotatePivot[iDepth] );
    
    const float fDegreeToRadian = 3.14159f / 180.0f;
    pModelInstance->maRotations[iDepth].fX *= fDegreeToRadian;
    pModelInstance->maRotations[iDepth].fY *= fDegreeToRadian;
    pModelInstance->maRotations[iDepth].fZ *= fDegreeToRadian;
    pModelInstance->maRotations[iDepth].fW = 1.0f;
    
    TiXmlNode* pChildNode = pNode->FirstChild( "xform" );
    if( pChildNode )
    {
        loadXForms( pModelInstance, iDepth + 1, pChildNode );
    }
}

/*
**
*/
static void loadAnimModelXForms( tAnimModelInstance* pModelInstance, int iDepth, TiXmlNode* pNode )
{
    const char* szTranslation = pNode->FirstChild( "translation" )->FirstChild()->Value();
    const char* szScaling = pNode->FirstChild( "scaling" )->FirstChild()->Value();
    const char* szRotation = pNode->FirstChild( "rotation" )->FirstChild()->Value();
    const char* szScalePivot = pNode->FirstChild( "scalePivot" )->FirstChild()->Value();
    const char* szRotatePivot = pNode->FirstChild( "rotatePivot" )->FirstChild()->Value();
    
    parseVector( szTranslation, &pModelInstance->maPositions[iDepth] );
    parseVector( szScaling, &pModelInstance->maScalings[iDepth] );
    parseVector( szRotation, &pModelInstance->maRotations[iDepth] );
    parseVector( szScalePivot, &pModelInstance->maScalePivot[iDepth] );
    parseVector( szRotatePivot, &pModelInstance->maRotatePivot[iDepth] );
    
    const float fDegreeToRadian = 3.14159f / 180.0f;
    pModelInstance->maRotations[iDepth].fX *= fDegreeToRadian;
    pModelInstance->maRotations[iDepth].fY *= fDegreeToRadian;
    pModelInstance->maRotations[iDepth].fZ *= fDegreeToRadian;
    pModelInstance->maRotations[iDepth].fW = 1.0f;
    
    TiXmlNode* pChildNode = pNode->FirstChild( "xform" );
    if( pChildNode )
    {
        loadAnimModelXForms( pModelInstance, iDepth + 1, pChildNode );
    }
}

/*
**
*/
static void getNumXForms( TiXmlNode* pNode, int* piNumDepths )
{
    ++(*piNumDepths);
    
    TiXmlNode* pChild = pNode->FirstChild( "xform" );
    if( pChild )
    {
        getNumXForms( pChild, piNumDepths );
    }
}

/*
**
*/
static bool isModelInOctNode( tOctNode const* pOctNode, void* pObject)
{
    bool bRet = false;
    
    tModelInstance* pModelInstance = (tModelInstance *)pObject;
    modelInstanceUpdateXForm( pModelInstance );
    
    tVector4 const* pPos = pOctNode->mpCenter;
    float fSize = pOctNode->mfSize;
    
    // extent of the octnode
    float fX0 = pPos->fX - fSize * 0.5f;
    float fX1 = pPos->fX + fSize * 0.5f;
    
    float fY0 = pPos->fY - fSize * 0.5f;
    float fY1 = pPos->fY + fSize * 0.5f;
    
    float fZ0 = pPos->fZ - fSize * 0.5f;
    float fZ1 = pPos->fZ + fSize * 0.5f;
    
    // check if any vertex are in the node
    tVector4 const* aXFormPos = pModelInstance->maXFormPos;
    for( int i = 0; i < pModelInstance->mpModel->miNumVerts; i++ )
    {
        // transform to world position
        tVector4 xformPos;
        Matrix44Transform( &xformPos, aXFormPos, &pModelInstance->mXFormMat );
        
        if( xformPos.fX >= fX0 && xformPos.fX <= fX1 &&
            xformPos.fY >= fY0 && xformPos.fY <= fY1 &&
            xformPos.fZ >= fZ0 && xformPos.fZ <= fZ1 )
        {
            bRet = true;
            break;
        }
        
        ++aXFormPos;
        
    }   // for i = 0 to num verts

    int iNumFaces = pModelInstance->mpModel->miNumFaces;
    tFace const* aFaces = pModelInstance->mpModel->maFaces;
    aXFormPos = pModelInstance->maXFormPos;
    
    // check edges on the faces
    if( !bRet )
    {
        for( int i = 0; i < iNumFaces; i++ )
        {
            tVector4 const* pPt0 = &aXFormPos[aFaces->maiV[0]];
            tVector4 const* pPt1 = &aXFormPos[aFaces->maiV[1]];
            tVector4 const* pPt2 = &aXFormPos[aFaces->maiV[2]];
            
            // transform to world position
            tVector4 xformPos0, xformPos1, xformPos2;
            Matrix44Transform( &xformPos0, pPt0, &pModelInstance->mXFormMat );
            Matrix44Transform( &xformPos1, pPt1, &pModelInstance->mXFormMat );
            Matrix44Transform( &xformPos2, pPt2, &pModelInstance->mXFormMat );
            
            bRet = nodeIntersect( pOctNode, &xformPos0, &xformPos1, &xformPos2 );
            if( bRet )
            {
                break;
            }
            
            ++aFaces;
        }
    }
    
    return bRet;
}

/*
**
*/
static bool isAnimModelInOctNode( tOctNode const* pOctNode, void* pObject )
{
    bool bRet = false;
    
    tAnimModelInstance* pAnimModelInstance = (tAnimModelInstance *)pObject;
    animModelInstanceUpdateXForm( pAnimModelInstance );
    
    tVector4 const* pPos = pOctNode->mpCenter;
    float fSize = pOctNode->mfSize;
    
    // extent of the octnode
    float fX0 = pPos->fX - fSize * 0.5f;
    float fX1 = pPos->fX + fSize * 0.5f;
    
    float fY0 = pPos->fY - fSize * 0.5f;
    float fY1 = pPos->fY + fSize * 0.5f;
    
    float fZ0 = pPos->fZ - fSize * 0.5f;
    float fZ1 = pPos->fZ + fSize * 0.5f;
    
    // check if any vertex are in the node
    tVector4 const* aXFormPos = pAnimModelInstance->maXFormPos;
    for( int i = 0; i < pAnimModelInstance->mpAnimModel->miNumVerts; i++ )
    {
        if( aXFormPos->fX >= fX0 && aXFormPos->fX <= fX1 &&
            aXFormPos->fY >= fY0 && aXFormPos->fY <= fY1 &&
            aXFormPos->fZ >= fZ0 && aXFormPos->fZ <= fZ1 )
        {
            bRet = true;
            break;
        }
        
        ++aXFormPos;
        
    }   // for i = 0 to num verts
    
    int iNumFaces = pAnimModelInstance->mpAnimModel->miNumFaces;
    tFace const* aFaces = pAnimModelInstance->mpAnimModel->maFaces;
    aXFormPos = pAnimModelInstance->maXFormPos;
    
    // check edges on the faces
    if( !bRet )
    {
        for( int i = 0; i < iNumFaces; i++ )
        {
            tVector4 const* pPt0 = &aXFormPos[aFaces->maiV[0]];
            tVector4 const* pPt1 = &aXFormPos[aFaces->maiV[1]];
            tVector4 const* pPt2 = &aXFormPos[aFaces->maiV[2]];
            
            bRet = nodeIntersect( pOctNode, pPt0, pPt1, pPt2 );
            if( bRet )
            {
                break;
            }
            
            ++aFaces;
        }
    }
    
    return bRet;
}

/*
**
*/
bool nodeIntersect( tOctNode const* pOctNode, tVector4 const* pPt0, tVector4 const* pPt1, tVector4 const* pPt2 )
{
    tVector4 const* pPos = pOctNode->mpCenter;
    float fSize = pOctNode->mfSize;
    
    float fX0 = pPos->fX - fSize * 0.5f;
	float fX1 = pPos->fX + fSize * 0.5f;
    
	float fY0 = pPos->fY - fSize * 0.5f;
	float fY1 = pPos->fY + fSize * 0.5f;
	
	float fZ0 = pPos->fZ - fSize * 0.5f;
	float fZ1 = pPos->fZ + fSize * 0.5f;
    
	if( pPt0->fX >= fX0 && pPt0->fX <= fX1 &&
       pPt0->fY >= fY0 && pPt0->fY <= fY1 &&
       pPt0->fZ >= fZ0 && pPt0->fZ <= fZ1 )
	{
		return true;
	}
    
	if( pPt1->fX >= fX0 && pPt1->fX <= fX1 &&
       pPt1->fY >= fY0 && pPt1->fY <= fY1 &&
       pPt1->fZ >= fZ0 && pPt1->fZ <= fZ1 )
	{
		return true;
	}
    
	// check all 6 sides
	tVector4 aNormals[] =
	{
		{ 0.0f, 0.0f, -1.0f, 1.0f },		// front
		{ 0.0f, 0.0f, 1.0f, 1.0f },		// back
		{ -1.0f, 0.0f, 0.0f, 1.0f },		// left
		{ 1.0f, 0.0f, 0.0f, 1.0f },		// right
		{ 0.0f, 1.0f, 0.0f, 1.0f },		// top
		{ 0.0f, -1.0f, 0.0f, 1.0f },		// bottom
	};
	
	for( int i = 0; i < 6; i++ )
	{
		tVector4* pNormal = &aNormals[i];
		
		// calculate D on the normal plane
		float fPlaneD = 0.0f;
		tVector4 pt;
        
		if( i == 0 )
		{
			// front
			pt.fX = fX0;
			pt.fY = fY0;
			pt.fZ = fZ0;
		}
		else if( i == 1 )
		{
			// back
			pt.fX = fX0;
			pt.fY = fY0;
			pt.fZ = fZ1;
		}
		else if( i == 2 )
		{
			// left
			pt.fX = fX0;
			pt.fY = fY0;
			pt.fZ = fZ0;
		}
		else if( i == 3 )
		{
			// right
			pt.fX = fX1;
			pt.fY = fY0;
			pt.fZ = fZ0;
		}
		else if( i == 4 )
		{
			// top
			pt.fX = fX0;
			pt.fY = fY1;
			pt.fZ = fZ0;
		}
		else if( i == 5 )
		{
			// bottom
			pt.fX = fX0;
			pt.fY = fY0;
			pt.fZ = fZ0;
		}
		
		// D = -( Ax + By + Cz )
		fPlaneD = -( pt.fX * pNormal->fX + pt.fY * pNormal->fY + pt.fZ * pNormal->fZ );
        
		float fDP0 = Vector4Dot( pNormal, pPt0 );
		tVector4 diff;
		Vector4Subtract( &diff, pPt0, pPt1 );
		float fNumerator = fDP0 + fPlaneD;
		float fDenominator = Vector4Dot( pNormal, &diff );
		if( fDenominator == 0.0f )
		{
			continue;
		}
        
		float fT = fNumerator / fDenominator;
        
        
		if( fT >= 0.0f && fT <= 1.0f )
		{
			tVector4 intersect;
			intersect.fX = pPt0->fX + fT * ( pPt1->fX - pPt0->fX );
			intersect.fY = pPt0->fY + fT * ( pPt1->fY - pPt0->fY );
			intersect.fZ = pPt0->fZ + fT * ( pPt1->fZ - pPt0->fZ );
			intersect.fW = 1.0f;
			
			if( i == 0 )
			{
				// front
				WTFASSERT2( fabs( intersect.fZ - fZ0 ) < 0.001f, "WTF intersect", NULL );
				if( intersect.fX >= fX0 && intersect.fX <= fX1 &&
                   intersect.fY >= fY0 && intersect.fY <= fY1 )
				{
					return true;
				}
			}
			else if( i == 1 )
			{
				// back
				WTFASSERT2( fabs( intersect.fZ - fZ1 ) < 0.001f, "WTF intersect", NULL );
				if( intersect.fX >= fX0 && intersect.fX <= fX1 &&
                   intersect.fY >= fY0 && intersect.fY <= fY1 )
				{
					return true;
				}
			}
			else if( i == 2 )
			{
				// left
				WTFASSERT2( fabs( intersect.fX - fX0 ) < 0.001f, "WTF intersect", NULL );
				if( intersect.fY >= fY0 && intersect.fY <= fY1 &&
                   intersect.fZ >= fZ0 && intersect.fZ <= fZ1 )
				{
					return true;
				}
			}
			else if( i == 3 )
			{
				// left
				WTFASSERT2( fabs( intersect.fX - fX1 ) < 0.001f, "WTF intersect", NULL );
                
				if( intersect.fY >= fY0 && intersect.fY <= fY1 &&
                   intersect.fZ >= fZ0 && intersect.fZ <= fZ1 )
				{
					return true;
				}
			}
			else if( i == 4 )
			{
				// top
				WTFASSERT2( fabs( intersect.fY - fY1 ) < 0.001f, "WTF intersect", NULL );
                
				if( intersect.fX >= fX0 && intersect.fX <= fX1 &&
                   intersect.fZ >= fZ0 && intersect.fZ <= fZ1 )
				{
					return true;
				}
			}
			else if( i == 5 )
			{
				// bottom
				WTFASSERT2( fabs( intersect.fY - fY0 ) < 0.001f, "WTF intersect", NULL );
                
				if( intersect.fX >= fX0 && intersect.fX <= fX1 &&
                   intersect.fZ >= fZ0 && intersect.fZ <= fZ1 )
				{
					return true;
				}
			}
            
			//if( intersect.fX >= fX0 && intersect.fX <= fX1 &&
			//	intersect.fY >= fY0 && intersect.fY <= fY1 &&
			//	intersect.fZ >= fZ0 && intersect.fZ <= fZ1 )
			//{
			//	return true;
			//}
		}
        
	}	// for i = 0 to 6 planes
    
	return false;
}
