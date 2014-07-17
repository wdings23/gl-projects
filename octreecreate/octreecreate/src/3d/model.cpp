//
//  model.cpp
//  animtest
//
//  Created by Tony Peng on 8/8/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "model.h"
#include "../tinyxml/tinyxml.h"
#include "../util/fileutil.h"
#include "../util/parseutil.h"
#include <math.h>

#include <vector>

#define WTFASSERT2( X, ... ) assert( X )

static int readVerts( TiXmlNode* pRoot, int iNumTotalVerts, tVector4* aPos, tModel* pModel );
static int readUV( TiXmlNode* pRoot, int iNumTotalUV, tVector2* aUV, tModel const* pModel );
static int readNormals( TiXmlNode* pRoot, int iNumTotalNormal, tVector4* aNormal, tModel const* pModel );
static int readFace( TiXmlNode* pRoot,
                    int iNumTotalFaces,
                    int iNumTotalVert,
                    int iNumTotalUV,
					int iNumTotalNormals,
                    tFace* aFaces,
                    tVector4* aNormals,
                    tModel const* pModel );

static void createVBOVerts( tModel* pModel );

static bool sameVec( tVector4 const* pV0, tVector4 const* pV1 );
static bool sameVec2( tVector2 const* pV0, tVector2 const* pV1 );


/*
**
*/
void modelInit( tModel* pModel )
{
    memset( pModel, 0, sizeof( tModel ) );
    pModel->mszName = (char *)malloc( sizeof( char ) * 256 );
}

/*
**
*/
void modelRelease( tModel* pModel )
{
    free( pModel->maPos );
    free( pModel->maNorm );
    free( pModel->maUV );
    free( pModel->maFaces );
    free( pModel->mszName );
    
    pModel->miNumVerts = 0;
    pModel->miNumUV = 0;
    pModel->miNumNormals = 0;
    pModel->miNumFaces = 0;
    
    pModel->mbLoaded = false;
}

/*
**
*/
void modelLoad( tModel* pModel, const char* szFileName )
{
    char szFullPath[256];
    getFullPath( szFullPath, szFileName );
    TiXmlDocument doc( szFullPath );
    
    std::vector<const char*> aNames;
    
    bool bLoaded = doc.LoadFile();
    if( bLoaded )
    {
        TiXmlNode* pRoot = doc.FirstChild( "model" );
        TiXmlNode* pMeshNode = pRoot->FirstChild( "mesh" );
        TiXmlNode* pFaceInfoNode = pRoot->FirstChild( "faceInfo" );
        
        // count number of entries
        int iTotalVerts = 0;
        int iTotalUV = 0;
        int iTotalFaces = 0;
		int iTotalNormals = 0;
        while( pMeshNode )
        {
            const char* szName = pMeshNode->FirstChild( "properties" )->FirstChild( "name" )->FirstChild()->Value();
            aNames.push_back( szName );
            
            TiXmlNode* pNumVertNode = pMeshNode->FirstChild( "properties" )->FirstChild( "numVertices" );
            const char* szData = pNumVertNode->FirstChild()->Value();
            int iNumVerts = atoi( szData );
            
            TiXmlNode* pNumUVNode = pMeshNode->FirstChild( "properties" )->FirstChild( "numTextureCoordinates" );
            szData = pNumUVNode->FirstChild()->Value();
            int iNumUV = atoi( szData );
            
			szData = pNumUVNode->FirstChild()->Value();
            int iNumNormals = atoi( szData );

            TiXmlNode* pNumFaceNode = pFaceInfoNode->FirstChild( "properties" )->FirstChild( "size" );
            szData = pNumFaceNode->FirstChild()->Value();
            int iNumFaces = atoi( szData );

            iTotalVerts += iNumVerts;
            iTotalFaces += iNumFaces;
			iTotalNormals += iNumNormals;
            iTotalUV += iNumUV;
            
            //OUTPUT( "number of vert=%d total vert=%d\n", iNumVerts, iTotalVerts );
            
            pMeshNode = pMeshNode->NextSibling( "mesh" );
            pFaceInfoNode = pFaceInfoNode->NextSibling( "faceInfo" );
            
        }   // while still reading in mesh
    
        // allocate info
        pModel->maPos = (tVector4 *)malloc( sizeof( tVector4 ) * iTotalVerts );
        pModel->maNorm = (tVector4 *)malloc( sizeof( tVector4 ) * iTotalFaces * 3 );
        pModel->maUV = (tVector2 *)malloc( sizeof( tVector2 ) * iTotalUV );
        pModel->maFaces = (tFace *)malloc( sizeof( tFace ) * iTotalFaces );
        
        pModel->miNumVerts = iTotalVerts;
        pModel->miNumFaces = iTotalFaces;
        pModel->miNumUV = iTotalUV;
        pModel->miNumNormals = iTotalFaces * 3;
        
        pMeshNode = pRoot->FirstChild( "mesh" );
        pFaceInfoNode = pRoot->FirstChild( "faceInfo" );
        
        // read in info
        int iCurrTotalVerts = 0;
        int iCurrTotalUV = 0;
        int iCurrTotalFaces = 0;
		int iCurrTotalNormals = 0;
        while( pMeshNode )
        {
            int iNumVerts = readVerts( pMeshNode, iCurrTotalVerts, pModel->maPos, pModel );
            int iNumUV = readUV( pMeshNode, iCurrTotalUV, pModel->maUV, pModel );
			int iNumNormal = readNormals( pMeshNode, iCurrTotalNormals, pModel->maNorm, pModel );
			int iNumFaces = readFace( pFaceInfoNode,
                                     iCurrTotalFaces,
                                     iCurrTotalVerts,
                                     iCurrTotalUV,
									 iCurrTotalNormals,
                                     pModel->maFaces,
                                     pModel->maNorm,
                                     pModel );
            
            iCurrTotalVerts += iNumVerts;
            iCurrTotalUV += iNumUV;
            iCurrTotalNormals += iNumNormal;
			iCurrTotalFaces += iNumFaces;
            
            //OUTPUT( "number of vert=%d total vert=%d\n", iNumVerts, iCurrTotalVerts );
            
            WTFASSERT2( iCurrTotalVerts <= iTotalVerts );
            WTFASSERT2( iCurrTotalUV <= iTotalUV );
            WTFASSERT2( iCurrTotalFaces <= iTotalFaces );
            
            pMeshNode = pMeshNode->NextSibling( "mesh" );
            pFaceInfoNode = pFaceInfoNode->NextSibling( "faceInfo" );
            
        }   // while still reading in mesh
        
        createVBOVerts( pModel );
        
    }   // if loaded
	else
	{
		WTFASSERT2( 0, "can't load %s : %s\n", 
					szFileName, 
					doc.ErrorDesc() );
	}
    
    pModel->mbLoaded = true;
}

/*
**
*/
void modelSetupGL( tModel* pModel )
{
    pModel->miNumVBOIndices = pModel->miNumFaces * 3;
    pModel->miNumVBOVerts = pModel->miNumVBOVerts;
    
#if 0
    // data array
    glGenBuffers( 1, &pModel->miVBO );
    glBindBuffer( GL_ARRAY_BUFFER, pModel->miVBO );
    glBufferData( GL_ARRAY_BUFFER, sizeof( tInterleaveVert ) * pModel->miNumVBOVerts, pModel->maVBOVerts, GL_STATIC_DRAW );
    
    // index array
    glGenBuffers( 1, &pModel->miIBO );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pModel->miIBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( int ) * pModel->miNumVBOIndices, pModel->maiVBOIndices, GL_STATIC_DRAW );
    
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
#endif // #if 0
}

/*
**
*/
static int readVerts( TiXmlNode* pRoot, int iNumTotalVerts, tVector4* aPos, tModel* pModel )
{
    tVector4 minPos = { 999999.0f, 999999.0f, 999999.0f, 1.0f };
    tVector4 maxPos = { -999999.0f, -999999.0f, -999999.0f, 1.0f };
    
    int iNumVerts = 0;
    TiXmlNode* pVertNode = pRoot->FirstChild( "data" )->FirstChild( "vertex" );
    while( pVertNode )
    {
        TiXmlElement* pElement = pVertNode->ToElement();
        const char* szX = pElement->Attribute( "x" );
        const char* szY = pElement->Attribute( "y" );
        const char* szZ = pElement->Attribute( "z" );
        
        int iIndex = iNumTotalVerts + iNumVerts;
        WTFASSERT2( iIndex < pModel->miNumVerts, "too many vertices" );
        
        tVector4* pPos = &aPos[iIndex];
        pPos->fX = (float)atof( szX );
        pPos->fY = (float)atof( szY );
        pPos->fZ = (float)atof( szZ );
        pPos->fW = 1.0f;
        
        //pPos->fX *= -1.0f;
        
        if( pPos->fX < minPos.fX )
        {
            minPos.fX = pPos->fX;
        }
        
        if( pPos->fY < minPos.fY )
        {
            minPos.fY = pPos->fY;
        }
        
        if( pPos->fZ < minPos.fZ )
        {
            minPos.fZ = pPos->fZ;
        }
        
        if( pPos->fX > maxPos.fX )
        {
            maxPos.fX = pPos->fX;
        }
        
        if( pPos->fY > maxPos.fY )
        {
            maxPos.fY = pPos->fY;
        }
        
        if( pPos->fZ > maxPos.fZ )
        {
            maxPos.fZ = pPos->fZ;
        }
        
        ++iNumVerts;
        pVertNode = pVertNode->NextSibling( "vertex" );
    }
    
    pModel->mDimension.fX = maxPos.fX - minPos.fX;
    pModel->mDimension.fY = maxPos.fY - minPos.fY;
    pModel->mDimension.fZ = maxPos.fZ - minPos.fZ;
    pModel->mDimension.fW = 1.0f;
    
    pModel->mCenter.fX = ( maxPos.fX + minPos.fX ) * 0.5f;
    pModel->mCenter.fY = ( maxPos.fY + minPos.fY ) * 0.5f;
    pModel->mCenter.fZ = ( maxPos.fZ + minPos.fZ ) * 0.5f;
    pModel->mCenter.fW = 1.0f;
    
    return iNumVerts;
}

/*
**
*/
static int readUV( TiXmlNode* pRoot, int iNumTotalUV, tVector2* aUV, tModel const* pModel )
{
    int iNumUV = 0;
    TiXmlNode* pUVNode = pRoot->FirstChild( "data" )->FirstChild( "textureCoordinate" );
    while( pUVNode )
    {
        TiXmlElement* pElement = pUVNode->ToElement();
        const char* szX = pElement->Attribute( "u" );
        const char* szY = pElement->Attribute( "v" );
        
        int iIndex = iNumTotalUV + iNumUV;
        WTFASSERT2( iIndex < pModel->miNumUV, "too many uv" );
        
        tVector2* pUV = &aUV[iIndex];
        pUV->fX = (float)atof( szX );
        pUV->fY = (float)atof( szY );
        
        ++iNumUV;
        pUVNode = pUVNode->NextSibling( "textureCoordinate" );
    }
    
    return iNumUV;
}

/*
**
*/
static int readNormals( TiXmlNode* pRoot, int iNumTotalNormal, tVector4* aNormal, tModel const* pModel )
{
	int iNumNorm = 0;
	TiXmlNode* pNormNode = pRoot->FirstChild( "data" )->FirstChild( "normal" );
	while( pNormNode )
	{
		const char* szVector = pNormNode->FirstChild()->Value();

		int iIndex = iNumTotalNormal + iNumNorm;
		WTFASSERT2( iIndex < pModel->miNumNormals, "too many uv" );

		tVector4* pNormal = &aNormal[iIndex];
		parseVector( pNormal, szVector );

		++iNumNorm;
		pNormNode = pNormNode->NextSibling( "normal" );
	}

	return iNumNorm;
}

/*
**
*/
static int readFace( TiXmlNode* pRoot,
                    int iNumTotalFaces,
                    int iNumTotalVert,
                    int iNumTotalUV,
					int iNumTotalNormals,
                    tFace* aFaces,
                    tVector4* aNormals,
                    tModel const* pModel )
{
    int iNumFace = 0;
    int iNIndex = iNumTotalFaces * 3;
    TiXmlNode* pFaceNode = pRoot->FirstChild( "data" )->FirstChild( "verticesInFace" );
    while( pFaceNode )
    {
        TiXmlNode* pVNode = pFaceNode->FirstChild( "vertex" );
        
        tFace* pFace = &aFaces[iNumTotalFaces+iNumFace];
        int iVIndex = 0;
        while( pVNode )
        {
            TiXmlElement* pElement = pVNode->ToElement();
            int iV = atoi( pElement->Attribute( "id" ) );
            int iUV = atoi( pElement->Attribute( "uv" ) );
            int iNorm = atoi( pElement->Attribute( "normal" ) );

            pFace->maiNorm[iVIndex] = iNorm + iNumTotalNormals;
            pFace->maiV[iVIndex] = iV + iNumTotalVert;
            pFace->maiUV[iVIndex] = iUV + iNumTotalUV;
            
            ++iVIndex;
            ++iNIndex;
            
            // reverse face winding
            if( iVIndex > 0 && iVIndex % 3 == 0 )
            {
                int iTemp = pFace->maiNorm[iVIndex-3];
                pFace->maiNorm[iVIndex-3] = pFace->maiNorm[iVIndex-1];
                pFace->maiNorm[iVIndex-1] = iTemp;
                
                iTemp = pFace->maiV[iVIndex-3];
                pFace->maiV[iVIndex-3] = pFace->maiV[iVIndex-1];
                pFace->maiV[iVIndex-1] = iTemp;
                
                iTemp = pFace->maiUV[iVIndex-3];
                pFace->maiUV[iVIndex-3] = pFace->maiUV[iVIndex-1];
                pFace->maiUV[iVIndex-1] = iTemp;
            }
            
            pVNode = pVNode->NextSibling( "vertex" );
        }
        
        ++iNumFace;
        pFaceNode = pFaceNode->NextSibling( "verticesInFace" );
    }
    
    return iNumFace;
}

/*
**
*/
static void createVBOVerts( tModel* pModel )
{
    int iNumVBOVerts = pModel->miNumVerts;
    
    pModel->maiVBOIndices = (unsigned int *)malloc( sizeof( int ) * pModel->miNumFaces * 3 );
    pModel->miNumVBOVerts = iNumVBOVerts;
    pModel->maVBOVerts = (tInterleaveVert *)malloc( sizeof( tInterleaveVert ) * pModel->miNumFaces * 3 );
    pModel->maVBOVertPtrs = (tInterleaveVertMap *)malloc( sizeof( tInterleaveVertMap ) * pModel->miNumFaces * 3 );
    
    int iCurrVBOVerts = 0;
    int iCount = 0;
    
    int iNumFaces = pModel->miNumFaces;
    for( int i = 0; i < iNumFaces; i++ )
    {
        tFace* pFace = &pModel->maFaces[i];
        
        // look for position and normal corresponding to the uv
        for( int j = 0; j < 3; j++ )
        {
            int iUV = pFace->maiUV[j];
            int iNorm = pFace->maiNorm[j];
            int iV = pFace->maiV[j];
            
            pModel->maiVBOIndices[iCount] = iCount;
            
            // new vertex
            WTFASSERT2( iCount < pModel->miNumFaces * 3, "array out of bounds" );
            tInterleaveVert* pInterleaveVert = &pModel->maVBOVerts[iCount];
            
            tVector4 const* pPos = &pModel->maPos[iV];
            tVector4 const* pNorm = &pModel->maNorm[iNorm];
            tVector2 const* pUV = &pModel->maUV[iUV];
            
            memcpy( &pInterleaveVert->mPos, pPos, sizeof( tVector4 ) );
            memcpy( &pInterleaveVert->mNorm, pNorm, sizeof( tVector4 ) );
            memcpy( &pInterleaveVert->mUV, pUV, sizeof( tVector2 ) );
            
            tInterleaveVertMap* pPtr = &pModel->maVBOVertPtrs[iCount];
            pPtr->miPos = iV;
            pPtr->miNorm = iNorm;
            pPtr->miUV = iUV;
            
            ++iCurrVBOVerts;
            ++iCount;
            
        }   // for j = 0 to 3
        
    }   // for i = 0 to num faces
    
    // repeat indices to delete
    int* aiToDelete = (int *)malloc( sizeof( int ) * iCount );
    int iNumDelete = 0;
    
    // look for repeats
    int iTrimmed = 0;
    for( int i = 0; i < iCount; i++ )
    {
        tVector4 const* pPos = &pModel->maVBOVerts[i].mPos;
        tVector4 const* pNorm = &pModel->maVBOVerts[i].mNorm;
        tVector2 const* pUV = &pModel->maVBOVerts[i].mUV;
        
        bool bRepeat = false;
        for( int j = i + 1; j < iCount; j++ )
        {
            tVector4 const* pCheckPos = &pModel->maVBOVerts[j].mPos;
            tVector4 const* pCheckNorm = &pModel->maVBOVerts[j].mNorm;
            tVector2 const* pCheckUV = &pModel->maVBOVerts[j].mUV;
            
            if( sameVec( pPos, pCheckPos ) &&
               sameVec( pNorm, pCheckNorm ) &&
               sameVec2( pUV, pCheckUV ) )
            {
                WTFASSERT2( iNumDelete < iCount, "array out of bounds" );
                aiToDelete[iNumDelete++] = j;
                bRepeat = true;
                break;
            }
            
        }   // for j = i to num total vbo vertices
        
        // not a repeat, add to list
        if( !bRepeat )
        {
            tInterleaveVert* pV = &pModel->maVBOVerts[iTrimmed];
            memcpy( &pV->mPos, pPos, sizeof( tVector4 ) );
            memcpy( &pV->mNorm, pNorm, sizeof( tVector4 ) );
            memcpy( &pV->mUV, pUV, sizeof( tVector2 ) );
            
            tInterleaveVertMap* pPtr = &pModel->maVBOVertPtrs[iTrimmed];
            pPtr->miPos = pModel->maVBOVertPtrs[i].miPos;
            pPtr->miNorm = pModel->maVBOVertPtrs[i].miNorm;
            pPtr->miUV = pModel->maVBOVertPtrs[i].miUV;
            
            ++iTrimmed;
        }
        
    }   // for i = 0 to total vbo vertices
    
    pModel->miNumVBOVerts = iCount - iNumDelete;
    WTFASSERT2( iTrimmed == pModel->miNumVBOVerts, "didn't trim vbo vert right" );
    tInterleaveVert* aTrimmedVBOVerts = (tInterleaveVert *)malloc( pModel->miNumVBOVerts * sizeof( tInterleaveVert ) );
    
    pModel->maVBOVertPtrs = (tInterleaveVertMap *)realloc( pModel->maVBOVertPtrs, sizeof( tInterleaveVertMap ) * pModel->miNumVBOVerts );
    
    // set the vbo face indices with trimmed vertices
    iCount = 0;
    for( int i = 0; i < iNumFaces; i++ )
    {
        tFace const* pFace = &pModel->maFaces[i];
        for( int j = 0; j < 3; j++ )
        {
            int iV = pFace->maiV[j];
            int iUV = pFace->maiUV[j];
            int iNorm = pFace->maiNorm[j];
            
            tVector4* pPos = &pModel->maPos[iV];
            tVector4* pNorm = &pModel->maNorm[iNorm];
            tVector2* pUV = &pModel->maUV[iUV];
            
            // determine if it's this vertex
            bool bFound = false;
            for( int k = 0; k < pModel->miNumVBOVerts; k++ )
            {
                tInterleaveVert* pV = &pModel->maVBOVerts[k];
                if( sameVec( pPos, &pV->mPos ) &&
                   sameVec( pNorm, &pV->mNorm ) &&
                   sameVec2( pUV, &pV->mUV ) )
                {
                    pModel->maiVBOIndices[iCount++] = k;
                    bFound = true;
                    break;
                }
                
            }   // for k = 0 to num vbo verts
            
            WTFASSERT2( bFound, "didn't find a matching vertex" );
            
        }   // for j = 0 to 3
        
    }   // for i = 0 to num faces
    
    free( pModel->maVBOVerts );
    pModel->maVBOVerts = aTrimmedVBOVerts;
    
    free( aiToDelete );
}

/*
**
*/
static bool sameVec( tVector4 const* pV0, tVector4 const* pV1 )
{
    tVector4 diff;
    Vector4Subtract( &diff, pV0, pV1 );
    
    return ( fabsf( diff.fX ) < 0.001f && fabsf( diff.fY ) < 0.001f && fabsf( diff.fZ ) < 0.001f );
}

/*
**
*/
static bool sameVec2( tVector2 const* pV0, tVector2 const* pV1 )
{
    tVector2 diff =
    {
        pV0->fX - pV1->fX,
        pV0->fY - pV1->fY
    };
    
    return ( fabsf( diff.fX ) < 0.001f && fabsf( diff.fY ) < 0.001f );
}
