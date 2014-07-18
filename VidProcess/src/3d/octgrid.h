//
//  grid.h
//  animtest
//
//  Created by Dingwings on 9/26/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#ifndef __OCTGRID_H__
#define __OCTGRID_H__

#include "vector.h"
#include "../util/factory.h"
#include "camera.h"

#include <vector>

#define NUM_VISIBLE_NODES_PER_ALLOC 1000
#define NUM_VISIBLE_MODELS_PER_ALLOC 1000

struct OctNode
{
    tVector4*           mpCenter;
    float               mfSize;
    
    void**               mapObjects;
    int                 miNumObjects;
    int                 miNumObjectAlloc;

	bool				mbFlag;
};

typedef struct OctNode tOctNode;

struct OctGrid
{
    tOctNode*           maNodes;
    int                 miNumNodes;
    int                 miNumNodesInDimension;
    
    tVector4            mDimension;
    tVector4*           maNodeCenters;
    
	tVector4			mCenter;
    
    CFactory<tVector4>*    mpOctNodeCenterFactory;
};

typedef struct OctGrid tOctGrid;

struct VisibleOctNodes
{
	tOctNode const**	mapNodes;
	int					miNumNodes;
	int					miNumAlloc;
};

typedef struct VisibleOctNodes tVisibleOctNodes;

struct VisibleModels
{
	void**				mapModels;
	int					miNumModels;
	int					miNumAlloc;
};

typedef struct VisibleModels tVisibleModels;

void octGridInit( tOctGrid* pOctGrid,
                 tVector4 const* pCenter,
                 float fTotalSize,
                 float fNodeSize );
void octGridRelease( tOctGrid* pOctGrid );

void octGridAddObject( tOctGrid* pOctGrid,
                      void* pObject,
                      bool (*pfnInOctNode)(tOctNode const*, void*) );

void octGridGetVisibleNodes( tOctGrid const* pOctGrid,
                             CCamera const* pCamera,
                             tVisibleOctNodes* pVisibleOctNodes,
                             tVector4 const* pOffset );

void octGridGetVisibleNodes2( tOctGrid const* pOctGrid,
                             CCamera const* pCamera,
                             tVisibleOctNodes* pVisibleNodes,
							 tVector4 const* pOffset,
							 float fScreenRatio,
							 bool bOrthographic,
							 float fViewWidth,
							 float fViewHeight );

void octGridSave( tOctGrid const* pOctGrid,
                 const char* szFileName );

void octGridLoad( tOctGrid* pOctGrid,
                 const char* szFileName,
                 void* (*allocObject)( const char* szName, void* pUserData0, void* pUserData1, void* pUserData2, void* pUserData3 ),
                 void* pUserData0,
				 void* pUserData1,
				 void* pUserData2,
				 void* pUserData3 );

void octGridSaveBinary( tOctGrid const* pOctGrid, 
					    const char* szFileName );

void octGridLoadBinary( tOctGrid* pOctGrid,
					    const char* szFileName,
						void* (*allocObject)( const char* szName, void* pUserData0, void* pUserData1, void* pUserData2, void* pUserData3 ),
						void* pUserData0,
						void* pUserData1,
						void* pUserData2,
						void* pUserData3 );

void octGridAddObjectToNode( tOctGrid* pGrid, int iNodeIndex, void* pObject );

#endif // __GRID_H__
