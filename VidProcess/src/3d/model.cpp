//
//  model.cpp
//  animtest
//
//  Created by Tony Peng on 8/8/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "model.h"
#include "tinyxml.h"
#include "filepathutil.h"
#include "parseutil.h"

#include <vector>

#include <stdio.h>

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
    pModel->mszName = (char *)MALLOC( sizeof( char ) * 256 );
    pModel->mszFileName = (char *)MALLOC( sizeof( char ) * 256 );
}

/*
**
*/
void modelRelease( tModel* pModel )
{
    FREE( pModel->maPos );
    FREE( pModel->maNorm );
    FREE( pModel->maUV );
    FREE( pModel->maFaces );
    FREE( pModel->mszName );
    FREE( pModel->mszFileName );
    
    pModel->miNumVerts = 0;
    pModel->miNumUV = 0;
    pModel->miNumNormals = 0;
    pModel->miNumFaces = 0;
    
    //pModel->mbLoaded = false;
	pModel->miLoadState = MODEL_LOADSTATE_NOT_LOADED;
}

/*
**
*/
void modelLoad( tModel* pModel, const char* szFileName )
{
    //OUTPUT( "!!! LOAD MODEL %s !!!\n", szFileName );
    
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
            
			TiXmlNode* pNumNormalNode = pMeshNode->FirstChild( "properties" )->FirstChild( "numNormals" );
			szData = pNumNormalNode->FirstChild()->Value();
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
        pModel->maPos = (tVector4 *)MALLOC( sizeof( tVector4 ) * iTotalVerts );
        pModel->maNorm = (tVector4 *)MALLOC( sizeof( tVector4 ) * iTotalNormals );
        pModel->maUV = (tVector2 *)MALLOC( sizeof( tVector2 ) * iTotalUV );
        pModel->maFaces = (tFace *)MALLOC( sizeof( tFace ) * iTotalFaces );
        
        pModel->miNumVerts = iTotalVerts;
        pModel->miNumFaces = iTotalFaces;
        pModel->miNumUV = iTotalUV;
        pModel->miNumNormals = iTotalNormals;
        
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
            
            WTFASSERT2( iCurrTotalVerts <= iTotalVerts, "too many vertices" );
            WTFASSERT2( iCurrTotalUV <= iTotalUV, "too many uv" );
            WTFASSERT2( iCurrTotalFaces <= iTotalFaces, "too many faces" );
            
            pMeshNode = pMeshNode->NextSibling( "mesh" );
            pFaceInfoNode = pFaceInfoNode->NextSibling( "faceInfo" );
            
        }   // while still reading in mesh
        
        createVBOVerts( pModel );
        
    }   // if loaded
    else
    {
        WTFASSERT2( 0, "can't load %s : %s", szFileName, doc.ErrorDesc() );
    }
    
    //pModel->mbLoaded = true;
	pModel->miLoadState = MODEL_LOADSTATE_LOADED;
}

/*
**
*/
void modelSetupGL( tModel* pModel )
{
    pModel->miNumVBOIndices = pModel->miNumFaces * 3;
    pModel->miNumVBOVerts = pModel->miNumVBOVerts;
    
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

#if 0
/*
**
*/
static void consolidateNormals( tModel* pModel )
{
    int iNumFaces = pModel->miNumFaces;
    int iNumNormals = iNumFaces * 3;
    
    tVector4* aUnique = (tVector4 *)malloc( iNumNormals * sizeof( tVector4 ) );
    int iNumUniques = 0;
    
    // get the unique normal positions
    for( int i = 0; i < iNumNormals; i++ )
    {
        bool bDuplicate = false;
        for( int j = 0; j < iNumUniques; j++ )
        {
            if( sameVec( &pModel->maNorm[i], &aUnique[j] ) )
            {
                bDuplicate = true;
                break;
            }
        }
        
        // unique
        if( !bDuplicate )
        {
            memcpy( &aUnique[iNumUniques], &pModel->maNorm[i], sizeof( tVector4 ) );
            ++iNumUniques;
        }
        
    }   // for i = 0 to num normals
    
	WTFASSERT2( iNumUniques < iNumNormals, "too many unique normals" );
	tVector4* aUnique2 = (tVector4 *)malloc( iNumUniques * sizeof( tVector4 ) );
	memcpy( aUnique2, aUnique, sizeof( tVector4 ) * iNumUniques );
	FREE( aUnique );

    // use the unique normal for face indices
    for( int i = 0; i < iNumFaces; i++ )
    {
        tFace* pFace = &pModel->maFaces[i];
        for( int j = 0; j < 3; j++ )
        {
            int iV = pFace->maiNorm[j];
            tVector4* pOldPos = &pModel->maNorm[iV];
            
            bool bFound = false;
            for( int k = 0; k < iNumUniques; k++ )
            {
                tVector4* pPos = &aUnique2[k];
                if( sameVec( pPos, pOldPos ) )
                {
                    pFace->maiNorm[j] = k;
                    bFound = true;
                    break;
                }
                
            }   // for k = 0 to num uniques
            
            WTFASSERT2( bFound, "didn't find matching normal" );
            
        }   // for j = 0 to 3
        
    }   // for i = 0 to num faces

	// set to unique normals
    FREE( pModel->maNorm );
    pModel->maNorm = aUnique2;
    pModel->miNumNormals = iNumUniques;

}
#endif // #if 0

/*
**
*/
static void createVBOVerts( tModel* pModel )
{
    int iNumVBOVerts = pModel->miNumVerts;
    
	pModel->miNumVBOIndices = pModel->miNumFaces * 3;
    pModel->maiVBOIndices = (unsigned int *)MALLOC( sizeof( int ) * pModel->miNumVBOIndices );
    pModel->miNumVBOVerts = iNumVBOVerts;
    pModel->maVBOVerts = (tInterleaveVert *)MALLOC( sizeof( tInterleaveVert ) * pModel->miNumFaces * 3 );
    pModel->maVBOVertPtrs = (tInterleaveVertMap *)MALLOC( sizeof( tInterleaveVertMap ) * pModel->miNumFaces * 3 );

    int iCurrVBOVerts = 0;
    int iCount = 0;
    
    // initial un-trimmed VBO vertices
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
    
    // potential repeat indices to delete
    int* aiToDelete = (int *)MALLOC( sizeof( int ) * iCount );
    int iNumDelete = 0;
    
    tInterleaveVertMap* aTempMap = (tInterleaveVertMap *)MALLOC( iCount * sizeof( tInterleaveVertMap ) );
    
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
            tInterleaveVertMap* pPtr = &aTempMap[iTrimmed];
            tInterleaveVertMap* pVBOPtr = &pModel->maVBOVertPtrs[i];
            
            pPtr->miPos = pVBOPtr->miPos;
            pPtr->miNorm = pVBOPtr->miNorm;
            pPtr->miUV = pVBOPtr->miUV;
            
            ++iTrimmed;
        }
        
    }   // for i = 0 to total vbo vertices
    
    pModel->miNumVBOVerts = iCount - iNumDelete;
    WTFASSERT2( iTrimmed == pModel->miNumVBOVerts, "didn't trim vbo vert right" );
    
    // copy vbo verts with the new index info
    tInterleaveVert* aTrimmedVBOVerts = (tInterleaveVert *)malloc( pModel->miNumVBOVerts * sizeof( tInterleaveVert ) );
    tInterleaveVert* pVBOVert = aTrimmedVBOVerts;
    for( int i = 0; i < iTrimmed; i++ )
    {
        tInterleaveVertMap* pPtr = &aTempMap[i];
        
        tVector4 const* pPos = &pModel->maPos[pPtr->miPos];
        tVector4 const* pNorm = &pModel->maNorm[pPtr->miNorm];
        tVector2 const* pUV = &pModel->maUV[pPtr->miUV];
        
        memcpy( &pVBOVert->mPos, pPos, sizeof( tVector4 ) );
        memcpy( &pVBOVert->mNorm, pNorm, sizeof( tVector4 ) );
        memcpy( &pVBOVert->mUV, pUV, sizeof( tVector2 ) );
        
        ++pVBOVert;
        
    }   // for i = 0 to num trimmed
    
    // set the vbo face indices with trimmed vertices
    //pModel->maVBOVertPtrs = (tInterleaveVertMap *)REALLOC( pModel->maVBOVertPtrs, sizeof( tInterleaveVertMap ) * pModel->miNumVBOVerts );
    FREE( pModel->maVBOVertPtrs );
	pModel->maVBOVertPtrs = aTempMap;
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
                tInterleaveVert* pV = &aTrimmedVBOVerts[k];
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
    
	WTFASSERT2( iCount == pModel->miNumVBOIndices, "not equal num vbo indices" );

    FREE( pModel->maVBOVerts );
    pModel->maVBOVerts = aTrimmedVBOVerts;
    
	for( int i = 0; i < pModel->miNumVBOVerts; i++ )
	{
		memcpy( &pModel->maVBOVerts[i].mOrigPos,
				&pModel->maVBOVerts[i].mPos,
				sizeof( tVector4 ) );

		memcpy( &pModel->maVBOVerts[i].mOrigNorm,
				&pModel->maVBOVerts[i].mNorm,
				sizeof( tVector4 ) );
	
		pModel->maVBOVerts[i].mColor.fX = 1.0f;
		pModel->maVBOVerts[i].mColor.fY = 1.0f;
		pModel->maVBOVerts[i].mColor.fZ = 1.0f;
		pModel->maVBOVerts[i].mColor.fW = 1.0f;
	}

    //FREE( aTempMap );
    FREE( aiToDelete );
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
