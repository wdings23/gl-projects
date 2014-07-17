//
//  grid.cpp
//  Game4
//
//  Created by Dingwings on 9/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "grid.h"
#include "filepathutil.h"
#include "parseutil.h"
#include "timeutil.h"

#include "tinyxml.h"

#define DEFAULT_GRID_SIZE 5.0f

/*
**
*/
CGrid::CGrid( void )
{
    mfGridSize = DEFAULT_GRID_SIZE;
    miNumOctNodesX = mDimension.fX / mfGridSize; 
    miNumOctNodesY = mDimension.fY / mfGridSize;
    miNumOctNodesZ = mDimension.fZ / mfGridSize; 
    
    maOctNodes = NULL;
    miNumTotalNodes = 0;
}

/*
**
*/
CGrid::~CGrid( void )
{
    release();
}

/*
**
*/
void CGrid::init( tVector4 const* pDimension, float fGridSize )
{
    mfGridSize = fGridSize;
    
    memcpy( &mDimension, pDimension, sizeof( mDimension ) );
    miNumOctNodesX = mDimension.fX / mfGridSize; 
    miNumOctNodesY = mDimension.fY / mfGridSize; 
    miNumOctNodesZ = mDimension.fZ / mfGridSize;
    
    miNumTotalNodes = miNumOctNodesX * miNumOctNodesY * miNumOctNodesZ;
    
    if( miNumTotalNodes > 0 )
    {
        maOctNodes = new COctNode[miNumTotalNodes];
        
        // initialize octnodes
        for( int iZ = 0; iZ < miNumOctNodesZ; iZ++ )
        {
            for( int iY = 0; iY < miNumOctNodesY; iY++ )
            {
                for( int iX = 0; iX < miNumOctNodesX; iX++ )
                {
                    tVector4 pos = 
                    { 
                        (float)( iX - miNumOctNodesX / 2 ) * mfGridSize + mfGridSize * 0.5f, 
                        (float)( iY - miNumOctNodesY / 2 ) * mfGridSize + mfGridSize * 0.5f, 
                        (float)( iZ - miNumOctNodesZ / 2 ) * mfGridSize + mfGridSize * 0.5f, 
                        1.0f
                    };
                    
                    int iIndex = iZ * miNumOctNodesZ * miNumOctNodesY + iY * miNumOctNodesX + iX;
                    WTFASSERT2( iIndex >= 0 && iIndex < miNumTotalNodes, "array out of bounds adding object to octnode", NULL );
                    COctNode* pOctNode = &maOctNodes[iIndex];
                    
                    tVector4 topRightBack = 
                    {
                        pos.fX + mfGridSize * 0.5f,
                        pos.fY + mfGridSize * 0.5f,
                        pos.fZ + mfGridSize * 0.5f,
                        1.0f
                    };
                    
                    tVector4 bottomLeftFront = 
                    {
                        pos.fX - mfGridSize * 0.5f,
                        pos.fY - mfGridSize * 0.5f,
                        pos.fZ - mfGridSize * 0.5f,
                        1.0f
                    };
                    
                    tVector4 extent;
                    extent.fX = 0.5f * ( topRightBack.fX - bottomLeftFront.fX );
                    extent.fY = 0.5f * ( topRightBack.fY - bottomLeftFront.fY );
                    extent.fZ = 0.5f * ( topRightBack.fZ - bottomLeftFront.fZ );
                    extent.fW = 1.0f;
                    
                    pOctNode->setSize( mfGridSize );
                    pOctNode->setExtent( &extent );
                    pOctNode->setPosition( &pos );
                    
                    
                }   // for x = 0 to num nodes in x
                
            }   // for y = 0 to num nodes in y
            
        }   // for z = 0 to num nodes in z
    }
}

/*
**
*/
void CGrid::release( void )
{
    delete[] maOctNodes;
    miNumTotalNodes = 0;
    miNumOctNodesX = 0; 
    miNumOctNodesY = 0; 
    miNumOctNodesZ = 0;
}

/*
**
*/
void CGrid::addObject( void* pObject,
                       bool (*pfnInsideOctNode)( tVector4 const* pPos, float fSize, void* pObject ) )
{
    // check for all the octnode that can contain the object
    for( int i = 0; i < miNumTotalNodes; i++ )
    {
        COctNode* pNode = &maOctNodes[i];
        if( pfnInsideOctNode( pNode->getPosition(), pNode->getSize(), pObject ) )
        {
            pNode->addObject( pObject );
            
            printf( "inside octnode pos ( %f, %f, %f )\n",
                    pNode->getPosition()->fX,
                    pNode->getPosition()->fY,
                    pNode->getPosition()->fZ );
        }
    }
}

/*
**
*/
void CGrid::getNodesInFrustum( CCamera const* pCamera,
                               bool (*nodeIsVisible)( CCamera const* pCamera, COctNode const* pNode, void** apBlockers, int iNumBlockers ),
                               COctNode** apVisibleNodes,
                               int* piNumVisibleNodes ) const
{
//printf( "****** START VISIBLE NODES ******\n" );
    
    *piNumVisibleNodes = 0;
    for( int i = 0; i < miNumTotalNodes; i++ )
    {
        COctNode* pNode = &maOctNodes[i];
        bool bInFrustum = false;
        if( pNode->getNumObjects() > 0 )
        {
            bInFrustum = pCamera->cubeInFrustum( pNode->getPosition(), mfGridSize );
        }
        
        // inside frustum, save to list
        if( bInFrustum )
        {
            pNode->computeCameraDistance( pCamera );
            float fCamDistance = pNode->getCameraDistance();
            
            //OUTPUT( "VISIBLE node ( %f, %f, %f )\n",
            //        pNode->getPosition()->fX,
            //        pNode->getPosition()->fY,
            //        pNode->getPosition()->fZ );
            
            // insert
            bool bInserted = false;
            int iNumVisible = *piNumVisibleNodes;
            for( int j = 0; j < iNumVisible; j++ )
            {
                float fCurrCamDistance = apVisibleNodes[j]->getCameraDistance();
                if( fCurrCamDistance > fCamDistance )
                {
                    // shift down
                    for( int k = iNumVisible; k > j; k-- )
                    {
                        apVisibleNodes[k] = apVisibleNodes[k-1];
                        
                    }   // for k = num visible - 1 to j
                    
                    apVisibleNodes[j] = pNode;
                    bInserted = true;
                    
                    break;
                }
            }
            
            // have not inserted, back of the list
            if( !bInserted )
            {
                apVisibleNodes[*piNumVisibleNodes] = pNode;
            }
            
            ++(*piNumVisibleNodes);
            
        }   // if in frustum
    
    }   // for i = 0 to num total octnodes
    
//printf( "****** END VISIBLE NODES ******\n\n" );
}

/*
**
*/
void CGrid::saveToFile( const char* szFileName, void (*pfnWriteObjectInfo)( FILE* fp, void* pObject, int iNumTabs ) )
{
    char szFullPath[256];
    getWritePath( szFullPath, szFileName );
    FILE* fp = fopen( szFullPath, "wb" );
    
    fprintf( fp, "<nodes>\n" );
    fprintf( fp, "\t<dimension>%.3f,%.3f,%.3f</dimension>\n", mDimension.fX, mDimension.fY, mDimension.fZ );
    fprintf( fp, "\t<node_size>%f</node_size>\n", mfGridSize );
    
    for( int i = 0; i < miNumTotalNodes; i++ )
    {
        COctNode* pNode = &maOctNodes[i];
        int iNumObjects = pNode->getNumObjects();
        if( iNumObjects > 0 )
        {
            tVector4 const* pNodePos = pNode->getPosition();
            
            fprintf( fp, "\t<node>\n" );
            fprintf( fp, "\t\t<center>%.3f,%.3f,%.3f</center>\n",
                     pNodePos->fX,
                     pNodePos->fY,
                     pNodePos->fZ );
            
            void** apObjects = pNode->getObjects();
            for( int iObject = 0; iObject < iNumObjects; iObject++ )
            {
                fprintf( fp, "\t\t<object>\n" );
                pfnWriteObjectInfo( fp, apObjects[iObject], 3 );
                fprintf( fp, "\t\t</object>\n" );
                
            }   // for object = 0 to num objects
            
            fprintf( fp, "\t</node>\n" );
        }
        
    }   // for i = 0 to num nodes
    
    fprintf( fp, "</nodes>\n" );
    fclose( fp );
    
}

/*
**
*/
void CGrid::load( const char* szFileName,
                  void* pfnGetObject( const char* szName ) )
{
    float fGridSize = 0.0f;
    tVector4 dimension = { 0.0f, 0.0f, 0.0f, 1.0f };
    
    char szFullPath[256];
    getWritePath( szFullPath, szFileName );
    TiXmlDocument doc( szFullPath );
    bool bLoaded = doc.LoadFile();
    if( bLoaded )
    {
        TiXmlNode* pNode = doc.FirstChild()->FirstChild();
        while( pNode )
        {
            const char* szNodeValue = pNode->Value();
            if( !strcmp( szNodeValue, "dimension" ) )
            {
                const char* szDimension = pNode->FirstChild()->Value();
                parseVector( &dimension, szDimension );
                
            }   // if "dimension"
            else if( !strcmp( szNodeValue, "node_size" ) )
            {
                fGridSize = atof( pNode->FirstChild()->Value() );
                init( &dimension, fGridSize );
                
            }   // if "node_size"
            else if( !strcmp( szNodeValue, "node" ) )
            {
                COctNode* pCurrOctNode = NULL;
                
                TiXmlNode* pChild = pNode->FirstChild();
                while( pChild )
                {
                    const char* szValue = pChild->Value();
                    if( !strcmp( szValue, "object" ) )
                    {
                        TiXmlNode* pObjectNode = pChild->FirstChild();
                        while( pObjectNode )
                        {
                            const char* szValue = pObjectNode->Value();
                            if( !strcmp( szValue, "name" ) )
                            {
                                void* pObject = pfnGetObject( pObjectNode->FirstChild()->Value() );
                                WTFASSERT2( pObject, "can't find object for oct node", NULL );
                                
                                WTFASSERT2( pCurrOctNode, "no octnode to add object", NULL );
                                pCurrOctNode->addObject( pObject );
                            }
                            
                            pObjectNode = pObjectNode->NextSibling();
                            
                        }   // while valid object node
                        
                    }   // "object"
                    else if( !strcmp( szValue, "center" ) )
                    {
                        const char* szCenter = pChild->FirstChild()->Value();
                        
                        tVector4 center;
                        parseVector( &center, szCenter );
                        
                        int iX = (int)( ( center.fX + mDimension.fX * 0.5f ) / mfGridSize );
                        int iY = (int)( ( center.fY + mDimension.fY * 0.5f ) / mfGridSize );
                        int iZ = (int)( ( center.fZ + mDimension.fZ * 0.5f ) / mfGridSize );
                        
                        int iIndex = iZ * miNumOctNodesZ * miNumOctNodesY + iY * miNumOctNodesX + iX;
                        /*WTFASSERT2( iIndex >= 0 && iIndex < miNumTotalNodes, "array out of bounds adding object to octnode", NULL );
                        pCurrOctNode = &maOctNodes[iIndex];
                        
                        tVector4 const* pNodePos = pCurrOctNode->getPosition();
                        
                        tVector4 diff;
                        Vector4Subtract( &diff, &center, pCurrOctNode->getPosition() );
                        WTFASSERT2( fabs( diff.fX ) <= 0.001f && fabs( diff.fY ) < 0.001f && fabs( diff.fZ ) < 0.001f, "not the same octnode ( %f, %f, %f )",
                                    pNodePos->fX,
                                    pNodePos->fY,
                                    pNodePos->fZ );*/
                        
                        for( int i = iIndex; i < miNumTotalNodes; i++ )
                        {
                            tVector4 const* pOctPos = maOctNodes[i].getPosition();
                            if( fabs( pOctPos->fX - center.fX ) <= 0.001f &&
                                fabs( pOctPos->fY - center.fY ) <= 0.001f &&
                                fabs( pOctPos->fZ - center.fZ ) <= 0.001f )
                            {
                                pCurrOctNode = &maOctNodes[i];
                                break;
                            }
                        
                        }   // for i = 0 to num oct nodes
                        
                        WTFASSERT2( pCurrOctNode, "did not find a valid oct node", NULL );
                        
                    }   // "center"
                    
                    pChild = pChild->NextSibling();
                    
                }   // while valid child node
            
            }   // if "node"
            
            pNode = pNode->NextSibling();
            
        }   // while valid node
        
    }
    else
    {
        WTFASSERT2( bLoaded, "error loading %s: %s", szFileName, doc.ErrorDesc() );
    }
    
}

/*
**
*/
void CGrid::drawOctNodes( void )
{
    for( int i = 0; i < miNumTotalNodes; i++ )
    {
        COctNode* pOctNode = &maOctNodes[i];
        pOctNode->draw();
        
    }   // for i = 0 to num total nodes
}

/*
**
*/
bool CGrid::dummyAdd( tVector4 const* pPos, float fSize, void* pObject )
{
    return true;
}