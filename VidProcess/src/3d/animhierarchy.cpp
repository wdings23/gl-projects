//
//  animhierarchy.cpp
//  animtest
//
//  Created by Tony Peng on 6/24/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "animhierarchy.h"
#include "joint.h"
#include "tinyxml.h"
#include "filepathutil.h"

static void sortJoints( tAnimHierarchy* pAnimHierarchy );
static void findNodeLevel( tAnimHierarchy* pAnimHierarchy );
static void sortJointByLevel( tAnimHierarchy* pAnimHierarchy );

/*
**
*/
void animHierarchyInit( tAnimHierarchy* pAnimHierarchy )
{
    pAnimHierarchy->mpRoot = pAnimHierarchy->maJoints = NULL;
    pAnimHierarchy->miNumJoints = 0;
}


/*
**
*/
void animHierarchyLoad( tAnimHierarchy* pAnimHierarchy,
                        const char* szFileName,
                        CFactory<tJoint>* pJointFactory,
                        CFactory<tMatrix44>* pMatrixFactory )
{
	char szFullPath[256];
	getFullPath( szFullPath, szFileName );
    
	TiXmlDocument doc( szFullPath );
	bool bLoaded = doc.LoadFile();
	assert( bLoaded );
    
	if( bLoaded )
	{
		TiXmlNode* pChild = doc.FirstChild( "animmodel" )->FirstChild( "joint" );
		
		// number of joints
		pAnimHierarchy->miNumJoints = 0;
		while( pChild )
		{
			++pAnimHierarchy->miNumJoints;
			pChild = pChild->NextSibling( "joint" );
		}
		
		pAnimHierarchy->maJoints = pJointFactory->alloc( pAnimHierarchy->miNumJoints );
		TiXmlNode* pNode = doc.FirstChild( "animmodel" )->FirstChild( "joint" );
        tJoint* pJoint = &pAnimHierarchy->maJoints[0];
		memset( pAnimHierarchy->maJoints, 0, sizeof( tJoint ) * pAnimHierarchy->miNumJoints );
		
		int iJoint = 0;
		while( pNode )
		{
			pChild = pNode->FirstChild();
			while( pChild )
			{
				const char* szVal = pChild->Value();
				if( !strcmp( szVal, "properties" ) )
				{
					TiXmlNode* pNameNode = pChild->FirstChild();
					const char* szName = pNameNode->FirstChild()->Value();
	                
					assert( iJoint < pAnimHierarchy->miNumJoints );
					pJoint = &pAnimHierarchy->maJoints[iJoint++];
	                
					if( iJoint == 1 )
					{
						pAnimHierarchy->mpRoot = pJoint;
					}
	                
					size_t iNameSize = strlen( szName ) + 1;
					pJoint->mszName = (char *)MALLOC( iNameSize );
					strncpy( pJoint->mszName, szName, iNameSize );
				}
				else if( !strcmp( szVal, "data" ) )
				{
					tVector4 orientation = { 0.0f, 0.0f, 0.0f, 1.0f };
					tVector4 pos = { 0.0f, 0.0f, 0.0f, 1.0f };
					tVector4 rot = { 0.0f, 0.0f, 0.0f, 1.0f };
					tVector4 scale = { 1.0f, 1.0f, 1.0f, 1.0f };
					const char* szParentName = NULL;
	                
					TiXmlNode* pDataNode = pChild->FirstChild();
					while( pDataNode )
					{
						const char* szValue = pDataNode->Value();
						if( !strcmp( szValue, "orientation" ) )
						{
							TiXmlElement* pElement = pDataNode->ToElement();
							const char* szNum = pElement->Attribute( "x" );
							orientation.fX = (float)atof( szNum );
							orientation.fX *= ( 3.14159f / 180.0f );
	                        
							szNum = pElement->Attribute( "y" );
							orientation.fY = (float)atof( szNum );
							orientation.fY *= ( 3.14159f / 180.0f );
	                        
							szNum = pElement->Attribute( "z" );
							orientation.fZ = (float)atof( szNum );
							orientation.fZ *= ( 3.14159f / 180.0f );
						}
						else if( !strcmp( szValue, "translation" ) )
						{
							TiXmlElement* pElement = pDataNode->ToElement();
							const char* szNum = pElement->Attribute( "x" );
							pos.fX = (float)atof( szNum );
							
							szNum = pElement->Attribute( "y" );
							pos.fY = (float)atof( szNum );
							
							szNum = pElement->Attribute( "z" );
							pos.fZ = (float)atof( szNum );
						}
						else if( !strcmp( szValue, "rotation" ) )
						{
							TiXmlElement* pElement = pDataNode->ToElement();
							const char* szNum = pElement->Attribute( "x" );
							rot.fX = (float)atof( szNum );
							
							szNum = pElement->Attribute( "y" );
							rot.fY = (float)atof( szNum );
							
							szNum = pElement->Attribute( "z" );
							rot.fZ = (float)atof( szNum );
						}
						else if( !strcmp( szValue, "scale" ) )
						{
							TiXmlElement* pElement = pDataNode->ToElement();
							const char* szNum = pElement->Attribute( "x" );
							scale.fX = (float)atof( szNum );
							
							szNum = pElement->Attribute( "y" );
							scale.fY = (float)atof( szNum );
							
							szNum = pElement->Attribute( "z" );
							scale.fZ = (float)atof( szNum );
						}
						else if( !strcmp( szValue, "parents" ) )
						{
							TiXmlElement* pElement = pDataNode->ToElement();
							szParentName = pElement->Attribute( "parent0" );
						}
	                    
						pDataNode = pDataNode->NextSibling();
					}	// while data node
					
					jointSetParent( pJoint, pAnimHierarchy, szParentName );
					jointInit( pJoint,
							   pMatrixFactory,
							   &orientation,
							   &pos,
							   &scale );
	                
				}	// if node value = "data"
	            
				pChild = pChild->NextSibling();
			}	// while valid child node

			pNode = pNode->NextSibling( "joint" );

		}	// while valid joint node
	}
    else
    {
        OUTPUT( "%s", doc.ErrorDesc() );
    }
    
    sortJoints( pAnimHierarchy );
    findNodeLevel( pAnimHierarchy );
    sortJointByLevel( pAnimHierarchy );
    
    /*for( int i = 0; i < pAnimHierarchy->miNumJoints; i++ )
    {
        tJoint* pJoint = &pAnimHierarchy->maJoints[i];
        if( pJoint->mpParent )
        {
            OUTPUT( "%d %s parent = %s level = %d\n", i, pJoint->mszName, pJoint->mpParent->mszName, pJoint->miLevel );
        }
        else
        {
            OUTPUT( "%d %s level = %d\n", i, pJoint->mszName, pJoint->miLevel );
        }
    }*/
}

/*
**
*/
void animHierarchyCopy( tAnimHierarchy* pAnimHierarchy,
                        tAnimHierarchy const* pOrig,
                        CFactory<struct Joint>* pJointFactory,
                        CFactory<tMatrix44>* pMatrixFactory )
{
    int iNumJoints = pOrig->miNumJoints;
    pAnimHierarchy->maJoints = pJointFactory->alloc( iNumJoints );
    memset( pAnimHierarchy->maJoints, 0, sizeof( tJoint ) * iNumJoints );

    tJoint* aJoints = pAnimHierarchy->maJoints;
    tJoint* aOrigJoints = pOrig->maJoints;
    for( int i = 0; i < iNumJoints; i++ )
    {
        tJoint* pJoint = &aJoints[i];
        tJoint* pOrigJoint = &aOrigJoints[i];
        
        // name
        size_t iNameSize = strlen( pOrigJoint->mszName ) + 1;
        pJoint->mszName = (char *)MALLOC( sizeof( char ) * iNameSize );
        memset( pJoint->mszName, 0, iNameSize );
        strncpy( pJoint->mszName, pOrigJoint->mszName, iNameSize );
        
        // children
        pJoint->miNumChildren = pOrigJoint->miNumChildren;
        pJoint->miNumChildrenAlloc = pJoint->miNumChildren;
        pJoint->mapChildren = (tJoint **)MALLOC( sizeof( tJoint* ) * pJoint->miNumChildren );
        
        // matrices
        tMatrix44* aMatrices = pMatrixFactory->alloc( 7 );
        
        pJoint->mpLocalMatrix = &aMatrices[0];
        pJoint->mpSkinMatrix = &aMatrices[1];
        pJoint->mpTotalMatrix = &aMatrices[2];
        pJoint->mpPoseMatrix = &aMatrices[3];
        pJoint->mpInvPoseMatrix = &aMatrices[4];
        pJoint->mpInvTransSkinningMatrix = &aMatrices[5];
        pJoint->mpAnimMatrix = &aMatrices[6];
        
        memcpy( pJoint->mpLocalMatrix, pOrigJoint->mpLocalMatrix, sizeof( tMatrix44 ) );
        memcpy( pJoint->mpSkinMatrix, pOrigJoint->mpSkinMatrix, sizeof( tMatrix44 ) );
        memcpy( pJoint->mpTotalMatrix, pOrigJoint->mpTotalMatrix, sizeof( tMatrix44 ) );
        memcpy( pJoint->mpPoseMatrix, pOrigJoint->mpPoseMatrix, sizeof( tMatrix44 ) );
        memcpy( pJoint->mpInvPoseMatrix, pOrigJoint->mpInvPoseMatrix, sizeof( tMatrix44 ) );
        memcpy( pJoint->mpInvTransSkinningMatrix, pOrigJoint->mpInvTransSkinningMatrix, sizeof( tMatrix44 ) );
        memcpy( pJoint->mpAnimMatrix, pOrigJoint->mpAnimMatrix, sizeof( tMatrix44 ) );
        
        // level
        pJoint->miLevel = pOrigJoint->miLevel;
        
    }   // for i = 0 to num joints
    
    // set parents
    for( int i = 0; i < iNumJoints; i++ )
    {
        tJoint* pJoint = &aJoints[i];
        tJoint* pOrigJoint = &aOrigJoints[i];
        
        if( pOrigJoint->mpParent )
        {
            const char* szName = pOrigJoint->mpParent->mszName;
            for( int j = 0; j < iNumJoints; j++ )
            {
                tJoint* pCheckJoint = &aJoints[j];
                if( !strcmp( pCheckJoint->mszName, szName ) )
                {
                    pJoint->mpParent = pCheckJoint;
                    break;
                }
            }
        }
        
    }   // for i = 0 to num joints
    
    // set children
    for( int i = 0; i < iNumJoints; i++ )
    {
        tJoint* pJoint = &aJoints[i];
        tJoint* pOrigJoint = &aOrigJoints[i];
        
        int iNumChildren = pOrigJoint->miNumChildren;
        for( int j = 0; j < iNumChildren; j++ )
        {
            const char* szChildName = pOrigJoint->mapChildren[j]->mszName;
            
            bool bFound = false;
            for( int k = 0; k < iNumJoints; k++ )
            {
                if( !strcmp( aJoints[k].mszName, szChildName ) )
                {
                    pJoint->mapChildren[j] = &aJoints[k];
                    bFound = true;
                    break;
                }
                
            }   // for k = 0 to num joints
            
            WTFASSERT2( bFound, "didn't find child joint" );
            
        }   // for j = 0 to num children
        
    }   // for i = 0 to num joints
    
    // root
    const char* szRootName = pOrig->mpRoot->mszName;
    for( int i = 0; i < iNumJoints; i++ )
    {
        if( !strcmp( aJoints[i].mszName, szRootName ) )
        {
            pAnimHierarchy->mpRoot = &aJoints[i];
            break;
        }
    }
    
    pAnimHierarchy->miNumJoints = pOrig->miNumJoints;
    pAnimHierarchy->miMaxJointLevel = pOrig->miMaxJointLevel;
}

/*
**
*/
void animHierarchyPrintMatrices( tAnimHierarchy const* pAnimHierarchy )
{
    for( int i = 0; i < pAnimHierarchy->miNumJoints; i++ )
    {
        tJoint* pJoint = &pAnimHierarchy->maJoints[i];
        
        OUTPUT( "******** %s *********\n", pJoint->mszName );
        OUTPUT( "pose matrix\n" );
        Matrix44Print( pJoint->mpPoseMatrix );
        
        OUTPUT( "total matrix\n" );
        Matrix44Print( pJoint->mpTotalMatrix );
        
        OUTPUT( "skin matrix\n" );
        Matrix44Print( pJoint->mpSkinMatrix );
        
        OUTPUT( "anim matrix\n" );
        Matrix44Print( pJoint->mpAnimMatrix );
    }
}

/*
**
*/
static void sortJoints( tAnimHierarchy* pAnimHierarchy )
{
    int iNumJoints = pAnimHierarchy->miNumJoints;
    int iCurrIndex = 1;
    
    char** aszParentName = (char **)MALLOC( sizeof( char* ) * iNumJoints );
    for( int i = 0; i < iNumJoints; i++ )
    {
        aszParentName[i] = (char *)MALLOC( sizeof( char ) * 64 );
        memset( aszParentName[i], 0, sizeof( char ) * 64 );
        if( pAnimHierarchy->maJoints[i].mpParent )
        {
            strncpy( aszParentName[i], pAnimHierarchy->maJoints[i].mpParent->mszName, 64 );
        }
    }
    
    for( int i = 0; i < iNumJoints; i++ )
    {
        tJoint const* pJoint = &pAnimHierarchy->maJoints[i];
        
        // check for children
        for( int j = i + 1; j < iNumJoints; j++ )
        {
            tJoint* pCurrJoint = &pAnimHierarchy->maJoints[j];
        
            if( pCurrJoint->mpParent == pJoint )
            {
                tJoint* pSwapJoint = &pAnimHierarchy->maJoints[iCurrIndex];
                
                char* szName0 = aszParentName[j];
                char* szName1 = aszParentName[iCurrIndex];
                char szTemp[64];
                memcpy( szTemp, szName0, sizeof( char ) * 64 );
                memcpy( szName0, szName1, sizeof( char ) * 64 );
                memcpy( szName1, szTemp, sizeof( char ) * 64 );
                
                // swap
                tJoint temp;
                memcpy( &temp, pSwapJoint, sizeof( tJoint ) );
                memcpy( pSwapJoint, pCurrJoint, sizeof( tJoint ) );
                memcpy( pCurrJoint, &temp, sizeof( tJoint ) );
                
                if( iCurrIndex + 1 < iNumJoints )
                {
                    ++iCurrIndex;
                }
                else
                {
                    // done
                    break;
                }
                
            }   // if current joint parent == joint
            
        }   // for j = i + 1 to num joints
    
    }   // for i = 0 to num joints
    
    // reset parent
    for( int i = 0; i < iNumJoints; i++ )
    {
        const char* szParent = aszParentName[i];
        tJoint* pJoint = &pAnimHierarchy->maJoints[i];
        for( int j = 0; j < iNumJoints; j++ )
        {
            if( i == j )
            {
                continue;
            }
            
            if( !strcmp( szParent, pAnimHierarchy->maJoints[j].mszName ) )
            {
                pJoint->mpParent = &pAnimHierarchy->maJoints[j];
                break;
            }
        }   // for j = 0 to num joints
        
    }   // for i = 0 to num joints
    
    for( int i = 0; i < iNumJoints; i++ )
    {
        FREE( aszParentName[i] );
    }
    
    FREE( aszParentName );
}

/*
**
*/
static void sortJointByLevel( tAnimHierarchy* pAnimHierarchy )
{
    int iNumJoints = pAnimHierarchy->miNumJoints;
    
    char** aszParentNames = (char **)MALLOC( sizeof( char* ) * iNumJoints );
    char** aszNames = (char **)MALLOC( sizeof( char* ) * iNumJoints );
    for( int i = 0; i < iNumJoints; i++ )
    {
        tJoint* pJoint = &pAnimHierarchy->maJoints[i];
    
        aszParentNames[i] = (char *)MALLOC( sizeof( char ) * 64 );
        memset( aszParentNames[i], 0, sizeof( char ) * 64 );
        
        aszNames[i] = (char *)MALLOC( sizeof( char ) * 64 );
        memset( aszNames[i], 0, sizeof( char ) * 64 );
        
        strncpy( aszNames[i], pJoint->mszName, 64 );
        if( pJoint->mpParent )
        {
            strncpy( aszParentNames[i], pJoint->mpParent->mszName, 64 );
        }
    }
    
    for( int i = 0; i < iNumJoints; i++ )
    {
        tJoint const* pJoint = &pAnimHierarchy->maJoints[i];
        for( int j = i + 1; j < iNumJoints; j++ )
        {
            tJoint* pCurrJoint = &pAnimHierarchy->maJoints[j];
            if( pCurrJoint->miLevel < pJoint->miLevel )
            {
                tJoint temp;
                memcpy( &temp, &pAnimHierarchy->maJoints[i], sizeof( tJoint ) );
                memcpy( &pAnimHierarchy->maJoints[i], &pAnimHierarchy->maJoints[j], sizeof( tJoint ) );
                memcpy( &pAnimHierarchy->maJoints[j], &temp, sizeof( tJoint ) );
            }
            
        }   // for j = i + 1 to num joints
        
    }   // for i = 0 to num joints

    // rebuild parents
    for( int i = 0; i < iNumJoints; i++ )
    {
        tJoint* pJoint = &pAnimHierarchy->maJoints[i];
        int iIndex = 0;
        for( iIndex = 0; iIndex < iNumJoints; iIndex++ )
        {
            if( !strcmp( pJoint->mszName, aszNames[iIndex] ) )
            {
                break;
            }
        }
        
        WTFASSERT2( iIndex < iNumJoints, "array out of bounds" );
        
        const char* szParentName = aszParentNames[iIndex];
        for( int j = 0; j < iNumJoints; j++ )
        {
            if( !strcmp( szParentName, pAnimHierarchy->maJoints[j].mszName ) )
            {
                pJoint->mpParent = &pAnimHierarchy->maJoints[j];
                break;
            }
        }
    }
    
    for( int i = 0; i < iNumJoints; i++ )
    {
        FREE( aszParentNames[i] );
        FREE( aszNames[i] );
    }
    
    FREE( aszParentNames );
    FREE( aszNames );
}

/*
**
*/
static void findNodeLevel( tAnimHierarchy* pAnimHierarchy )
{
    pAnimHierarchy->miMaxJointLevel = 0;
    
    int iNumJoints = pAnimHierarchy->miNumJoints;
    for( int i = 0; i < iNumJoints; i++ )
    {
        tJoint* pJoint = &pAnimHierarchy->maJoints[i];
        pJoint->miLevel = 0;
        
        tJoint* pParent = pJoint->mpParent;
        while( pParent )
        {
            pParent = pParent->mpParent;
            ++pJoint->miLevel;
        }
        
        if( pAnimHierarchy->miMaxJointLevel < pJoint->miLevel )
        {
            pAnimHierarchy->miMaxJointLevel = pJoint->miLevel;
        }
        
    }   // for i = 0 to num joints
}