//
//  octgrid.cpp
//  animtest
//
//  Created by Dingwings on 9/26/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "octgrid.h"
#include "camera.h"
#include "../util/fileutil.h"

#include "modelinstance.h"

#include <assert.h>
#include <math.h>

#define NUM_OBJECTS_PER_ALLOC 10

/*
**
*/
void octGridInit( tOctGrid* pOctGrid, 
				  tVector4 const* pGridCenter, 
				  float fTotalSize, 
				  float fNodeSize )
{
	memcpy( &pOctGrid->mCenter, pGridCenter, sizeof( tVector4 ) );

    pOctGrid->miNumNodesInDimension = (int)ceil( fTotalSize / fNodeSize );
    int iNumNodes = pOctGrid->miNumNodesInDimension;
    
    pOctGrid->miNumNodes = iNumNodes * iNumNodes * iNumNodes;
    pOctGrid->maNodes = (tOctNode *)malloc( sizeof( tOctNode ) * pOctGrid->miNumNodes );
    
    pOctGrid->mpOctNodeCenterFactory = new CFactory<tVector4>( iNumNodes * iNumNodes * iNumNodes );
    
    // dimension
    pOctGrid->mDimension.fX = fTotalSize;
    pOctGrid->mDimension.fY = fTotalSize;
    pOctGrid->mDimension.fZ = fTotalSize;
    pOctGrid->mDimension.fW = 1.0f;
    
    float fHalfNumNodes = (float)( iNumNodes >> 1 );
    
    // nodes
    pOctGrid->maNodeCenters = pOctGrid->mpOctNodeCenterFactory->alloc( pOctGrid->miNumNodes );
    tVector4* aCenters = pOctGrid->maNodeCenters;
    
    for( int iZ = 0; iZ < iNumNodes; iZ ++ )
    {
        for( int iY = 0; iY < iNumNodes; iY++ )
        {
            for( int iX = 0; iX < iNumNodes; iX++ )
            {
                int iIndex = iZ * iNumNodes * iNumNodes + iY * iNumNodes + iX;
                assert( iIndex < pOctGrid->miNumNodes );
                
                // center and size
                tVector4* pCenter = &aCenters[iIndex];
                tOctNode* pNode = &pOctGrid->maNodes[iIndex];
                pNode->mpCenter = pCenter;
                
                pCenter->fX = ( -fHalfNumNodes + (float)iX ) * fNodeSize + fNodeSize * 0.5f + pGridCenter->fX;
                pCenter->fY = ( -fHalfNumNodes + (float)iY ) * fNodeSize + fNodeSize * 0.5f + pGridCenter->fY;
                pCenter->fZ = ( -fHalfNumNodes + (float)iZ ) * fNodeSize + fNodeSize * 0.5f + pGridCenter->fZ;
                pCenter->fW = 1.0f;
                
                pNode->mfSize = fNodeSize;
                
                // list of objects in the node
                pNode->miNumObjectAlloc = 0; //NUM_OBJECTS_PER_ALLOC;
                pNode->miNumObjects = 0;
				pNode->mapObjects = NULL;
				//pNode->mapObjects = (void **)malloc( sizeof( void* ) * pNode->miNumObjectAlloc );

                //assert( pNode->mapObjects );

            }   // for x = 0 to num nodes
        
        }   // for y = 0 to num nodes
    
    }   // for z = 0 to num nodes
    
    
}

/*
**
*/
void octGridRelease( tOctGrid* pOctGrid )
{
    for( int i = 0; i < pOctGrid->miNumNodes; i++ )
    {
        free( pOctGrid->maNodes[i].mapObjects );
    }
    
    free( pOctGrid->maNodes );
    delete pOctGrid->mpOctNodeCenterFactory;
    
}

/*
**
*/
void octGridAddObject( tOctGrid* pOctGrid,
                       void* pObject,
                       bool (*pfnInOctNode)(tOctNode const*, void*) )
{
    for( int i = 0; i < pOctGrid->miNumNodes; i++ )
    {
        tOctNode* pNode = &pOctGrid->maNodes[i];
        bool bInNode = pfnInOctNode( pNode, pObject );
        
        if( bInNode )
        {
            if( pNode->miNumObjects + 1 >= pNode->miNumObjectAlloc )
            {
                pNode->miNumObjectAlloc += NUM_OBJECTS_PER_ALLOC;
                pNode->mapObjects = (void **)realloc( pNode->mapObjects, sizeof( void* ) * pNode->miNumObjectAlloc );
            }
                    
			tModelInstance* pModelInstance = (tModelInstance *)pObject;
			printf( "added model ( %.2f, %.2f, %.2f ) radius = %f\nto octnode ( %.2f, %.2f, %.2f )\n", 
				pModelInstance->mXFormMat.M( 0, 3 ),
				pModelInstance->mXFormMat.M( 1, 3 ),
				pModelInstance->mXFormMat.M( 2, 3 ),
				pModelInstance->mfRadius,
				pNode->mpCenter->fX,
				pNode->mpCenter->fY,
				pNode->mpCenter->fZ );

            pNode->mapObjects[pNode->miNumObjects] = pObject;
            ++pNode->miNumObjects;
        }
    }
}

/*
**
*/
static void getExtent( tVector4* pTopLeftNear, tVector4* pBottomRightFar, tVector4 const* pPt )
{
    if( pPt->fX < pTopLeftNear->fX )
    {
        pTopLeftNear->fX = pPt->fX;
    }
    
    if( pPt->fY > pTopLeftNear->fY )
    {
        pTopLeftNear->fY = pPt->fY;
    }
    
    if( pPt->fZ < pTopLeftNear->fZ )
    {
        pTopLeftNear->fZ = pPt->fZ;
    }
    
    if( pPt->fX > pBottomRightFar->fX )
    {
        pBottomRightFar->fX = pPt->fX;
    }
    
    if( pPt->fY < pBottomRightFar->fY )
    {
        pBottomRightFar->fY = pPt->fY;
    }
    
    if( pPt->fZ > pBottomRightFar->fZ )
    {
        pBottomRightFar->fZ = pPt->fZ;
    }
}

/*
**
*/
void octGridGetVisibleNodes( tOctGrid const* pOctGrid,
                             CCamera const* pCamera,
                             std::vector<tOctNode const*>& aVisibleNodes )
{
    // determine the extent of the frustum
    tVector4 topLeftNear = { 9999.0f, -9999.0f, 9999.0f };
    tVector4 bottomRightFar = { -9999.0f, 9999.0f, -9999.0f };
    
    getExtent( &topLeftNear, &bottomRightFar, &pCamera->mFarBottomLeft );
    getExtent( &topLeftNear, &bottomRightFar, &pCamera->mFarBottomRight );
    getExtent( &topLeftNear, &bottomRightFar, &pCamera->mFarTopLeft );
    getExtent( &topLeftNear, &bottomRightFar, &pCamera->mFarTopRight );
    
    getExtent( &topLeftNear, &bottomRightFar, &pCamera->mNearBottomLeft );
    getExtent( &topLeftNear, &bottomRightFar, &pCamera->mNearBottomRight );
    getExtent( &topLeftNear, &bottomRightFar, &pCamera->mNearTopLeft );
    getExtent( &topLeftNear, &bottomRightFar, &pCamera->mNearTopRight );
    
    tVector4 nodeSize =
    {
        pOctGrid->mDimension.fX / (float)pOctGrid->miNumNodesInDimension,
        pOctGrid->mDimension.fY / (float)pOctGrid->miNumNodesInDimension,
        pOctGrid->mDimension.fZ / (float)pOctGrid->miNumNodesInDimension,
        1.0
    };
    
    tVector4 gridNearTopLeft =
    {
        -nodeSize.fX * (float)pOctGrid->miNumNodesInDimension * 0.5f,
        nodeSize.fY * (float)pOctGrid->miNumNodesInDimension * 0.5f,
        -nodeSize.fZ * (float)pOctGrid->miNumNodesInDimension * 0.5f,
        1.0f
    };
    
    tVector4 gridFarBottomRight =
    {
        nodeSize.fX * (float)pOctGrid->miNumNodesInDimension * 0.5f,
        -nodeSize.fY * (float)pOctGrid->miNumNodesInDimension * 0.5f,
        nodeSize.fZ * (float)pOctGrid->miNumNodesInDimension * 0.5f,
        1.0f
    };
    
    // get the octnode indices in the grid
    float fLeft = topLeftNear.fX / nodeSize.fX;
    float fRight = bottomRightFar.fX / nodeSize.fX;
    
    float fTop = topLeftNear.fY / nodeSize.fY;
    float fBottom = bottomRightFar.fY / nodeSize.fY;
    
    float fNear = topLeftNear.fZ / nodeSize.fZ;
    float fFar = bottomRightFar.fZ / nodeSize.fZ;
    
    // X
    int iLeft = (int)ceilf( fLeft );
    int iRight = (int)ceilf( fRight );
    
    // Y
    int iTop = (int)ceilf( fTop );
    int iBottom = (int)ceilf( fBottom );
    
    // Z
    int iNear = (int)ceilf( fNear );
    int iFar = (int)ceilf( fFar );
    
    // floor for sign correctness
    if( fLeft < 0.0 )
    {
        iLeft = (int)floorf( fLeft );
    }
    
    if( fRight < 0.0 )
    {
        iRight = (int)floorf( fRight );
    }
    
    if( fTop < 0.0 )
    {
        iTop = (int)floorf( fTop );
    }
    
    if( fBottom < 0.0 )
    {
        iBottom = (int)floorf( fBottom );
    }
    
    if( fNear < 0.0 )
    {
        iNear = (int)floorf( fNear );
    }
    
    if( fFar < 0.0 )
    {
        iFar = (int)floorf( fFar );
    }
    
    // grid index extent
    int iGridLeft = (int)floorf( gridNearTopLeft.fX / nodeSize.fX );
    int iGridRight = (int)ceilf( gridFarBottomRight.fX / nodeSize.fX );
    
    int iGridTop = (int)ceilf( gridNearTopLeft.fY / nodeSize.fY );
    int iGridBottom = (int)ceilf( gridFarBottomRight.fY / nodeSize.fY );
    
    int iGridNear = (int)floorf( gridNearTopLeft.fZ / nodeSize.fZ );
    int iGridFar = (int)ceilf( gridFarBottomRight.fZ / nodeSize.fZ );
    
    // clamp
    if( iLeft < iGridLeft )
    {
        iLeft = iGridLeft;
    }
    
    if( iRight >= iGridRight )
    {
        iRight = iGridRight - 1;
    }
    
    if( iBottom < iGridBottom )
    {
        iBottom = iGridBottom;
    }
    
    if( iTop >= iGridTop )
    {
        iTop = iGridTop - 1;
    }
    
    if( iNear < iGridNear )
    {
        iNear = iGridNear;
    }
    
    if( iFar >= iGridFar )
    {
        iFar = iGridFar - 1;
    }
    
    int iNumNodes = pOctGrid->miNumNodesInDimension;
    int iHalfNumNodes = iNumNodes >> 1;
    
    // shift to (0, num nodes)
    iLeft += iHalfNumNodes;
    iRight += iHalfNumNodes;
    iTop += iHalfNumNodes;
    iBottom += iHalfNumNodes;
    iNear += iHalfNumNodes;
    iFar += iHalfNumNodes;
    
    // check for nodes in boundary
    tOctNode const* aNodes = pOctGrid->maNodes;
    for( int iZ = iNear; iZ <= iFar; iZ++ )
    {
        for( int iY = iBottom; iY <= iTop; iY++ )
        {
            for( int iX = iLeft; iX <= iRight; iX++ )
            {
                int iIndex = iZ * iNumNodes * iNumNodes + iY * iNumNodes + iX;
                tOctNode const* pOctNode = &aNodes[iIndex];
                bool bInFrustum = pCamera->cubeInFrustum( pOctNode->mpCenter, pOctNode->mfSize );
                if( bInFrustum )
                {
                    aVisibleNodes.push_back( pOctNode );
                    //OUTPUT( "%d OCTNODE IN FRUSTUM ( %f, %f, %f )\n", (int)aVisibleNodes.size(), pOctNode->mpCenter->fX, pOctNode->mpCenter->fY, pOctNode->mpCenter->fZ );
                }
                
            }   // for x = left to right
            
        }   // for y = bottom to top
    
    }   // for z = near to far
    
#if 0
    aVisibleNodes.clear();
    
    // camera direction
    tVector4 const* pCamPos = pCamera->getPosition();
    tVector4 const* pLookAt = pCamera->getLookAt();
    tVector4 dir;
    Vector4Subtract( &dir, pLookAt, pCamPos );
    Vector4Normalize( &dir, &dir );
    
    tVector4 const* aCenters = pOctGrid->maNodeCenters;
    float fNodeSize = pOctGrid->maNodes[0].mfSize;
    
    // brute force for now
    for( int iZ = 0; iZ < iNumNodes; iZ++ )
    {
        for( int iY = 0; iY < iNumNodes; iY++ )
        {
            for( int iX = 0; iX < iNumNodes; iX++ )
            {
                bool bInFrustum = pCamera->cubeInFrustum( aCenters, fNodeSize );
                ++aCenters;
                
                if( bInFrustum )
                {
                    int iIndex = iZ * iNumNodes * iNumNodes + iY * iNumNodes + iX;
                    WTFASSERT2( iIndex < pOctGrid->miNumNodes, "nodes out of bounds while getting visible" );
                    aVisibleNodes.push_back( &pOctGrid->maNodes[iIndex] );
                }
                
            }   // for x = 0 to num nodes
        
        }   // for y = 0 to num nodes
        
    }   // for z = 0 to num nodes
#endif // #if 0
    
}

#include "modelinstance.h"
#include "../tinyxml/tinyxml.h"
#include "parseutil.h"

/*
**
*/
void octGridSave( tOctGrid const* pOctGrid,
                  const char* szFileName )
{
    char szFullPath[256];
    getWritePath( szFullPath, szFileName );
    
    FILE* fp = fopen( szFullPath, "wb" );
    assert( fp );
    fprintf( fp, "<octgrid>\n" );

    fprintf( fp, "\t<dimensions>%f,%f,%f</dimensions>\n", pOctGrid->mDimension.fX, pOctGrid->mDimension.fY, pOctGrid->mDimension.fZ );
    fprintf( fp, "\t<node_size>%f</node_size>\n", pOctGrid->maNodes[0].mfSize );
    fprintf( fp, "\t<num_nodes_in_dimension>%d</num_nodes_in_dimension>\n", pOctGrid->miNumNodesInDimension );
	fprintf( fp, "\t<center>%f,%f,%f</center>\n", pOctGrid->mCenter.fX, pOctGrid->mCenter.fY, pOctGrid->mCenter.fZ );

    int iNumDimensionNode = pOctGrid->miNumNodesInDimension;
   
    for( int iZ = 0; iZ < iNumDimensionNode; iZ++ )
    {
        for( int iY = 0; iY < iNumDimensionNode; iY++ )
        {
            for( int iX = 0; iX < iNumDimensionNode; iX++ )
            {
                int iIndex = iZ * iNumDimensionNode * iNumDimensionNode + iY * iNumDimensionNode + iX;
                tOctNode const* pNode = &pOctGrid->maNodes[iIndex];
                
				if( pNode->miNumObjects > 0 )
				{
					tVector4 const* pCenter = pNode->mpCenter;
					fprintf( fp, "\t<node>\n" );
					fprintf( fp, "\t\t<center>%f,%f,%f</center>\n", pCenter->fX, pCenter->fY, pCenter->fZ );
					fprintf( fp, "\t\t<num_obj>%d</num_obj>\n", pNode->miNumObjects );
	                fprintf( fp, "\t\t<index>%d</index>\n", iIndex );

					for( int i = 0; i < pNode->miNumObjects; i++ )
					{
						tModelInstance* pModelInstance = (tModelInstance *)pNode->mapObjects[i];
						//fprintf( fp, "\t\t<object>%s</object>\n", pModelInstance->mszName );
	                    
						fprintf( fp, "\t\t<object>\n" );
						fprintf( fp, "\t\t\t<name>%s</name>\n", pModelInstance->mszName );
						fprintf( fp, "\t\t\t<center>%.4f,%.4f,%.4f</center>\n", 
								 pModelInstance->mXFormMat.M( 0, 3 ) + pModelInstance->mpModel->mCenter.fX,
								 pModelInstance->mXFormMat.M( 1, 3 ) + pModelInstance->mpModel->mCenter.fY,
								 pModelInstance->mXFormMat.M( 2, 3 ) + pModelInstance->mpModel->mCenter.fZ );
						fprintf( fp, "\t\t\t<radius>%.4f</radius>\n",
								 pModelInstance->mfRadius );
						fprintf( fp, "\t\t</object>\n" );

					}   // for i = 0 to num objects
	                
					fprintf( fp, "\t</node>\n" );
				}
                
            }   // for x = 0 to num dimension nodes
            
        }   // for y = 0 to num dimension nodes
        
    }   // for z = 0 to num dimension nodes
    
    fprintf( fp, "</octgrid>\n" );
    
    fclose( fp );
}

/*
**
*/
void octGridLoad( tOctGrid* pOctGrid,
                  const char* szFileName,
                  void* (*allocObject)( const char* szName, void* pUserData0, void* pUserData1, void* pUserData2, void* pUserData3 ),
                  void* pUserData0,
				  void* pUserData1,
				  void* pUserData2,
				  void* pUserData3 )
{
    char szFullPath[256];
    getFullPath( szFullPath, szFileName );
    
    TiXmlDocument doc( szFullPath );
	bool bLoaded = doc.LoadFile();

	if( bLoaded )
	{
		TiXmlNode* pNode = doc.FirstChild();
		while( pNode )
		{
			TiXmlNode* pDimension = pNode->FirstChild( "dimensions" );
			const char* szDimensions = pDimension->FirstChild()->Value();
	        
			tVector4 gridDimension;
			parseVector( &gridDimension, szDimensions );
	        
			TiXmlNode* pSize = pNode->FirstChild( "node_size" );
			const char* szSize = pSize->FirstChild()->Value();
			float fNodeSize = (float)atof( szSize );
	        
			TiXmlNode* pCenter = pNode->FirstChild( "center" );
			const char* szCenter = pCenter->FirstChild()->Value();
			
			tVector4 center;
			parseVector( &center, szCenter );

			// init grid
			octGridInit( pOctGrid, &center, gridDimension.fX, fNodeSize );
	        
			// add object to oct node
			TiXmlNode* pOctNode = pNode->FirstChild( "node" );
			while( pOctNode )
			{
				TiXmlNode* pNumObjects = pOctNode->FirstChild( "num_obj" );
				const char* szNumObjects = pNumObjects->FirstChild()->Value();
				int iNumObjects = atoi( szNumObjects );

				const char* szIndex = pOctNode->FirstChild( "index" )->FirstChild()->Value();
				int iOctNodeIndex = (int)atoi( szIndex );

				if( iNumObjects > 0 )
				{
					TiXmlNode* pObjects = pOctNode->FirstChild( "object" );
					while( pObjects )
					{
						const char* szName = pObjects->FirstChild( "name" )->FirstChild()->Value();
						void* pObject = allocObject( szName, pUserData0, pUserData1, pUserData2, pUserData3 );
	                    
						octGridAddObjectToNode( pOctGrid, iOctNodeIndex, pObject );
						pObjects = pObjects->NextSibling();
					}   
				}
	            
				pOctNode = pOctNode->NextSibling();
				++iOctNodeIndex;
	            
			}   // while valid oct node
	        
			pNode = pNode->NextSibling();
	        
		}   // while grid
	}	// if loaded
}

/*
**
*/
void octGridSaveBinary( tOctGrid const* pOctGrid, 
					    const char* szFileName )
{
	char szFullPath[256];
    getWritePath( szFullPath, szFileName );
    
    FILE* fp = fopen( szFullPath, "wb" );
    assert( fp );

	int iNumDimensionNode = pOctGrid->miNumNodesInDimension;
   
	fwrite( &pOctGrid->mDimension, sizeof( tVector4 ), 1, fp );
	fwrite( &pOctGrid->maNodes[0].mfSize, sizeof( float ), 1, fp );
	fwrite( &pOctGrid->miNumNodesInDimension, sizeof( int ), 1, fp );
	fwrite( &pOctGrid->mCenter, sizeof( tVector4 ), 1, fp );

	// count the number of nodes with objects
	int iValidNode = 0;
	for( int iZ = 0; iZ < iNumDimensionNode; iZ++ )
    {
        for( int iY = 0; iY < iNumDimensionNode; iY++ )
        {
            for( int iX = 0; iX < iNumDimensionNode; iX++ )
            {
                int iIndex = iZ * iNumDimensionNode * iNumDimensionNode + iY * iNumDimensionNode + iX;
                tOctNode const* pNode = &pOctGrid->maNodes[iIndex];
                
				if( pNode->miNumObjects > 0 )
				{
					++iValidNode;
				}
			}
		}
	}

	fwrite( &iValidNode, sizeof( int ), 1, fp );

    for( int iZ = 0; iZ < iNumDimensionNode; iZ++ )
    {
        for( int iY = 0; iY < iNumDimensionNode; iY++ )
        {
            for( int iX = 0; iX < iNumDimensionNode; iX++ )
            {
                int iIndex = iZ * iNumDimensionNode * iNumDimensionNode + iY * iNumDimensionNode + iX;
                tOctNode const* pNode = &pOctGrid->maNodes[iIndex];
                
				if( pNode->miNumObjects > 0 )
				{
					tVector4 const* pCenter = pNode->mpCenter;
					
					fwrite( &iIndex, sizeof( int ), 1, fp );
					fwrite( pCenter, sizeof( tVector4 ), 1, fp );
					fwrite( &pNode->miNumObjects, sizeof( int ), 1, fp );
					
					for( int i = 0; i < pNode->miNumObjects; i++ )
					{
						tModelInstance* pModelInstance = (tModelInstance *)pNode->mapObjects[i];
						size_t iLen = strlen( pModelInstance->mszName );
						fwrite( pModelInstance->mszName, sizeof( char ), iLen, fp );
						fputc( '\0', fp );

						tVector4 center = 
						{ 
							pModelInstance->mXFormMat.M( 0, 3 ),
							pModelInstance->mXFormMat.M( 1, 3 ),
							pModelInstance->mXFormMat.M( 2, 3 ),
							1.0f
						};

						fwrite( &center, sizeof( tVector4 ), 1, fp );
						fwrite( &pModelInstance->mfRadius, sizeof( float ), 1, fp );
                        
					}   // for i = 0 to num objects
				}
                
            }   // for x = 0 to num dimension nodes
            
        }   // for y = 0 to num dimension nodes
        
    }   // for z = 0 to num dimension nodes

	fclose( fp );
}

/*
**
*/
void octGridLoadBinary( tOctGrid* pOctGrid,
					    const char* szFileName,
						void* (*allocObject)( const char* szName, void* pUserData0, void* pUserData1, void* pUserData2, void* pUserData3 ),
						void* pUserData0,
						void* pUserData1,
						void* pUserData2,
						void* pUserData3 )
{
	char szFullPath[256];
    getFullPath( szFullPath, szFileName );

	//FILE* fp = fopen( szFullPath, "rb" );
	FILE* fp = fopen( szFileName, "rb" );
	assert( fp );

	float fNodeSize = 0.0f;

	fread( &pOctGrid->mDimension, sizeof( tVector4 ), 1, fp );
	fread( &fNodeSize, sizeof( float ), 1, fp );
	fread( &pOctGrid->miNumNodesInDimension, sizeof( int ), 1, fp );
	fread( &pOctGrid->mCenter, sizeof( tVector4 ), 1, fp );
	
	pOctGrid->miNumNodes = pOctGrid->miNumNodesInDimension * pOctGrid->miNumNodesInDimension * pOctGrid->miNumNodesInDimension;

	int iValidNodes = 0;
	fread( &iValidNodes, sizeof( int ), 1, fp );

	for( int i = 0; i < iValidNodes; i++ )
	{
		// index of the node 
		int iIndex = 0;
		fread( &iIndex, sizeof( int ), 1, fp );

		assert( iIndex >= 0 && iIndex < pOctGrid->miNumNodes );
		tOctNode* pNode = &pOctGrid->maNodes[iIndex];
		
		// center and number of objects in the node
		tVector4 nodeCenter = { 0.0f, 0.0f, 0.0f, 1.0f };
		fread( &nodeCenter, sizeof( tVector4 ), 1, fp );
		fread( &pNode->miNumObjects, sizeof( int ), 1, fp );
		
		// add objects to node
		for( int j = 0; j < pNode->miNumObjects; j++ )
		{
			char szName[256];
			fread( szName, sizeof( char ), sizeof( szName ), fp );
			void* pObject = allocObject( szName, pUserData0, pUserData1, pUserData2, pUserData3 );
			octGridAddObjectToNode( pOctGrid, iIndex, pObject );
			
			tVector4 modelCenter = { 0.0f, 0.0f, 0.0f, 1.0f };
			float fRadius = 0.0f;

			fread( &modelCenter, sizeof( tVector4 ), 1, fp );
			fread( &fRadius, sizeof( float ), 1, fp );

		}	// for j = 0 to num objects
		
	}	// for i = 0 to num valid nodes

	fclose( fp );
}

/*
**
*/
void octGridAddObjectToNode( tOctGrid* pGrid, int iNodeIndex, void* pObject )
{
    assert( iNodeIndex >= 0 && iNodeIndex < pGrid->miNumNodes );
    tOctNode* pNode = &pGrid->maNodes[iNodeIndex];
    
    if( pNode->miNumObjects + 1 >= pNode->miNumObjectAlloc )
    {
        pNode->miNumObjectAlloc += NUM_OBJECTS_PER_ALLOC;
        pNode->mapObjects = (void **)realloc( pNode->mapObjects, sizeof( void* ) * pNode->miNumObjectAlloc );
    }
    
    pNode->mapObjects[pNode->miNumObjects] = pObject;
    ++pNode->miNumObjects;
}

