//
//  octgrid.cpp
//  animtest
//
//  Created by Dingwings on 9/26/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "octgrid.h"
#include "camera.h"
#include "filepathutil.h"
#include "timeutil.h"
#include "quaternion.h"

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
    
    pOctGrid->miNumNodesInDimension = (int)ceilf( fTotalSize / fNodeSize );
    int iNumNodes = pOctGrid->miNumNodesInDimension;
    
    pOctGrid->miNumNodes = iNumNodes * iNumNodes * iNumNodes;
    pOctGrid->maNodes = (tOctNode *)MALLOC( sizeof( tOctNode ) * pOctGrid->miNumNodes );
    
    pOctGrid->mpOctNodeCenterFactory = new CFactory<tVector4>();
    
    // dimension
    pOctGrid->mDimension.fX = fTotalSize;
    pOctGrid->mDimension.fY = fTotalSize;
    pOctGrid->mDimension.fZ = fTotalSize;
    pOctGrid->mDimension.fW = 1.0f;
    
    float fHalfNumNodes = (float)( iNumNodes >> 1 );
    
    // nodes
    pOctGrid->maNodeCenters = (tVector4 *)MALLOC( sizeof( tVector4 ) * pOctGrid->miNumNodes );
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
                
				pNode->miNumObjectAlloc = 0;
				pNode->miNumObjects = 0;
				pNode->mapObjects = NULL;
				pNode->mbFlag = false;

#if 0
                // list of objects in the node
                pNode->miNumObjectAlloc = NUM_OBJECTS_PER_ALLOC;
                pNode->miNumObjects = 0;
                pNode->mapObjects = (void **)malloc( sizeof( void* ) * pNode->miNumObjectAlloc );
                
				WTFASSERT2( pNode->mapObjects, "can't allocate enough memory" );
#endif // #if 0

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
        FREE( pOctGrid->maNodes[i].mapObjects );
    }
    
    FREE( pOctGrid->maNodes );
	FREE( pOctGrid->maNodeCenters );

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
                pNode->mapObjects = (void **)REALLOC( pNode->mapObjects, sizeof( void* ) * pNode->miNumObjectAlloc );
            }
                    
            pNode->mapObjects[pNode->miNumObjects] = pObject;
            ++pNode->miNumObjects;
        }
    }
}

/*
**
*/
static void getExtent( tVector4* pBottomLeftNear, tVector4* pTopRightFar, tVector4 const* pPt )
{
    if( pPt->fX > pTopRightFar->fX )
    {
        pTopRightFar->fX = pPt->fX;
    }
    
    if( pPt->fX < pBottomLeftNear->fX )
    {
        pBottomLeftNear->fX = pPt->fX;
    }
    
    if( pPt->fY > pTopRightFar->fY )
    {
        pTopRightFar->fY = pPt->fY;
    }
    
    if( pPt->fY < pBottomLeftNear->fY )
    {
        pBottomLeftNear->fY = pPt->fY;
    }
    
    if( pPt->fZ < pBottomLeftNear->fZ )
    {
        pBottomLeftNear->fZ = pPt->fZ;
    }
    
    if( pPt->fZ > pTopRightFar->fZ )
    {
        pTopRightFar->fZ = pPt->fZ;
    }
    
}

/*
**
*/
void octGridGetVisibleNodes( tOctGrid const* pOctGrid,
                             CCamera const* pCamera,
                             tVisibleOctNodes* pVisibleOctNodes,
                             tVector4 const* pOffset )
{
//double fStart = getTime();

	pVisibleOctNodes->miNumNodes = 0;

    // determine the extent of the frustum
    tVector4 bottomLeftNear = { 9999.0f, 9999.0f, 9999.0f, 1.0f };
    tVector4 topRightFar = { -9999.0f, -9999.0f, -9999.0f, 1.0f };
    
    getExtent( &bottomLeftNear, &topRightFar, &pCamera->mFarBottomLeft );
    getExtent( &bottomLeftNear, &topRightFar, &pCamera->mFarBottomRight );
    getExtent( &bottomLeftNear, &topRightFar, &pCamera->mFarTopLeft );
    getExtent( &bottomLeftNear, &topRightFar, &pCamera->mFarTopRight );
    
    getExtent( &bottomLeftNear, &topRightFar, &pCamera->mNearBottomLeft );
    getExtent( &bottomLeftNear, &topRightFar, &pCamera->mNearBottomRight );
    getExtent( &bottomLeftNear, &topRightFar, &pCamera->mNearTopLeft );
    getExtent( &bottomLeftNear, &topRightFar, &pCamera->mNearTopRight );

    tVector4 nodeSize =
    {
        pOctGrid->mDimension.fX / (float)pOctGrid->miNumNodesInDimension,
        pOctGrid->mDimension.fY / (float)pOctGrid->miNumNodesInDimension,
        pOctGrid->mDimension.fZ / (float)pOctGrid->miNumNodesInDimension,
        1.0
    };
    
    int iNearBottomLeftIndex = 0;
    int iFarTopRightIndex = pOctGrid->miNumNodesInDimension * pOctGrid->miNumNodesInDimension * pOctGrid->miNumNodesInDimension - 1;
    
    tVector4* pGridNearBottomLeft = &pOctGrid->maNodeCenters[iNearBottomLeftIndex];
    tVector4* pGridFarTopRight = &pOctGrid->maNodeCenters[iFarTopRightIndex];
    
    // get the frustum's indices in the grid
    float fLeft = bottomLeftNear.fX / nodeSize.fX;
    float fRight = topRightFar.fX / nodeSize.fX;
    
    float fTop = topRightFar.fY / nodeSize.fY;
    float fBottom = bottomLeftNear.fY / nodeSize.fY;
    
    float fNear = bottomLeftNear.fZ / nodeSize.fZ;
    float fFar = topRightFar.fZ / nodeSize.fZ;
    
    // X
    int iLeft = (int)floorf( fLeft );
    int iRight = (int)ceilf( fRight );
    
    // Y
    int iTop = (int)ceilf( fTop );
    int iBottom = (int)floorf( fBottom );
    
    // Z
    int iNear = (int)floorf( fNear );
    int iFar = (int)ceilf( fFar );
    
    // floor for sign correctness
    if( fLeft < 0.0 )
    {
        iLeft = floorf( fLeft );
    }
    
    if( fRight < 0.0 )
    {
        iRight = floorf( fRight );
    }
    
    if( fTop < 0.0 )
    {
        iTop = floorf( fTop );
    }
    
    if( fBottom < 0.0 )
    {
        iBottom = floorf( fBottom );
    }
    
    if( fNear < 0.0 )
    {
        iNear = floorf( fNear );
    }
    
    if( fFar < 0.0 )
    {
        iFar = floorf( fFar );
    }
    
    // grid index extent
    int iGridLeft = (int)floorf( pGridNearBottomLeft->fX / nodeSize.fX );
    int iGridRight = (int)ceilf( pGridFarTopRight->fX / nodeSize.fX );
    
    int iGridTop = (int)ceilf( pGridFarTopRight->fY / nodeSize.fY );
    int iGridBottom = (int)ceilf( pGridNearBottomLeft->fY / nodeSize.fY );
    
    int iGridNear = (int)floorf( pGridNearBottomLeft->fZ / nodeSize.fZ );
    int iGridFar = (int)ceil( pGridFarTopRight->fZ / nodeSize.fZ );
    
    // clamp frustum extent to octgrid's extent
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
    
    // offset to local
    iLeft -= iGridLeft;
    iRight -= iGridLeft;
    iTop -= iGridBottom;
    iBottom -= iGridBottom;
    iNear -= iGridNear;
    iFar -= iGridNear;
    
	if( iLeft < 0 ) 
	{
		iLeft = 0;
	}

	if( iRight < 0 ) 
	{
		iRight = 0;
	}

	if( iTop < 0 )
	{
		iTop = 0;
	}

	if( iBottom < 0 )
	{
		iBottom = 0;
	}

	if( iNear < 0 )
	{
		iNear = 0;
	}

	if( iFar < 0 )
	{
		iFar = 0;
	}

    int iNumNodes = pOctGrid->miNumNodesInDimension;
    
//double fElapsed0 = getTime() - fStart;
//double fElapsed1 = 0.0;
//double fElapsed2 = 0.0;

//fStart = getTime();
//double fFrustumCheckTime = 0.0;
//double fPushBackTime = 0.0;

    // check for nodes in boundary is within the frustum
    tOctNode const* aNodes = pOctGrid->maNodes;
    for( int iZ = iNear; iZ <= iFar; iZ++ )
    {
        for( int iY = iBottom; iY <= iTop; iY++ )
        {
			int iIndex = iZ * iNumNodes * iNumNodes + iY * iNumNodes + iLeft;
            for( int iX = iLeft; iX <= iRight; iX++ )
			{
                WTFASSERT2( iIndex >= 0 && iIndex < iNumNodes * iNumNodes * iNumNodes, "num nodes out of bounds" );
                tOctNode* pOctNode = (tOctNode *)&aNodes[iIndex];
                
				if( pOctNode->mbFlag == false &&
					pOctNode->miNumObjects > 0 )
				{
//double fSegStart = getCurrTime();

					// sphere check first for rough estimate of the oct node
					bool bCheck0 = pCamera->sphereInFrustum( pOctNode->mpCenter, pOctNode->mfSize * 2.0f );
					if( !bCheck0 )
					{
						++iIndex;
						continue;
					}

					bool bInFrustum = pCamera->cubeInFrustum( pOctNode->mpCenter, pOctNode->mfSize );
//fFrustumCheckTime += ( getCurrTime() - fSegStart );

					if( bInFrustum )
					{
						if( pVisibleOctNodes->miNumNodes + 1 >= pVisibleOctNodes->miNumAlloc )
						{
							pVisibleOctNodes->miNumAlloc += NUM_VISIBLE_NODES_PER_ALLOC;
							pVisibleOctNodes->mapNodes = (tOctNode const**)REALLOC( pVisibleOctNodes->mapNodes, 
																					sizeof( tOctNode const* ) * pVisibleOctNodes->miNumAlloc );
						}
						
						pVisibleOctNodes->mapNodes[pVisibleOctNodes->miNumNodes++] = pOctNode;
						pOctNode->mbFlag = true;
					}
				}

				++iIndex;
                
            }   // for x = left to right
            
        }   // for y = bottom to top
        
    }   // for z = near to far
	
	for( int i = 0; i < pVisibleOctNodes->miNumNodes; i++ )
	{
		( (tOctNode *)pVisibleOctNodes->mapNodes[i] )->mbFlag = false;

	}	// for i = 0 to num nodes

//double fElapsed3 = getTime() - fStart;

}

/*
**
*/
void octGridGetVisibleNodes3( tOctGrid const* pOctGrid,
                              CCamera const* pCamera,
                              tVisibleOctNodes* pVisibleNodes,
							  tVector4 const* pOffset,
							  float fScreenRatio,
							  bool bOrthographic,
							  float fViewWidth,
							  float fViewHeight )
{
	tVector4 const* pCamPos = pCamera->getPosition();

	//tVector4 origin = { 0.0f, 0.0f, 0.0f, 1.0f };
	//tVector4 const* pCamPos = &origin;

	// look at vector
	tVector4 lookAt = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4Subtract( &lookAt, pCamera->getLookAt(), pCamPos );
	Vector4Normalize( &lookAt, &lookAt );
	
//lookAt.fX = lookAt.fY = 0.0f; lookAt.fZ = 1.0f;

	// up vector
	tVector4 up = { 0.0f, 1.0f, 0.0f, 1.0f };
	if( fabs( lookAt.fY ) > fabs( lookAt.fX ) &&
		fabs( lookAt.fY ) > fabs( lookAt.fZ ) )
	{
		up.fX = up.fY = 0.0f;
		up.fZ = 1.0f;
	}
	
	tVector4 right = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4Cross( &right, &up, &lookAt );
	
	tVector4 farCenter = { 0.0f, 0.0f, 0.0f, 1.0f };
	tVector4 farTopLeft = { 0.0f, 0.0f, 0.0f, 1.0f };
	tVector4 farTopRight = { 0.0f, 0.0f, 0.0f, 1.0f };
	tVector4 farBottomLeft = { 0.0f, 0.0f, 0.0f, 1.0f };
	tVector4 farBottomRight = { 0.0f, 0.0f, 0.0f, 1.0f };

	float fFar = 100.0f;

	float fFarWidth = 2.0f * tan( pCamera->getFOV() * 0.5f ) * fFar;
	float fFarHeight = fFarWidth; // * fScreenRatio;

	farCenter.fX = pCamPos->fX + lookAt.fX * fFar;
	farCenter.fY = pCamPos->fY + lookAt.fY * fFar;
	farCenter.fZ = pCamPos->fZ + lookAt.fZ * fFar;
	farCenter.fW = 1.0f;
	
	farTopLeft.fX = farCenter.fX + ( up.fX * fFarHeight * 0.5f ) - ( right.fX * fFarWidth * 0.5f );
	farTopLeft.fY = farCenter.fY + ( up.fY * fFarHeight * 0.5f ) - ( right.fY * fFarWidth * 0.5f );
	farTopLeft.fZ = farCenter.fZ + ( up.fZ * fFarHeight * 0.5f ) - ( right.fZ * fFarWidth * 0.5f);
	farTopLeft.fW = 1.0f;
	
	farTopRight.fX = farCenter.fX + ( up.fX * fFarHeight * 0.5f ) + ( right.fX * fFarWidth * 0.5f );
	farTopRight.fY = farCenter.fY + ( up.fY * fFarHeight * 0.5f ) + ( right.fY * fFarWidth * 0.5f );
	farTopRight.fZ = farCenter.fZ + ( up.fZ * fFarHeight * 0.5f ) + ( right.fZ * fFarWidth * 0.5f );
	farTopRight.fW = 1.0f;
	
	farBottomLeft.fX = farCenter.fX - ( up.fX * fFarHeight * 0.5f ) - ( right.fX * fFarWidth * 0.5f );
	farBottomLeft.fY = farCenter.fY - ( up.fY * fFarHeight * 0.5f ) - ( right.fY * fFarWidth * 0.5f );
	farBottomLeft.fZ = farCenter.fZ - ( up.fZ * fFarHeight * 0.5f ) - ( right.fZ * fFarWidth * 0.5f );
	farBottomLeft.fW = 1.0f;
	
	farBottomRight.fX = farCenter.fX - ( up.fX * fFarHeight * 0.5f ) + ( right.fX * fFarWidth * 0.5f );
	farBottomRight.fY = farCenter.fY - ( up.fY * fFarHeight * 0.5f ) + ( right.fY * fFarWidth * 0.5f );
	farBottomRight.fZ = farCenter.fZ - ( up.fZ * fFarHeight * 0.5f ) + ( right.fZ * fFarWidth * 0.5f );
	farBottomRight.fW = 1.0f;

	tVector4 topLeftV = { 0.0f, 0.0f, 0.0f, 1.0f };
	tVector4 topRightV = { 0.0f, 0.0f, 0.0f, 1.0f };
	tVector4 bottomLeftV = { 0.0f, 0.0f, 0.0f, 1.0f };
	tVector4 bottomRightV = { 0.0f, 0.0f, 0.0f, 1.0f };

	Vector4Subtract( &topLeftV, &farTopLeft, pCamPos );
	Vector4Subtract( &bottomLeftV, &farBottomLeft, pCamPos );
	Vector4Subtract( &topRightV, &farTopRight, pCamPos );
	Vector4Subtract( &bottomRightV, &farBottomRight, pCamPos );
	
	Vector4Normalize( &topLeftV, &topLeftV );
	Vector4Normalize( &bottomLeftV, &bottomLeftV );
	Vector4Normalize( &topRightV, &topRightV );
	Vector4Normalize( &bottomRightV, &bottomRightV );

}

/*
**
*/
void octGridGetVisibleNodes2( tOctGrid const* pOctGrid,
                             CCamera const* pCamera,
                             tVisibleOctNodes* pVisibleNodes,
							 tVector4 const* pOffset,
							 float fScreenRatio,
							 bool bOrthographic,
							 float fViewWidth,
							 float fViewHeight )
{
	octGridGetVisibleNodes3( pOctGrid, 
							 pCamera, 
							 pVisibleNodes, 
							 pOffset, 
							 fScreenRatio,
							 bOrthographic,
							 fViewWidth,
							 fViewHeight );

	const float fNumSegmentsPct = 0.75f;

	pVisibleNodes->miNumNodes = 0;
//double fStart = getCurrTime();

	tVector4 const* pCamPos = pCamera->getPosition();
	
	// look at vector
	tVector4 lookAt = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4Subtract( &lookAt, pCamera->getLookAt(), pCamPos );
	Vector4Normalize( &lookAt, &lookAt );
	
	tVector4 up = { 0.0f, 1.0f, 0.0f, 1.0f };
	if( fabs( lookAt.fY ) > fabs( lookAt.fX ) &&
		fabs( lookAt.fY ) > fabs( lookAt.fZ ) )
	{
		up.fX = up.fY = 0.0f;
		up.fZ = 1.0f;
	}

	// x axis vector
	tVector4 horizontal = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4Cross( &horizontal, &up, &lookAt );
	Vector4Normalize( &horizontal, &horizontal );

	// y axis vector
	tVector4 vertical = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4Cross( &vertical, &lookAt, &horizontal );
	
	const float fNodeSize = 10.0f;
	const float fHalfNodeSize = fNodeSize * 0.5f;
	const float fOneOverNodeSize = 1.0f / fNodeSize;

	// for calculating width
	// ie. x / z = tan( fov * 0.5 )
	// ( x / z ) * z = x, where z = camera's z - pt's z
	float fTwoTanHalfFOV = 2.0f * tanf( pCamera->getFOV() * 0.5f );

	// segments of oct-nodes on the look at vector
	int iNumSegments = (int)( pCamera->getFar() * fOneOverNodeSize * ( 1.0 / fNumSegmentsPct ) );

	int iNumNodesInDimension = pOctGrid->miNumNodesInDimension;
	
	// get grid's center
	int iNearBottomLeftIndex = 0;
    
    tVector4* pGridNearBottomLeft = &pOctGrid->maNodeCenters[iNearBottomLeftIndex];
    
	// grid index extent to offset the index to the grid's coordinate
    int iGridLeft = (int)floorf( pGridNearBottomLeft->fX * fOneOverNodeSize );
    int iGridBottom = (int)ceilf( pGridNearBottomLeft->fY * fOneOverNodeSize );
    int iGridNear = (int)floorf( pGridNearBottomLeft->fZ * fOneOverNodeSize );
    
	// get the center point of the node at camera's position
	// oct-node index, relative to the grid's center
	int iStartOctNodeX = (int)ceilf( pCamPos->fX * fOneOverNodeSize );
	int iStartOctNodeY = (int)ceilf( pCamPos->fY * fOneOverNodeSize );
	int iStartOctNodeZ = (int)ceilf( pCamPos->fZ * fOneOverNodeSize );
	
	iStartOctNodeX -= iGridLeft;
	iStartOctNodeY -= iGridBottom;
	iStartOctNodeZ -= iGridNear;
	
	int iStartIndex = iStartOctNodeZ * iNumNodesInDimension * iNumNodesInDimension +
					  iStartOctNodeY * iNumNodesInDimension +
					  iStartOctNodeX;

	tOctNode const* pStartNode = &pOctGrid->maNodes[iStartIndex];
	tVector4 const* pStartPos = pStartNode->mpCenter;

	for( int i = 0; i < iNumSegments; i++ )
	{	
		float fCurrSection = (float)i;

		// center point for the section on the look at vector
		tVector4 currCenterPt = 
		{
			pStartPos->fX + fCurrSection * lookAt.fX * fNodeSize * fNumSegmentsPct,
			pStartPos->fY + fCurrSection * lookAt.fY * fNodeSize * fNumSegmentsPct,
			pStartPos->fZ + fCurrSection * lookAt.fZ * fNodeSize * fNumSegmentsPct,
			1.0f
		};
		
		// distance length in camera relative z direction
		tVector4 startToCenter = { 0.0f, 0.0f, 0.0f, 1.0f };
		Vector4Subtract( &startToCenter, &currCenterPt, pStartPos );
		float fDistanceZ = Vector4Dot( &startToCenter, &lookAt );
		
		// current section dimension
		float fHalfWidth = fDistanceZ * fTwoTanHalfFOV * 0.5f;
		float fHalfHeight = fHalfWidth * fScreenRatio;
		
		// specified view dimension (orthograhic)
		if( bOrthographic )
		{
			fHalfWidth = fViewWidth * 0.5f;
			fHalfHeight = fViewHeight * 0.5f;
		}
		
		// section boundary
		tVector4 leftPt = 
		{
			currCenterPt.fX - horizontal.fX * fHalfWidth,
			currCenterPt.fY - horizontal.fY * fHalfWidth,
			currCenterPt.fZ - horizontal.fZ * fHalfWidth,
			1.0f
		};

		tVector4 topPt =
		{
			currCenterPt.fX + vertical.fX * fHalfHeight,
			currCenterPt.fY + vertical.fY * fHalfHeight,
			currCenterPt.fZ + vertical.fZ * fHalfHeight,
			1.0f
		};

		tVector4 leftCenterDiff = 
		{
			leftPt.fX - currCenterPt.fX,
			leftPt.fY - currCenterPt.fY,
			leftPt.fZ - currCenterPt.fZ,
			1.0f
		};

		tVector4 topCenterDiff = 
		{
			topPt.fX - currCenterPt.fX,
			topPt.fY - currCenterPt.fY,
			topPt.fZ - currCenterPt.fZ,
			1.0f
		};

		tVector4 topLeftPt = 
		{
			currCenterPt.fX + leftCenterDiff.fX + topCenterDiff.fX,
			currCenterPt.fY + leftCenterDiff.fY + topCenterDiff.fY,
			currCenterPt.fZ + leftCenterDiff.fZ + topCenterDiff.fZ,
			1.0f
		};

		int iNumY = (int)ceilf( ( fHalfHeight * 2.0f ) * fOneOverNodeSize ) + 1;
		int iNumX = (int)ceilf( ( fHalfWidth * 2.0f ) * fOneOverNodeSize ) + 1;

		for( int iY = 0; iY <= iNumY; iY++ )
		{
			float fNodeSizeY = (float)iY * fNodeSize;

			tVector4 startPt = 
			{
				topLeftPt.fX - vertical.fX * fNodeSizeY,
				topLeftPt.fY - vertical.fY * fNodeSizeY,
				topLeftPt.fZ - vertical.fZ * fNodeSizeY,
				1.0f
			};

			for( int iX = 0; iX <= iNumX; iX++ )
			{
				float fNodeSizeX = (float)iX * fNodeSize;

				// current start of consecutive octnode on this line
				tVector4 currPt = 
				{
					startPt.fX + horizontal.fX * fNodeSizeX,
					startPt.fY + horizontal.fY * fNodeSizeX,
					startPt.fZ + horizontal.fZ * fNodeSizeX,
					1.0
				};
				
				// oct-node index, relative to the grid's center
				int iOctNodeX = (int)ceilf( currPt.fX * fOneOverNodeSize );
				int iOctNodeY = (int)ceilf( currPt.fY * fOneOverNodeSize );
				int iOctNodeZ = (int)ceilf( currPt.fZ * fOneOverNodeSize );
				
				iOctNodeX -= iGridLeft;
				iOctNodeY -= iGridBottom;
				iOctNodeZ -= iGridNear;

				// out of bounds
				if( iOctNodeX < 0 || iOctNodeX >= iNumNodesInDimension )
				{
					continue;
				}

				if( iOctNodeY < 0 || iOctNodeX >= iNumNodesInDimension )
				{
					continue;
				}

				if( iOctNodeZ < 0 || iOctNodeZ >= iNumNodesInDimension )
				{
					continue;
				}

				int iIndex = iOctNodeZ * iNumNodesInDimension * iNumNodesInDimension +
							 iOctNodeY * iNumNodesInDimension +
							 iOctNodeX;
				
				// add the node
				tOctNode const* pNode = &pOctGrid->maNodes[iIndex];
				WTFASSERT2( fabs( pNode->mpCenter->fX - currPt.fX ) < fHalfNodeSize * 4.0f, "not the correct node" );
				WTFASSERT2( fabs( pNode->mpCenter->fY - currPt.fY ) < fHalfNodeSize * 4.0f, "not the correct node" );
				WTFASSERT2( fabs( pNode->mpCenter->fZ - currPt.fZ ) < fHalfNodeSize * 4.0f, "not the correct node" );

				if( pNode->miNumObjects > 0 )
				{
					if( !pNode->mbFlag )
					{
						//aVisibleNodes.push_back( pNode );
						if( pVisibleNodes->miNumNodes + 1 >= pVisibleNodes->miNumAlloc )
						{
							pVisibleNodes->miNumAlloc += NUM_VISIBLE_NODES_PER_ALLOC;
							pVisibleNodes->mapNodes = (tOctNode const **)REALLOC( pVisibleNodes->mapNodes, sizeof( tOctNode* ) * pVisibleNodes->miNumAlloc );
						}
						
						// mark as added
						pVisibleNodes->mapNodes[pVisibleNodes->miNumNodes++] = pNode;
						((tOctNode *)pNode)->mbFlag = true;
					}
				}

			}	// for x = 0 to section width

		}	// for y = 0 to section height

	}	// for i = 0 to num segments

//double fElapsed0 = getCurrTime() - fStart;
//fStart = getCurrTime();
	
	// reset flag
	for( int i = 0; i < pVisibleNodes->miNumNodes; i++ )
	{
		((tOctNode *)pVisibleNodes->mapNodes[i])->mbFlag = false;
	}

//double fElapsed1 = getCurrTime() - fStart;
}

#include "modelinstance.h"
#include "tinyxml.h"
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
                                pModelInstance->mXFormMat.M( 0, 3 ),
                                pModelInstance->mXFormMat.M( 1, 3 ),
                                pModelInstance->mXFormMat.M( 2, 3 ) );
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
	// USE THIS ONLY FOR SMALL FILES, binary is preferable

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
    else
    {
        OUTPUT( "can't load %s: %s\n", szFileName, doc.ErrorDesc() );
    }
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

	FILE* fp = fopen( szFullPath, "rb" );

	float fNodeSize = 0.0f;

	fread( &pOctGrid->mDimension, sizeof( tVector4 ), 1, fp );
	fread( &fNodeSize, sizeof( float ), 1, fp );
	fread( &pOctGrid->miNumNodesInDimension, sizeof( int ), 1, fp );
	fread( &pOctGrid->mCenter, sizeof( tVector4 ), 1, fp );

	// init grid
	octGridInit( pOctGrid, &pOctGrid->mCenter, pOctGrid->mDimension.fX, fNodeSize );

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
		int iNumObjects = 0;
		fread( &nodeCenter, sizeof( tVector4 ), 1, fp );
		fread( &iNumObjects, sizeof( int ), 1, fp );
		
		tVector4 diff;
		Vector4Subtract( &diff, &nodeCenter, pNode->mpCenter );
		WTFASSERT2( fabs( diff.fX ) < 0.01f && fabs( diff.fY ) < 0.01f && fabs( diff.fZ ) < 0.01f, "not the same node center" );

		// add objects to node
		for( int j = 0; j < iNumObjects; j++ )
		{
			char szName[256];
			char* pszName = szName;
			memset( szName, 0, sizeof( szName ) );
			
			char c;
			while( ( c = fgetc( fp ) ) != '\0' )
			{
				*pszName = c;
				++pszName;
			}

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
        
		if( pNode->mapObjects == NULL )
		{
			pNode->mapObjects = (void **)MALLOC( sizeof( void* ) * pNode->miNumObjectAlloc );
		}
		else
		{
			pNode->mapObjects = (void **)REALLOC( pNode->mapObjects, sizeof( void* ) * pNode->miNumObjectAlloc );
		}
	}
    
    pNode->mapObjects[pNode->miNumObjects] = pObject;
    ++pNode->miNumObjects;
}

