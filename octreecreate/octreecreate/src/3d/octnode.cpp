//
//  octnode.cpp
//  Game4
//
//  Created by Dingwings on 7/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "octnode.h"
#include "ShaderManager.h"

#include <stdlib.h>
#include <assert.h>
#include <math.h>

#define WTFASSERT2( X, ... ) assert( X )

#define OBJECTS_PER_ALLOC 100

/*
**
*/
COctNode::COctNode( void )
{
    miNumObjects = 0;
    miNumObjectsAlloc = OBJECTS_PER_ALLOC;
    mapObjects = (void **)malloc( sizeof( void* ) * miNumObjectsAlloc );
    
    memset( mapChildren, 0, sizeof( mapChildren ) );
    mpParent = NULL;
    
    mfCameraDistance = 0.0f;
}

/*
**
*/
COctNode::COctNode( tVector4 const* pPosition, float fSize, COctNode* pParent )
{
    memcpy( &mCenter, pPosition, sizeof( tVector4 ) );
    
    tVector4 topRightBack = 
    {
        pPosition->fX + fSize * 0.5f,
        pPosition->fY + fSize * 0.5f,
        pPosition->fZ + fSize * 0.5f,
        1.0f
    };
    
    tVector4 bottomLeftFront = 
    {
        pPosition->fX - fSize * 0.5f,
        pPosition->fY - fSize * 0.5f,
        pPosition->fZ - fSize * 0.5f,
        1.0f
    };
    
    mExtent.fX = 0.5f * ( topRightBack.fX - bottomLeftFront.fX );
    mExtent.fY = 0.5f * ( topRightBack.fY - bottomLeftFront.fY );
    mExtent.fZ = 0.5f * ( topRightBack.fZ - bottomLeftFront.fZ );
    mExtent.fW = 1.0f;
    
    miNumObjects = 0;
    miNumObjectsAlloc = OBJECTS_PER_ALLOC;
    mapObjects = (void **)malloc( sizeof( void* ) * miNumObjectsAlloc );
    
    mfSize = fSize;
    
    memset( mapChildren, 0, sizeof( mapChildren ) );
    memset( mapSiblings, 0, sizeof( mapSiblings ) );
    
    mpParent = NULL;
}

/*
**
*/
COctNode::~COctNode( void )
{
    free( mapObjects );
}

/*
**
*/
void COctNode::addObject( void* pObject )
{
	if( mapObjects == NULL )
	{
		 miNumObjectsAlloc = OBJECTS_PER_ALLOC;
		 mapObjects = (void **)malloc( sizeof( void* ) * miNumObjectsAlloc );
	}
    else if( miNumObjects + 1 >= miNumObjectsAlloc )
    {
        miNumObjectsAlloc += OBJECTS_PER_ALLOC;
        mapObjects = (void **)realloc( mapObjects, sizeof( void* ) * miNumObjectsAlloc );
    }
    
    mapObjects[miNumObjects++] = pObject; 
}

/*
**
*/
bool COctNode::intersect( tVector4 const* pPos, float fSize, tVector4 const* pPt0, tVector4 const* pPt1 )
{
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

/*
**
*/
void COctNode::draw( void )
{
#if 0
    glDisable( GL_CULL_FACE );
    //glDisable( GL_DEPTH_TEST );
    glDepthMask( GL_FALSE );
    
    int iShader = CShaderManager::instance()->getShader( "generic" );
    glUseProgram( iShader );
    
    tVector4 topRightBack = 
    {
        mCenter.fX + mfSize * 0.5f,
        mCenter.fY + mfSize * 0.5f,
        mCenter.fZ + mfSize * 0.5f,
        1.0f
    };
    
    tVector4 bottomLeftFront = 
    {
        mCenter.fX - mfSize * 0.5f,
        mCenter.fY - mfSize * 0.5f,
        mCenter.fZ - mfSize * 0.5f,
        1.0f
    };
    
    tVector4 aPos[] = 
    {
        // front
        { bottomLeftFront.fX, topRightBack.fY, bottomLeftFront.fZ, 1.0f },
        { topRightBack.fX, topRightBack.fY, bottomLeftFront.fZ, 1.0f },
        
        { topRightBack.fX, topRightBack.fY, bottomLeftFront.fZ, 1.0f },
        { topRightBack.fX, bottomLeftFront.fY, bottomLeftFront.fZ, 1.0f },
        
        { topRightBack.fX, bottomLeftFront.fY, bottomLeftFront.fZ, 1.0f },
        { bottomLeftFront.fX, bottomLeftFront.fY, bottomLeftFront.fZ, 1.0f },
        
        { bottomLeftFront.fX, bottomLeftFront.fY, bottomLeftFront.fZ, 1.0f },
        { bottomLeftFront.fX, topRightBack.fY, bottomLeftFront.fZ, 1.0f },
        
        // back
        { bottomLeftFront.fX, topRightBack.fY, topRightBack.fZ, 1.0f },
        { topRightBack.fX, topRightBack.fY, topRightBack.fZ, 1.0f },
        
        { topRightBack.fX, topRightBack.fY, topRightBack.fZ, 1.0f },
        { topRightBack.fX, bottomLeftFront.fY, topRightBack.fZ, 1.0f },
        
        { topRightBack.fX, bottomLeftFront.fY, topRightBack.fZ, 1.0f },
        { bottomLeftFront.fX, bottomLeftFront.fY, topRightBack.fZ, 1.0f },
        
        { bottomLeftFront.fX, bottomLeftFront.fY, topRightBack.fZ, 1.0f },
        { bottomLeftFront.fX, topRightBack.fY, topRightBack.fZ, 1.0f },
        
        // top left
        { bottomLeftFront.fX, topRightBack.fY, bottomLeftFront.fZ, 1.0f },
        { bottomLeftFront.fX, topRightBack.fY, topRightBack.fZ, 1.0f },
        
        // top right
        { topRightBack.fX, topRightBack.fY, bottomLeftFront.fZ, 1.0f },
        { topRightBack.fX, topRightBack.fY, topRightBack.fZ, 1.0f },
        
        // bottom left
        { bottomLeftFront.fX, bottomLeftFront.fY, bottomLeftFront.fZ, 1.0f },
        { bottomLeftFront.fX, bottomLeftFront.fY, topRightBack.fZ, 1.0f },
        
        // bottom right
        { topRightBack.fX, bottomLeftFront.fY, bottomLeftFront.fZ, 1.0f },
        { topRightBack.fX, bottomLeftFront.fY, topRightBack.fZ, 1.0f },
    };
    
    tVector2 aUV[] =
    {
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
    };
    
    tVector4 aColor[] =
    {
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
    };
    
    int iPosition = glGetAttribLocation( iShader, "position" );
    int iTexCoord = glGetAttribLocation( iShader, "textureUV" );
    int iColor = glGetAttribLocation( iShader, "color" );
    int iViewProj = glGetUniformLocation( iShader, "viewProjMat" );
    
    glVertexAttribPointer( iPosition, 4, GL_FLOAT, GL_FALSE, 0, aPos );
	glEnableVertexAttribArray( iPosition );
	
	glVertexAttribPointer( iTexCoord, 2, GL_FLOAT, GL_FALSE, 0, aUV );
	glEnableVertexAttribArray( iTexCoord );
    
	glVertexAttribPointer( iColor, 4, GL_FLOAT, GL_FALSE, 0, aColor );
	glEnableVertexAttribArray( iColor );
	
	const tMatrix44* pViewProjMatrix = CCamera::instance()->getViewProjMatrix();
	glUniformMatrix4fv( iViewProj, 1, GL_FALSE, pViewProjMatrix->afEntries );
	
    glDrawArrays( GL_LINES, 0, sizeof( aPos ) / sizeof( *aPos ) );
    
    glEnable( GL_CULL_FACE );
    //glEnable( GL_DEPTH_TEST );
    glDepthMask( GL_TRUE );
#endif // #if 0
}

/*
**
*/
void COctNode::computeCameraDistance( CCamera const* pCamera )
{
    tVector4 const* pCamPos = pCamera->getPosition();
    
    tVector4 diff;
    Vector4Subtract( &diff, &mCenter, pCamPos );
    
    mfCameraDistance = Vector4Magnitude( &diff );
}
