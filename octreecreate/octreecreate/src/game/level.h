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
};

typedef struct Level tLevel;

void levelInit( tLevel* pLevel );
void levelUpdate( tLevel* pLevel, CCamera const* pCamera );
void levelDraw( tLevel* pLevel,
                tMatrix44 const* pViewMatrix,
                tMatrix44 const* pProjectionMatrix,
                int iShader );
void levelLoad( tLevel* pLevel, const char* szFileName );

void levelDrawCullDebug( tLevel* pLevel,
                        tMatrix44 const* pViewMatrix,
                        tMatrix44 const* pProjMatrix );

#endif // __LEVEL_H__
