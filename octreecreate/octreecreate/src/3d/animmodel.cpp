//
//  animmodel.cpp
//  animtest
//
//  Created by Tony Peng on 7/19/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "animmodel.h"

#include "tinyxml.h"
#include "fileutil.h"

#include "animhierarchy.h"
#include "model.h"
#include <vector>


#define MAX_JOINT_INFLUENCES 2

static int readVerts( TiXmlNode* pRoot, int iNumTotalVerts, tVector4* aPos, tAnimModel* pAnimModel );
static int readUV( TiXmlNode* pRoot, int iNumTotalUV, tVector2* aUV, tAnimModel const* pAnimModel );
static int readFace( TiXmlNode* pRoot,
                    int iNumTotalFaces,
                    int iNumTotalVert,
                    int iNumTotalUV,
                    tFace* aFaces,
                    tVector4* aNormals,
                    tAnimModel const* pAnimModel );

static int readSkinCluster( TiXmlNode* pRoot,
                            int iMesh,
                            tAnimHierarchy const* pAnimHierarchy,
                            int iNumTotalVert,
                            float* afBlendVal,
                            tJoint** apJoints,
                            std::vector<const char*>* paNames,
                            tAnimModel const* pAnimModel );

static bool sameVec( tVector4 const* pV0, tVector4 const* pV1 );
static bool sameVec2( tVector2 const* pV0, tVector2 const* pV1 );

static void consolidateNormals( tAnimModel* pAnimModel );
static void createVBOVerts( tAnimModel* pAnimModel );

/*
**
*/
void animModelInit( tAnimModel* pAnimModel )
{
    pAnimModel->mafBlendVals = NULL;
    pAnimModel->maNorm = NULL;
    pAnimModel->maPos = NULL;
    pAnimModel->maUV = NULL;
    pAnimModel->maVBOVerts = NULL;
    pAnimModel->maiJointIndices = NULL;
    
    pAnimModel->miNumBlendVals = 0;
    pAnimModel->miNumVerts = 0;
    pAnimModel->miNumVBOVerts = 0;
}

/*
**
*/
void animModelRelease( tAnimModel* pAnimModel )
{
    free( pAnimModel->mafBlendVals );
    free( pAnimModel->maNorm );
    free( pAnimModel->maPos );
    free( pAnimModel->maUV );
    free( pAnimModel->maVBOVerts );
    free( pAnimModel->maiVBOIndices );
    free( pAnimModel->maiJointIndices );
    free( pAnimModel->maiNormalJointIndices );
    free( pAnimModel->mafNormalBlendVals );
    
    pAnimModel->miNumBlendVals = 0;
    pAnimModel->miNumVerts = 0;
    pAnimModel->miNumVBOVerts = 0;
}

/*
**
*/
void animModelLoad( tAnimModel* pAnimModel,
                    tAnimHierarchy const* pAnimHierarchy,
                    const char* szFileName )
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
        //TiXmlNode* pSkinClusterNode = pRoot->FirstChild( "skinCluster" );
        
        // count number of entries
        int iTotalVerts = 0;
        int iTotalUV = 0;
        int iTotalFaces = 0;
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
            
            TiXmlNode* pNumFaceNode = pFaceInfoNode->FirstChild( "properties" )->FirstChild( "size" );
            szData = pNumFaceNode->FirstChild()->Value();
            int iNumFaces = atoi( szData );
            
            iTotalVerts += iNumVerts;
            iTotalFaces += iNumFaces;
            iTotalUV += iNumUV;
            
//OUTPUT( "number of vert=%d total vert=%d\n", iNumVerts, iTotalVerts );
            
            pMeshNode = pMeshNode->NextSibling( "mesh" );
            pFaceInfoNode = pFaceInfoNode->NextSibling( "faceInfo" );
            
        }   // while still reading in mesh
        
        // allocate info
        pAnimModel->maPos = (tVector4 *)malloc( sizeof( tVector4 ) * iTotalVerts );
        pAnimModel->maNorm = (tVector4 *)malloc( sizeof( tVector4 ) * iTotalFaces * 3 );
        pAnimModel->maUV = (tVector2 *)malloc( sizeof( tVector2 ) * iTotalUV );
        pAnimModel->maFaces = (tFace *)malloc( sizeof( tFace ) * iTotalFaces );
        
        pAnimModel->maXFormPos = (tVector4 *)malloc( sizeof( tVector4 ) * iTotalVerts );
        pAnimModel->maXFormNorm = (tVector4 *)malloc( sizeof( tVector4 ) * iTotalFaces * 3 );
        
        pAnimModel->miNumVerts = iTotalVerts;
        pAnimModel->miNumFaces = iTotalFaces;
        pAnimModel->miNumUV = iTotalUV;
        pAnimModel->miNumNormals = iTotalFaces * 3;
        
//OUTPUT( "total verts = %d\n", iTotalVerts );
//OUTPUT( "total faces = %d\n", iTotalFaces );
//OUTPUT( "total uv = %d\n", iTotalUV );
        
        pMeshNode = pRoot->FirstChild( "mesh" );
        pFaceInfoNode = pRoot->FirstChild( "faceInfo" );
        
        // read in info
        int iCurrTotalVerts = 0;
        int iCurrTotalUV = 0;
        int iCurrTotalFaces = 0;
        while( pMeshNode )
        {
            int iNumVerts = readVerts( pMeshNode, iCurrTotalVerts, pAnimModel->maPos, pAnimModel );
            int iNumUV = readUV( pMeshNode, iCurrTotalUV, pAnimModel->maUV, pAnimModel );
            int iNumFaces = readFace( pFaceInfoNode,
                                      iCurrTotalFaces,
                                      iCurrTotalVerts,
                                      iCurrTotalUV,
                                      pAnimModel->maFaces,
                                      pAnimModel->maNorm,
                                      pAnimModel );
            
            iCurrTotalVerts += iNumVerts;
            iCurrTotalUV += iNumUV;
            iCurrTotalFaces += iNumFaces;
         
//OUTPUT( "number of vert=%d total vert=%d\n", iNumVerts, iCurrTotalVerts );
            
            WTFASSERT2( iCurrTotalVerts <= iTotalVerts, "too many verts" );
            WTFASSERT2( iCurrTotalUV <= iTotalUV, "too many uv" );
            WTFASSERT2( iCurrTotalFaces <= iTotalFaces, "too many faces" );
            
            pMeshNode = pMeshNode->NextSibling( "mesh" );
            pFaceInfoNode = pFaceInfoNode->NextSibling( "faceInfo" );
            
        }   // while still reading in mesh
        
        // skin cluster joint blend
        pAnimModel->miNumBlendVals = pAnimModel->miNumVerts * MAX_JOINT_INFLUENCES;
        pAnimModel->mafBlendVals = (float *)malloc( sizeof( float ) * pAnimModel->miNumVerts * MAX_JOINT_INFLUENCES );
        pAnimModel->mapInfluenceJoints = (tJoint **)malloc( sizeof( tJoint* ) * pAnimModel->miNumVerts * MAX_JOINT_INFLUENCES );
        pAnimModel->maiJointIndices = (int *)malloc( sizeof( int ) * pAnimModel->miNumVerts * MAX_JOINT_INFLUENCES );
        
        memset( pAnimModel->mapInfluenceJoints, 0, sizeof( tJoint* ) * pAnimModel->miNumVerts * MAX_JOINT_INFLUENCES );
        
        int iTotalVert = 0;
        for( int i = 0; i < aNames.size(); i++ )
        {
            int iIndex = readSkinCluster( pRoot,
                                          i,
                                          pAnimHierarchy,
                                          iTotalVert,
                                          pAnimModel->mafBlendVals,
                                          pAnimModel->mapInfluenceJoints,
                                          &aNames,
                                          pAnimModel );
            
            iTotalVert = iIndex;
        }
        
        // get the corresponding joint indices
        for( int i = 0; i < pAnimModel->miNumVerts * MAX_JOINT_INFLUENCES; i++ )
        {
            tJoint* pJoint = pAnimModel->mapInfluenceJoints[i];
            for( int j = 0; j < pAnimHierarchy->miNumJoints; j++ )
            {
                if( !strcmp( pJoint->mszName, pAnimHierarchy->maJoints[j].mszName ) )
                {
                    pAnimModel->maiJointIndices[i] = j;
                    break;
                }
                
            }   // for j = 0 to num joints
            
        }   // for i = 0 to total verts
        
        pAnimModel->mpAnimHierarchy = pAnimHierarchy;
       
    }
    else
    {
        WTFASSERT2( bLoaded, "can't load %s", szFileName );
    }
    
    consolidateNormals( pAnimModel );
    createVBOVerts( pAnimModel );
}

/*
**
*/
static int readVerts( TiXmlNode* pRoot, int iNumTotalVerts, tVector4* aPos, tAnimModel* pAnimModel )
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
        WTFASSERT2( iIndex < pAnimModel->miNumVerts, "too many vertices" );
        
        tVector4* pPos = &aPos[iIndex];
        pPos->fX = (float)atof( szX );
        pPos->fY = (float)atof( szY );
        pPos->fZ = (float)atof( szZ );
        pPos->fW = 1.0f;
        
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
    
    pAnimModel->mDimension.fX = maxPos.fX - minPos.fX;
    pAnimModel->mDimension.fY = maxPos.fY - minPos.fY;
    pAnimModel->mDimension.fZ = maxPos.fZ - minPos.fZ;
    pAnimModel->mDimension.fW = 1.0f;
    
    pAnimModel->mCenter.fX = ( maxPos.fX + minPos.fX ) * 0.5f;
    pAnimModel->mCenter.fY = ( maxPos.fY + minPos.fY ) * 0.5f;
    pAnimModel->mCenter.fZ = ( maxPos.fZ + minPos.fZ ) * 0.5f;
    pAnimModel->mCenter.fW = 1.0f;
    
    return iNumVerts;
}

/*
**
*/
static int readUV( TiXmlNode* pRoot, int iNumTotalUV, tVector2* aUV, tAnimModel const* pAnimModel )
{
    int iNumUV = 0;
    TiXmlNode* pUVNode = pRoot->FirstChild( "data" )->FirstChild( "textureCoordinate" );
    while( pUVNode )
    {
        TiXmlElement* pElement = pUVNode->ToElement();
        const char* szX = pElement->Attribute( "u" );
        const char* szY = pElement->Attribute( "v" );
        
        int iIndex = iNumTotalUV + iNumUV;
        WTFASSERT2( iIndex < pAnimModel->miNumUV, "too many uv" );
        
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
static int readFace( TiXmlNode* pRoot,
                     int iNumTotalFaces,
                     int iNumTotalVert,
                     int iNumTotalUV,
                     tFace* aFaces,
                     tVector4* aNormals,
                     tAnimModel const* pAnimModel )
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
            WTFASSERT2( iNIndex < pAnimModel->miNumFaces * 3, "too many normals" );
            tVector4* pNormal = &aNormals[iNIndex];
            
            TiXmlElement* pElement = pVNode->ToElement();
            int iV = atoi( pElement->Attribute( "vertexID" ) );
            int iUV = atoi( pElement->Attribute( "texture" ) );
            
            pFace->maiNorm[iVIndex] = iNIndex;
            pFace->maiV[iVIndex] = iV + iNumTotalVert;
            pFace->maiUV[iVIndex] = iUV + iNumTotalUV;
            
            pNormal->fX = (float)atof( pElement->Attribute( "normalx" ) );
            pNormal->fY = (float)atof( pElement->Attribute( "normaly" ) );
            pNormal->fZ = (float)atof( pElement->Attribute( "normalz" ) );
            pNormal->fW = 1.0f;
            
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
static int readSkinCluster( TiXmlNode* pRoot,
                            int iMesh,
                            tAnimHierarchy const* pAnimHierarchy,
                            int iNumTotalVert,
                            float* afBlendVal,
                            tJoint** apJoints,
                            std::vector<const char*>* paNames,
                            tAnimModel const* pAnimModel )
{
    int iNumVert = 0;
    int iPrevVertIndex = -1;
    
    float   afVertInfluences[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    tJoint* apVertJoints[4] = { NULL, NULL, NULL, NULL };
    
    int iCurrV = 0;
    int iIndex = iNumTotalVert;
    
    const char* szMeshName = (*paNames)[iMesh];
//OUTPUT( "%s skinCluster\n", szMeshName );
    
    // get the corresponding skincluster for the mesh
    TiXmlNode* pSkinClusterNode = NULL;
    TiXmlNode* pFindNode = pRoot->FirstChild( "skinCluster" );
    do {
        const char* szSkinMeshName = pFindNode->FirstChild( "properties" )->FirstChild( "meshName" )->FirstChild()->Value();
        if( !strcmp( szSkinMeshName, szMeshName ) )
        {
            pSkinClusterNode = pFindNode;
            break;
        }
        
        pFindNode = pFindNode->NextSibling( "skinCluster" );
    } while( true );
    
    WTFASSERT2( pSkinClusterNode, "can't find skin for mesh: %s", szMeshName );
    
    TiXmlNode* pInfluenceNode = pSkinClusterNode->FirstChild( "data" )->FirstChild( "influence" );
    
    int iNumInfluences = 0;
    while( pInfluenceNode )
    {
        TiXmlElement* pElement = pInfluenceNode->ToElement();
        int iV = atoi( pElement->Attribute( "vertID" ) );
        const char* szJointName = pElement->Attribute( "name" );
        float fWeight = (float)atof( pElement->Attribute( "weight" ) );
        
//OUTPUT( "v %d joint = %s weight = %f\n", iV, szJointName, fWeight );
        
        // new vertex
        if( iPrevVertIndex != -1 &&
            iPrevVertIndex != iV )
        {
            // normalize the weights
            float fTotalInfluences = 0.0f;
            for( int i = 0; i < MAX_JOINT_INFLUENCES; i++ )
            {
                fTotalInfluences += afVertInfluences[i];
            }
            
            for( int i = 0; i < MAX_JOINT_INFLUENCES; i++ )
            {
                afVertInfluences[i] /= fTotalInfluences;
            }
            
            // just want the first number of clamp influences
            for( int i = 0; i < MAX_JOINT_INFLUENCES; i++ )
            {
                WTFASSERT2( iIndex < pAnimModel->miNumVerts * MAX_JOINT_INFLUENCES, "too many blend values" );
                afBlendVal[iIndex] = afVertInfluences[i];
                
                // just set it to the previous joint
                if( apVertJoints[i] == NULL )
                {
                    apVertJoints[i] = apVertJoints[0];
                    WTFASSERT2( apVertJoints[i], "not valid joint" );
                }
                
                apJoints[iIndex] = apVertJoints[i];
                
//OUTPUT( "%d joint = %s blend: %f\n", iIndex, apJoints[iIndex]->mszName, afBlendVal[iIndex] );
                ++iIndex;
            }
            
            iCurrV = 0;
            ++iNumVert;
            iPrevVertIndex = iV;
            
            iNumInfluences = 0;
            memset( afVertInfluences, 0, sizeof( afVertInfluences ) );
            memset( apVertJoints, 0, sizeof( apVertJoints ) );
        }
        
        // count as influence on this vertex
        ++iNumInfluences;
        
        WTFASSERT2( iCurrV < 4, "array out of bounds" );
        afVertInfluences[iCurrV] = fWeight;
        
        // look for joint
        tJoint* pJoint = NULL;
        for( int i = 0; i < pAnimHierarchy->miNumJoints; i++ )
        {
            if( !strcmp( pAnimHierarchy->maJoints[i].mszName, szJointName ) )
            {
                pJoint = &pAnimHierarchy->maJoints[i];
                break;
            }
        }
        
        WTFASSERT2( pJoint, "can't find joint %s", szJointName );
        apVertJoints[iCurrV] = pJoint;
        
        if( iPrevVertIndex == -1 )
        {
            iPrevVertIndex = iV;
            ++iCurrV;
        }
        else
        {
            ++iCurrV;
        }
        
        pInfluenceNode = pInfluenceNode->NextSibling( "influence" );
    }
    
    // normalize the weights
    float fTotalInfluences = 0.0f;
    for( int i = 0; i < MAX_JOINT_INFLUENCES; i++ )
    {
        fTotalInfluences += afVertInfluences[i];
    }
    
    for( int i = 0; i < MAX_JOINT_INFLUENCES; i++ )
    {
        afVertInfluences[i] /= fTotalInfluences;
    }
    
    // just want the first number of clamp influences
    for( int i = 0; i < MAX_JOINT_INFLUENCES; i++ )
    {
        WTFASSERT2( iIndex < pAnimModel->miNumVerts * MAX_JOINT_INFLUENCES, "too many blend values" );
        afBlendVal[iIndex] = afVertInfluences[i];
        
        // just set it to the previous joint
        if( apVertJoints[i] == NULL )
        {
            apVertJoints[i] = apVertJoints[0];
            WTFASSERT2( apVertJoints[i], "not valid joint" );
        }
        
        apJoints[iIndex] = apVertJoints[i];
        
//OUTPUT( "%d joint = %s blend: %f\n", iIndex, apJoints[iIndex]->mszName, afBlendVal[iIndex] );
        ++iIndex;
    }
    
    iCurrV = 0;
    ++iNumVert;
    
//OUTPUT( "skinCluster verts = %d index = %d\n", iNumVert, iIndex );
    
    return iIndex;
}

/*
**
*/
void animModelUpdate( tAnimModel* pAnimModel )
{
    int iNumVerts = pAnimModel->miNumVerts;
    for( int i = 0; i < iNumVerts; i++ )
    {
        tVector4* pXForm = &pAnimModel->maXFormPos[i];
        tVector4* pPos = &pAnimModel->maPos[i];
        
        int iIndex = ( i << 1 );
        float fPct0 = pAnimModel->mafBlendVals[iIndex];
        float fPct1 = pAnimModel->mafBlendVals[iIndex+1];
        
        tJoint* pJoint0 = pAnimModel->mapInfluenceJoints[iIndex];
        tJoint* pJoint1 = pAnimModel->mapInfluenceJoints[iIndex+1];
        
        tVector4 xform0, xform1;
        Matrix44Transform( &xform0, pPos, pJoint0->mpSkinMatrix );
        Matrix44Transform( &xform1, pPos, pJoint1->mpSkinMatrix );
        
        Vector4MultScalar( &xform0, &xform0, fPct0 );
        Vector4MultScalar( &xform1, &xform1, fPct1 );
        
        Vector4Add( pXForm, &xform0, &xform1 );
        
        OUTPUT( "xform ( %f, %f, %f ) orig ( %f, %f, %f )",
                pXForm->fX,
                pXForm->fY,
                pXForm->fZ,
                pPos->fX,
                pPos->fY,
                pPos->fZ );
        
    }   // for i = 0 to num vertices
}

/*
**
*/
static void consolidateNormals( tAnimModel* pAnimModel )
{
    int iNumFaces = pAnimModel->miNumFaces;
    int iNumNormals = iNumFaces * 3;
    
    tVector4* aUnique = (tVector4 *)malloc( iNumFaces * 3 * sizeof( tVector4 ) );
    int iNumUniques = 0;

    /*OUTPUT( "*** BEFORE ***\n" );
    for( int i = 0; i < iNumFaces; i++ )
    {
        tFace* pFace = &pAnimModel->maFaces[i];
        OUTPUT( "face %d ( %.2f, %.2f, %.2f ) ( %.2f, %.2f, %.2f ) ( %.2f, %.2f, %.2f )\n",
                i,
                pAnimModel->maNorm[pFace->maiNorm[0]].fX,
                pAnimModel->maNorm[pFace->maiNorm[0]].fY,
                pAnimModel->maNorm[pFace->maiNorm[0]].fZ,
                pAnimModel->maNorm[pFace->maiNorm[1]].fX,
                pAnimModel->maNorm[pFace->maiNorm[1]].fY,
                pAnimModel->maNorm[pFace->maiNorm[1]].fZ,
                pAnimModel->maNorm[pFace->maiNorm[2]].fX,
                pAnimModel->maNorm[pFace->maiNorm[2]].fY,
                pAnimModel->maNorm[pFace->maiNorm[2]].fZ );
    }*/
    
    // get the unique normal positions
    for( int i = 0; i < iNumNormals; i++ )
    {
        bool bDuplicate = false;
        for( int j = 0; j < iNumUniques; j++ )
        {
            if( sameVec( &pAnimModel->maNorm[i], &aUnique[j] ) )
            {
                bDuplicate = true;
                break;
            }
        }
        
        // unique
        if( !bDuplicate )
        {
            memcpy( &aUnique[iNumUniques], &pAnimModel->maNorm[i], sizeof( tVector4 ) );
            ++iNumUniques;
        }
        
    }   // for i = 0 to num normals
    
    // use the unique normal for face indices
    for( int i = 0; i < iNumFaces; i++ )
    {
        tFace* pFace = &pAnimModel->maFaces[i];
        for( int j = 0; j < 3; j++ )
        {
            int iV = pFace->maiNorm[j];
            tVector4* pOldPos = &pAnimModel->maNorm[iV];
            
            bool bFound = false;
            for( int k = 0; k < iNumUniques; k++ )
            {
                tVector4* pPos = &aUnique[k];
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
    
    // set the normals to uniques
    free( pAnimModel->maNorm );
    pAnimModel->maNorm = aUnique;
    pAnimModel->miNumNormals = iNumUniques;
    
    tAnimHierarchy const* pAnimHierarchy = pAnimModel->mpAnimHierarchy;
    int iNumJoints = pAnimHierarchy->miNumJoints;
    
    pAnimModel->maiNormalJointIndices = (int *)malloc( sizeof( int ) * iNumUniques * MAX_JOINT_INFLUENCES );
    pAnimModel->mafNormalBlendVals = (float *)malloc( sizeof( float ) * iNumUniques * MAX_JOINT_INFLUENCES );
    pAnimModel->miNumNormalBlendVals = iNumUniques * MAX_JOINT_INFLUENCES;
    
    // find the influencing joint
    int iCount = 0;
    for( int i = 0; i < iNumUniques; i++ )
    {
        int j = 0;
        for( j = 0; j < iNumFaces; j++ )
        {
            // look for vertex in the same face as this normal
            tFace const* pFace = &pAnimModel->maFaces[j];
            
            bool bFound = false;
            for( int k = 0; k < 3; k++ )
            {
                if( pFace->maiNorm[k] == i )
                {
                    int iV = pFace->maiV[k];
                    
                    // get the associated joint for the number of influences
                    for( int m = 0; m < MAX_JOINT_INFLUENCES; m++ )
                    {
                        int iJointIndex = iV << 1;
                        WTFASSERT2( iJointIndex < pAnimModel->miNumVerts * MAX_JOINT_INFLUENCES, "array out of bounds looking for joint" );
                        tJoint const* pJoint = pAnimModel->mapInfluenceJoints[iJointIndex];
                        
                        for( int n = 0; n < iNumJoints; n++ )
                        {
                            if( !strcmp( pJoint->mszName, pAnimHierarchy->maJoints[n].mszName ) )
                            {
                                bFound = true;
                                pAnimModel->maiNormalJointIndices[iCount] = n;
                                WTFASSERT2( iJointIndex + m < pAnimModel->miNumVerts * MAX_JOINT_INFLUENCES, "array out of bounds looking for joint" );
                                
                                pAnimModel->mafNormalBlendVals[iCount] = pAnimModel->mafBlendVals[iJointIndex+m];
                                ++iCount;
                                break;
                            }
                            
                        }   // n = 0 to num joints
                        
                    }   // for m = 0 to max joint influences
                
                }   // if face norm index == i
                
                if( bFound )
                {
                    break;
                }
                
            }   // for k = 0 to 3
            
            if( bFound )
            {
                break;
            }
            
        }   // for j = 0 to num faces
        
        WTFASSERT2( j < iNumFaces, "didn't find coressponding face" );
        
    }   // for i = 0 to num uniques

    /*OUTPUT( "*** AFTER ***\n" );
    for( int i = 0; i < iNumFaces; i++ )
    {
        tFace* pFace = &pAnimModel->maFaces[i];
        OUTPUT( "face %d ( %.2f, %.2f, %.2f ) ( %.2f, %.2f, %.2f ) ( %.2f, %.2f, %.2f )\n",
               i,
               pAnimModel->maNorm[pFace->maiNorm[0]].fX,
               pAnimModel->maNorm[pFace->maiNorm[0]].fY,
               pAnimModel->maNorm[pFace->maiNorm[0]].fZ,
               pAnimModel->maNorm[pFace->maiNorm[1]].fX,
               pAnimModel->maNorm[pFace->maiNorm[1]].fY,
               pAnimModel->maNorm[pFace->maiNorm[1]].fZ,
               pAnimModel->maNorm[pFace->maiNorm[2]].fX,
               pAnimModel->maNorm[pFace->maiNorm[2]].fY,
               pAnimModel->maNorm[pFace->maiNorm[2]].fZ );
    }*/

}

/*
**
*/
static void createVBOVerts( tAnimModel* pAnimModel )
{
    //OUTPUT( "*** BEFORE ***\n" );
    /*for( int i = 0; i < pAnimModel->miNumFaces; i++ )
    {
        tFace* pFace = &pAnimModel->maFaces[i];
        
        tVector4* pPos0 = &pAnimModel->maPos[pFace->maiV[0]];
        tVector4* pNorm0 = &pAnimModel->maNorm[pFace->maiNorm[0]];
        tVector2* pUV0 = &pAnimModel->maUV[pFace->maiUV[0]];
        
        tVector4* pPos1 = &pAnimModel->maPos[pFace->maiV[1]];
        tVector4* pNorm1 = &pAnimModel->maNorm[pFace->maiNorm[1]];
        tVector2* pUV1 = &pAnimModel->maUV[pFace->maiUV[1]];
        
        tVector4* pPos2 = &pAnimModel->maPos[pFace->maiV[2]];
        tVector4* pNorm2 = &pAnimModel->maNorm[pFace->maiNorm[2]];
        tVector2* pUV2 = &pAnimModel->maUV[pFace->maiUV[2]];
         
        OUTPUT( "face %d\n0: pos ( %.2f, %.2f, %.2f ) norm ( %.2f, %.2f, %.2f ) uv( %.2f, %.2f )\n1: pos ( %.2f, %.2f, %.2f ) norm ( %.2f, %.2f, %.2f ) uv( %.2f, %.2f )\n2: pos ( %.2f, %.2f, %.2f ) norm ( %.2f, %.2f, %.2f ) uv( %.2f, %.2f )\n",
                i,
                pPos0->fX, pPos0->fY, pPos0->fZ,
                pNorm0->fX, pNorm0->fY, pNorm0->fZ,
                pUV0->fX, pUV0->fY,
               
                pPos1->fX, pPos1->fY, pPos1->fZ,
                pNorm1->fX, pNorm1->fY, pNorm1->fZ,
                pUV1->fX, pUV1->fY,
               
                pPos2->fX, pPos2->fY, pPos2->fZ,
                pNorm2->fX, pNorm2->fY, pNorm2->fZ,
                pUV2->fX, pUV2->fY );
    }
    */
    
    int iNumVBOVerts = pAnimModel->miNumVerts;
    
    pAnimModel->maiVBOIndices = (unsigned int *)malloc( sizeof( int ) * pAnimModel->miNumFaces * 3 );
    pAnimModel->miNumVBOVerts = iNumVBOVerts;
    pAnimModel->maVBOVerts = (tInterleaveVert *)malloc( sizeof( tInterleaveVert ) * pAnimModel->miNumFaces * 3 );
    pAnimModel->maVBOVertPtrs = (tInterleaveVertMap *)malloc( sizeof( tInterleaveVertMap ) * pAnimModel->miNumFaces * 3 );
    
    int iCurrVBOVerts = 0;
    int iCount = 0;
    
    int iNumFaces = pAnimModel->miNumFaces;
    for( int i = 0; i < iNumFaces; i++ )
    {
        tFace* pFace = &pAnimModel->maFaces[i];
        
        // look for position and normal corresponding to the uv
        for( int j = 0; j < 3; j++ )
        {
            int iUV = pFace->maiUV[j];
            int iNorm = pFace->maiNorm[j];
            int iV = pFace->maiV[j];
            
            pAnimModel->maiVBOIndices[iCount] = iCount;
            
            // new vertex
            WTFASSERT2( iCount < pAnimModel->miNumFaces * 3, "array out of bounds" );
            tInterleaveVert* pInterleaveVert = &pAnimModel->maVBOVerts[iCount];
            
            tVector4 const* pPos = &pAnimModel->maPos[iV];
            tVector4 const* pNorm = &pAnimModel->maNorm[iNorm];
            tVector2 const* pUV = &pAnimModel->maUV[iUV];
            
            memcpy( &pInterleaveVert->mPos, pPos, sizeof( tVector4 ) );
            memcpy( &pInterleaveVert->mNorm, pNorm, sizeof( tVector4 ) );
            memcpy( &pInterleaveVert->mUV, pUV, sizeof( tVector2 ) );
            
            tInterleaveVertMap* pPtr = &pAnimModel->maVBOVertPtrs[iCount];
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
        tVector4 const* pPos = &pAnimModel->maVBOVerts[i].mPos;
        tVector4 const* pNorm = &pAnimModel->maVBOVerts[i].mNorm;
        tVector2 const* pUV = &pAnimModel->maVBOVerts[i].mUV;
        
        bool bRepeat = false;
        for( int j = i + 1; j < iCount; j++ )
        {
            tVector4 const* pCheckPos = &pAnimModel->maVBOVerts[j].mPos;
            tVector4 const* pCheckNorm = &pAnimModel->maVBOVerts[j].mNorm;
            tVector2 const* pCheckUV = &pAnimModel->maVBOVerts[j].mUV;
            
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
            tInterleaveVert* pV = &pAnimModel->maVBOVerts[iTrimmed];
            memcpy( &pV->mPos, pPos, sizeof( tVector4 ) );
            memcpy( &pV->mNorm, pNorm, sizeof( tVector4 ) );
            memcpy( &pV->mUV, pUV, sizeof( tVector2 ) );
            
            tInterleaveVertMap* pPtr = &pAnimModel->maVBOVertPtrs[iTrimmed];
            pPtr->miPos = pAnimModel->maVBOVertPtrs[i].miPos;
            pPtr->miNorm = pAnimModel->maVBOVertPtrs[i].miNorm;
            pPtr->miUV = pAnimModel->maVBOVertPtrs[i].miUV;
            
            /*OUTPUT( "NON-REPEAT %d pos ( %.2f, %.2f, %.2f ) norm ( %.2f, %.2f, %.2f ) uv ( %.2f, %.2f )\n",
                    iTrimmed,
                    pV->mPos.fX, pV->mPos.fY, pV->mPos.fZ,
                    pV->mNorm.fX, pV->mNorm.fY, pV->mNorm.fZ,
                    pV->mUV.fX, pV->mUV.fY );*/
            
            ++iTrimmed;
        }
            
    }   // for i = 0 to total vbo vertices
    
    pAnimModel->miNumVBOVerts = iCount - iNumDelete;
    WTFASSERT2( iTrimmed == pAnimModel->miNumVBOVerts, "didn't trim vbo vert right" );
    tInterleaveVert* aTrimmedVBOVerts = (tInterleaveVert *)malloc( pAnimModel->miNumVBOVerts * sizeof( tInterleaveVert ) );
    
    pAnimModel->maVBOVertPtrs = (tInterleaveVertMap *)realloc( pAnimModel->maVBOVertPtrs, sizeof( tInterleaveVertMap ) * pAnimModel->miNumVBOVerts );
    
    // set the vbo face indices with trimmed vertices
    iCount = 0;
    for( int i = 0; i < iNumFaces; i++ )
    {
        tFace const* pFace = &pAnimModel->maFaces[i];
        for( int j = 0; j < 3; j++ )
        {
            int iV = pFace->maiV[j];
            int iUV = pFace->maiUV[j];
            int iNorm = pFace->maiNorm[j];
            
            tVector4* pPos = &pAnimModel->maPos[iV];
            tVector4* pNorm = &pAnimModel->maNorm[iNorm];
            tVector2* pUV = &pAnimModel->maUV[iUV];
            
            // determine if it's this vertex
            bool bFound = false;
            for( int k = 0; k < pAnimModel->miNumVBOVerts; k++ )
            {
                tInterleaveVert* pV = &pAnimModel->maVBOVerts[k];
                if( sameVec( pPos, &pV->mPos ) &&
                    sameVec( pNorm, &pV->mNorm ) &&
                    sameVec2( pUV, &pV->mUV ) )
                {
                    pAnimModel->maiVBOIndices[iCount++] = k;
                    bFound = true;
                    break;
                }
                
            }   // for k = 0 to num vbo verts
            
            /*OUTPUT( "%d LOOK FOR pos ( %.2f, %.2f, %.2f ) norm ( %.2f, %.2f, %.2f ) uv ( %.2f, %.2f )\n",
                    iCount,
                    pPos->fX, pPos->fY, pPos->fZ,
                    pNorm->fX, pNorm->fY, pNorm->fZ,
                    pUV->fX, pUV->fY );*/
            
            WTFASSERT2( bFound, "didn't find a matching vertex" );
            
        }   // for j = 0 to 3
        
    }   // for i = 0 to num faces
    
    //OUTPUT( "*** AFTER ***\n" );
    iCount = 0;
    for( int i = 0; i < pAnimModel->miNumFaces; i++ )
    {
#if 0
        int iV0 = pAnimModel->maiVBOIndices[iCount++];
        int iV1 = pAnimModel->maiVBOIndices[iCount++];
        int iV2 = pAnimModel->maiVBOIndices[iCount++];
    
        tVector4* pPos0 = &pAnimModel->maVBOVerts[iV0].mPos;
        tVector4* pNorm0 = &pAnimModel->maVBOVerts[iV0].mNorm;
        tVector2* pUV0 = &pAnimModel->maVBOVerts[iV0].mUV;
        
        tVector4* pPos1 = &pAnimModel->maVBOVerts[iV1].mPos;
        tVector4* pNorm1 = &pAnimModel->maVBOVerts[iV1].mNorm;
        tVector2* pUV1 = &pAnimModel->maVBOVerts[iV1].mUV;
        
        tVector4* pPos2 = &pAnimModel->maVBOVerts[iV2].mPos;
        tVector4* pNorm2 = &pAnimModel->maVBOVerts[iV2].mNorm;
        tVector2* pUV2 = &pAnimModel->maVBOVerts[iV2].mUV;
#endif // #if 0
        
        /*int iV0 = pAnimModel->maVBOVertPtrs[pAnimModel->maiVBOIndices[iCount]].miPos;
        int iN0 = pAnimModel->maVBOVertPtrs[pAnimModel->maiVBOIndices[iCount]].miNorm;
        int iT0 = pAnimModel->maVBOVertPtrs[pAnimModel->maiVBOIndices[iCount]].miUV;
        ++iCount;
        
        tVector4* pPos0 = &pAnimModel->maPos[iV0];
        tVector4* pNorm0 = &pAnimModel->maNorm[iN0];
        tVector2* pUV0 = &pAnimModel->maUV[iT0];
        
        int iV1 = pAnimModel->maVBOVertPtrs[pAnimModel->maiVBOIndices[iCount]].miPos;
        int iN1 = pAnimModel->maVBOVertPtrs[pAnimModel->maiVBOIndices[iCount]].miNorm;
        int iT1 = pAnimModel->maVBOVertPtrs[pAnimModel->maiVBOIndices[iCount]].miUV;
        ++iCount;
        
        tVector4* pPos1 = &pAnimModel->maPos[iV1];
        tVector4* pNorm1 = &pAnimModel->maNorm[iN1];
        tVector2* pUV1 = &pAnimModel->maUV[iT1];
        
        int iV2 = pAnimModel->maVBOVertPtrs[pAnimModel->maiVBOIndices[iCount]].miPos;
        int iN2 = pAnimModel->maVBOVertPtrs[pAnimModel->maiVBOIndices[iCount]].miNorm;
        int iT2 = pAnimModel->maVBOVertPtrs[pAnimModel->maiVBOIndices[iCount]].miUV;
        ++iCount;
        
        tVector4* pPos2 = &pAnimModel->maPos[iV2];
        tVector4* pNorm2 = &pAnimModel->maNorm[iN2];
        tVector2* pUV2 = &pAnimModel->maUV[iT2];
        
        
        OUTPUT( "face %d\n0: pos ( %.2f, %.2f, %.2f ) norm ( %.2f, %.2f, %.2f ) uv( %.2f, %.2f )\n1: pos ( %.2f, %.2f, %.2f ) norm ( %.2f, %.2f, %.2f ) uv( %.2f, %.2f )\n2: pos ( %.2f, %.2f, %.2f ) norm ( %.2f, %.2f, %.2f ) uv( %.2f, %.2f )\n",
               i,
               pPos0->fX, pPos0->fY, pPos0->fZ,
               pNorm0->fX, pNorm0->fY, pNorm0->fZ,
               pUV0->fX, pUV0->fY,
               
               pPos1->fX, pPos1->fY, pPos1->fZ,
               pNorm1->fX, pNorm1->fY, pNorm1->fZ,
               pUV1->fX, pUV1->fY,
               
               pPos2->fX, pPos2->fY, pPos2->fZ,
               pNorm2->fX, pNorm2->fY, pNorm2->fZ,
               pUV2->fX, pUV2->fY );*/
    }
    
    /*for( int i = 0; i < pAnimModel->miNumFaces * 3; i++ )
    {
        if( i % 3 == 0 )
        {
            OUTPUT( "%d ( ", i / 3 );
        }
        
        OUTPUT( "%d, ", pAnimModel->maiVBOIndices[i] );
        
        if( i % 3 == 2 )
        {
            OUTPUT( " )\n" );
        }
    }*/
    
    free( pAnimModel->maVBOVerts );
    pAnimModel->maVBOVerts = aTrimmedVBOVerts;
    
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
