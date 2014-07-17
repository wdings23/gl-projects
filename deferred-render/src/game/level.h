//
//  level.h
//  animtest
//
//  Created by Dingwings on 8/9/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#ifndef __LEVEL_H__
#define __LEVEL_H__

#include "model.h"
#include "modelinstance.h"
#include "camera.h"

#include "animmodelinstance.h"
#include "animhierarchy.h"

#include "octgrid.h"

enum
{
    LEVEL_LOADSTATE_NOT_LOADED = 0,
    LEVEL_LOADSTATE_LOADING,
    LEVEL_LOADSTATE_LOADED,
    
    NUM_LEVEL_LOADSTATES
};

struct Level
{
    tModel**                mapModels;
    int                     miNumModels;
    
    tModelInstance*         maModelInstances;
    int                     miNumModelInstances;
    
    tModelInstance**        mapVisible;
    int                     miNumVisibleAlloc;
    
    int                     miNumVisible;

    tAnimModel**            mapAnimModels;
    tAnimHierarchy**        mapAnimHierarchies;
    int                     miNumAnimModels;
    
    tAnimModelInstance*     maAnimModelInstances;
    tAnimHierarchy**        mapAnimModelInstanceHierarchies;
    int                     miNumAnimModelInstances;
    
    tAnimModelInstance**    mapVisibleAnimModels;
    int                     miNumVisibleAnimModelAlloc;
    int                     miNumVisibleAnimModels;
    
    tOctGrid                mGrid;
    
    std::vector<void *>     maVisibleModels;
    std::vector<tOctNode const*> maVisibleNodes;
    
    tVector4                mCenter;
    tVector4                mDimension;
    
    // for copy of the same level
    tVector4                mOffset;
    
    int                     miLoadState;
};

typedef struct Level tLevel;

void levelInit( tLevel* pLevel );
void levelUpdate( tLevel* pLevel, 
				  CCamera const* pCamera,
				  float fScreenRatio,
				  tVisibleOctNodes* pVisibleNodes );

void levelDraw( tLevel* pLevel,
                tMatrix44 const* pViewMatrix,
                tMatrix44 const* pProjectionMatrix,
                int iShader );
void levelLoad( tLevel* pLevel, const char* szFileName );

void levelOnDemandLoad( tLevel* pLevel,
                        const char* szFileName,
                        void* (*allocObject)( const char* szName, void* pUserData0, void* pUserData1, void* pUserData2, void* pUserData3 ) );

void levelDrawCullDebug( tLevel* pLevel,
                         tMatrix44 const* pViewMatrix,
                         tMatrix44 const* pProjMatrix );

void levelLoadAttributes( tLevel* pLevel, const char* szFileName );

void levelCopy( tLevel* pLevel, tLevel const* pOrig, tVector4 const* pCenter );

inline void levelOffset( tLevel* pLevel, tVector4 const* pOffset ) { memcpy( &pLevel->mOffset, pOffset, sizeof( tVector4 ) ); }

void levelDrawFaceColor( tLevel* pLevel,
						 tMatrix44 const* pViewMatrix,
						 tMatrix44 const* pProjectionMatrix,
						 int iShader );

void levelDrawDepth( tLevel* pLevel,
					 tMatrix44 const* pViewMatrix,
					 tMatrix44 const* pProjectionMatrix,
					 int iShader );

#endif // __LEVEL_H__
