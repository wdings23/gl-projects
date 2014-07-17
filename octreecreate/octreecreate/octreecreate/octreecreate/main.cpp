// octreecreate.cpp : Defines the entry point for the console application.
//
#include "fileutil.h"
#include "tinyxml.h"
#include "factory.h"
#include "model.h"
#include "modelinstance.h"
#include "octgrid.h"
#include "parseutil.h"

#define WTFASSERT2( X, ... ) assert( X )

static void makeOctGrid( const char* szFileName );
static void loadModels( tModel** apModels, TiXmlNode* pNode );
static void loadModelInstances( tModel** apModels, int iNumModels, tModelInstance* aModelInstances, TiXmlNode* pRoot );
static bool isModelInOctNode( tOctNode const* pOctNode, void* pObject );
static bool nodeIntersect( tOctNode const* pOctNode, tVector4 const* pPt0, tVector4 const* pPt1, tVector4 const* pPt2 );

CFactory<tModel>                gModelFactory;
CFactory<tModelInstance>		gModelInstanceFactory;

/*
 **
 */
int main(int argc, char* argv[])
{
	// extract directory
	char szDir[256];
	memset( szDir, 0, sizeof( szDir ) );
	
	char szFileName[256];
	memset( szFileName, 0, sizeof( szFileName ) );
	
	char szNoExtension[256];
	memset( szNoExtension, 0, sizeof( szNoExtension ) );
    
	const char* pszFile = argv[1];
	size_t iStrLen = strlen( pszFile );
	for( int i = (int)iStrLen - 1; i >= 0; i-- )
	{
		if( pszFile[i] == '/' || pszFile[i] == '\\' )
		{
			size_t iLength = ( iStrLen - i ) + 1;
			strncpy( szDir, pszFile, i );
			strncpy( szFileName, &pszFile[i+1], iLength );
            
			size_t iFileLen = strlen( szFileName );
			for( int i = (int)iFileLen - 1; i >= 0; i-- )
			{
				if( szFileName[i] == '.' )
				{
					memcpy( szNoExtension, szFileName, i );
					break;
				}
			}
            
			break;
		}
	}
    
	setFileDirectory( szDir );
    
	char szBinaryFileName[256];
	sprintf_s( szBinaryFileName, sizeof( szFileName ), "%s_octgrid.oct", szNoExtension );
    
	makeOctGrid( szFileName );
	
	//char szGridName[256];
	//sprintf_s( szGridName, sizeof( szGridName ), "%s_octgrid.cac", szNoExtension );
	//loadOctGrid( szGridName, szFileName );
	
	return 0;
}

/*
 **
 */
static void makeOctGrid( const char* szFileName )

{
	char szFullPath[256];
    getFullPath( szFullPath, szFileName );
    TiXmlDocument doc( szFullPath );
    
	char szNoExtension[128];
	memset( szNoExtension, 0, sizeof( szNoExtension ) );
	size_t iStrLen = strlen( szFileName );
	for( int i = (int)iStrLen - 1; i >= 0; i-- )
	{
		if( szFileName[i] == '.' )
		{
			memcpy( szNoExtension, szFileName, i );
			break;
		}
	}
    
	tOctGrid grid;
	tOctGrid* pGrid = &grid;
    
    bool bLoaded = doc.LoadFile();
    if( bLoaded )
    {
        // get the number of models
        int iNumModels = 0;
        TiXmlNode* pMeshNode = doc.FirstChild()->FirstChild( "model" );
        while( pMeshNode )
        {
            ++iNumModels;
            pMeshNode = pMeshNode->NextSibling( "model" );
        }
        
        int iNumAnimModels = 0;
        TiXmlNode* pAnimModelNode = doc.FirstChild()->FirstChild( "animmodel" );
        while( pAnimModelNode )
        {
            ++iNumAnimModels;
            pAnimModelNode = pAnimModelNode->NextSibling( "animmodel" );
        }
        
        int iNumAnimModelInstances = 0;
        int iNumModelInstances = 0;
        
        TiXmlNode* pModelInstanceNode = doc.FirstChild()->FirstChild( "instance" );
        while( pModelInstanceNode )
        {
            // check if it's a animation model
            TiXmlNode* pHierarchy = pModelInstanceNode->FirstChild( "animmodel" );
            if( pHierarchy )
            {
                ++iNumAnimModelInstances;
            }
            else
            {
                ++iNumModelInstances;
            }
            
            pModelInstanceNode = pModelInstanceNode->NextSibling( "instance" );
        }
        
        // add to factory
        tModel* aModels = gModelFactory.alloc( iNumModels );
		
        // load the models
        TiXmlNode* pRootNode = doc.FirstChild();
        
		tModel** apModels = (tModel **)malloc( sizeof( tModel* ) * iNumModels );
		memset( apModels, 0, sizeof( tModel* ) * iNumModels );
		for( int i = 0; i < iNumModels; i++ )
		{
			apModels[i] = &aModels[i];
		}
        
		tModelInstance* aModelInstances = (tModelInstance *)malloc( sizeof( tModelInstance ) * iNumModelInstances );
        
        loadModels( apModels, pRootNode );
		loadModelInstances( apModels, iNumModels, aModelInstances, pRootNode );
        
		// find the extent of the grid
		tVector4 largest = { -9999.0f, -9999.0f, -9999.0f, 1.0f };
		tVector4 smallest = { 9999.0f, 9999.0f, 9999.0f, 1.0f };
		for( int i = 0; i < iNumModelInstances; i++ )
		{
			tModelInstance* pModelInstance = &aModelInstances[i];
			tVector4 pos =
			{
				pModelInstance->mXFormMat.M( 0, 3 ),
				pModelInstance->mXFormMat.M( 1, 3 ),
				pModelInstance->mXFormMat.M( 2, 3 ),
				1.0f
			};
			
			float fRadius = pModelInstance->mfRadius;
			if( pos.fX - fRadius < smallest.fX )
			{
				smallest.fX = pos.fX - fRadius;
			}
            
			if( pos.fY - fRadius < smallest.fY )
			{
				smallest.fY = pos.fY - fRadius;
			}
            
			if( pos.fZ - fRadius < smallest.fZ )
			{
				smallest.fZ = pos.fZ - fRadius;
			}
            
			if( pos.fX + fRadius > largest.fX )
			{
				largest.fX = pos.fX + fRadius;
			}
            
			if( pos.fY + fRadius > largest.fY )
			{
				largest.fY = pos.fY + fRadius;
			}
            
			if( pos.fZ + fRadius > largest.fZ )
			{
				largest.fZ = pos.fZ + fRadius;
			}
            
		}	// for i = 0 to num model instances
        
		tVector4 center;
		Vector4Add( &center, &largest, &smallest );
		Vector4MultScalar( &center, &center, 0.5f );
        
		tVector4 size;
		Vector4Subtract( &size, &largest, &smallest );
		
		float fGridSize = size.fX;
		if( size.fY > fGridSize && size.fY > size.fZ )
		{
			fGridSize = size.fY;
		}
		else if( size.fZ > fGridSize && size.fZ > size.fY )
		{
			fGridSize = size.fZ;
		}
        
		// up to next size
		int iNodeSize = 10;
		int iGridSize = (int)ceilf( fGridSize );
		int iTotalSize = (int)ceilf( (float)iGridSize / (float)iNodeSize );
        
        octGridInit( pGrid, &center, (float)( iTotalSize * iNodeSize ), (float)iNodeSize );
        
		// calculate xform'd bbox
		tVector4 xform = { 0.0f, 0.0f, 0.0f, 1.0f };
		for( int i = 0; i < iNumModelInstances; i++ )
		{
			tModelInstance* pModelInstance = &aModelInstances[i];
			tModel const* pModel = pModelInstance->mpModel;
			tMatrix44* pMat = &pModelInstance->mXFormMat;
            
			tVector4 smallest = { 9999.0f, 9999.0f, 9999.0f, 1.0f };
			tVector4 largest = { -9999.0f, -9999.0f, -9999.0f, 1.0f };
            
			for( int j = 0; j < pModel->miNumVerts; j++ )
			{
				tVector4* pPos = &pModel->maPos[j];
				Matrix44Transform( &xform, pPos, pMat );
				
				if( xform.fX < smallest.fX )
				{
					smallest.fX = xform.fX;
				}
				if( xform.fY < smallest.fY )
				{
					smallest.fY = xform.fY;
				}
				if( xform.fZ < smallest.fZ )
				{
					smallest.fZ = xform.fZ;
				}
                
				if( xform.fX > largest.fX )
				{
					largest.fX = xform.fX;
				}
				if( xform.fY > largest.fY )
				{
					largest.fY = xform.fY;
				}
				if( xform.fZ > largest.fZ )
				{
					largest.fZ = xform.fZ;
				}
                
			}	// for j = 0 to num vertices
            
			memcpy( &pModelInstance->mLargest, &largest, sizeof( tVector4 ) );
			memcpy( &pModelInstance->mSmallest, &smallest, sizeof( tVector4 ) );
            
		}	// for i = 0 to num model instances
        
        // add model instance to grid
        for( int i = 0; i < iNumModelInstances; i++ )
        {
			printf( "add model %d/%d to octgrid\n", i, iNumModelInstances );
            
            tModelInstance* pModelInstance = &aModelInstances[i];
			octGridAddObject( pGrid,
                             pModelInstance,
                             isModelInOctNode );
        }
		
		char szFileName[256];
		sprintf_s( szFileName, sizeof( szFileName ), "%s_octgrid.cac", szNoExtension );
		octGridSave( pGrid, szFileName );
        
		char szBinaryFileName[256];
		sprintf_s( szBinaryFileName, sizeof( szFileName ), "%s_octgrid.oct", szNoExtension );
		octGridSaveBinary( pGrid, szBinaryFileName );
        
		free( apModels );
		free( aModelInstances );
    }
    else
    {
        printf( "error loading %s : %s\n", szFileName, doc.ErrorDesc() );
        WTFASSERT2( bLoaded, "can't load %s", szFileName );
    }
}

/*
 **
 */
static void loadModels( tModel** apModels, TiXmlNode* pNode )
{
    int iModel = 0;
    TiXmlNode* pModelNode = pNode->FirstChild( "model" );
    while( pModelNode )
    {
        const char* szFileName = pModelNode->FirstChild( "file" )->FirstChild()->Value();
        
        tModel* pModel = apModels[iModel];
        modelInit( pModel );
        modelLoad( pModel, szFileName );
        
        const char* szModelName = pModelNode->FirstChild( "name" )->FirstChild()->Value();
        strncpy( pModel->mszName, szModelName, 256 );
        
        pModelNode = pModelNode->NextSibling( "model" );
        ++iModel;
    }
}

/*
 **
 */
static void loadModelInstances( tModel** apModels, int iNumModels, tModelInstance* aModelInstances, TiXmlNode* pRoot )
{
    int iModelInstance = 0;
    
    TiXmlNode* pInstanceNode = pRoot->FirstChild( "instance" );
    while( pInstanceNode )
    {
        // not a model instance
        if( pInstanceNode->FirstChild( "model" ) == NULL )
        {
            pInstanceNode = pInstanceNode->NextSibling( "instance" );
            continue;
        }
        
        // look for the model this one is instance of
        const char* szModelName = pInstanceNode->FirstChild( "model" )->FirstChild()->Value();
        tModel* pFoundModel = NULL;
        for( int i = 0; i < iNumModels; i++ )
        {
            tModel* pModel = apModels[i];
            if( !strcmp( pModel->mszName, szModelName ) )
            {
                pFoundModel = pModel;
                break;
            }
        }   // for i = 0 to num models
        
        const char* szName = pInstanceNode->FirstChild( "name" )->FirstChild()->Value();
        
        // initialize the model instance
        WTFASSERT2( pFoundModel, "can't find model %s for instance", szModelName );
        tModelInstance* pModelInstance = &aModelInstances[iModelInstance];
        modelInstanceInit( pModelInstance );
        modelInstanceSet( pModelInstance, pFoundModel );
        
        strncpy( pModelInstance->mszName, szName, sizeof( pModelInstance->mszName ) );
        
#if 0
        TiXmlNode* pXFormNode = pInstanceNode->FirstChild( "xform" );
        
        int iNumXForms = 0;
        getNumXForms( pXFormNode, &iNumXForms );
        
        pModelInstance->maPositions = (tVector4 *)malloc( sizeof( tVector4 ) * iNumXForms );
        pModelInstance->maRotations = (tVector4 *)malloc( sizeof( tVector4 ) * iNumXForms );
        pModelInstance->maScalings = (tVector4 *)malloc( sizeof( tVector4 ) * iNumXForms );
        pModelInstance->maScalePivot = (tVector4 *)malloc( sizeof( tVector4 ) * iNumXForms );
        pModelInstance->maRotatePivot = (tVector4 *)malloc( sizeof( tVector4 ) * iNumXForms );
        pModelInstance->miNumXForms = iNumXForms;
        
        loadXForms( pModelInstance, 0, pXFormNode );
        
        modelInstanceUpdateXForm( pModelInstance );
#endif // #if 0
        
		int iNumEntries = 0;
		TiXmlNode* pMatrixNode = pInstanceNode->FirstChild( "matrix" );
        
		float* afEntries = NULL;
		parseFloatArray( &afEntries, &iNumEntries, pMatrixNode->FirstChild()->Value() );
        
		tMatrix44 temp;
		memcpy( temp.afEntries, afEntries, sizeof( float ) * 16 );
		Matrix44Transpose( &pModelInstance->mXFormMat, &temp );
		free( afEntries );
        
		tVector4 dimension = { 0.0f, 0.0f, 0.0f, 1.0f };
		memcpy( &dimension, &pFoundModel->mDimension, sizeof( tVector4 ) );
		dimension.fX *= pModelInstance->mXFormMat.M( 0, 0 );
		dimension.fY *= pModelInstance->mXFormMat.M( 0, 0 );
		dimension.fZ *= pModelInstance->mXFormMat.M( 0, 0 );
        
		pModelInstance->mfRadius = dimension.fX;
		if( dimension.fY > pModelInstance->mfRadius )
		{
			pModelInstance->mfRadius = dimension.fY;
		}
        
		if( dimension.fZ > pModelInstance->mfRadius )
		{
			pModelInstance->mfRadius = dimension.fZ;
		}
        
        
#if 0
		// radius of the model instance applied with xform matrix
		tModel const* pModel = pModelInstance->mpModel;
		tVector4 newDimension;
		memcpy( &newDimension, &pModel->mDimension, sizeof( tVector4 ) );
        
		newDimension.fX *= pModelInstance->mXFormMat.M( 0, 0 );
		newDimension.fY *= pModelInstance->mXFormMat.M( 1, 1 );
		newDimension.fZ *= pModelInstance->mXFormMat.M( 2, 2 );
        
		if( newDimension.fX > newDimension.fY &&
           newDimension.fX > newDimension.fZ )
		{
			pModelInstance->mfRadius = newDimension.fX * 0.5f;
		}
		else if( newDimension.fY > newDimension.fX &&
                newDimension.fY > newDimension.fZ )
		{
			pModelInstance->mfRadius = newDimension.fY * 0.5f;
		}
		else
		{
			pModelInstance->mfRadius = newDimension.fZ * 0.5f;
		}
#endif // #if 0
        
        pInstanceNode = pInstanceNode->NextSibling( "instance" );
        ++iModelInstance;
    }
}

/*
 **
 */
static bool isModelInOctNode( tOctNode const* pOctNode, void* pObject)
{
    bool bRet = false;
    
    tModelInstance* pModelInstance = (tModelInstance *)pObject;
    
    tVector4 const* pPos = pOctNode->mpCenter;
    float fSize = pOctNode->mfSize;
    
    // extent of the octnode
    float fX0 = pPos->fX - fSize * 0.5f;
    float fX1 = pPos->fX + fSize * 0.5f;
    
    float fY0 = pPos->fY - fSize * 0.5f;
    float fY1 = pPos->fY + fSize * 0.5f;
    
    float fZ0 = pPos->fZ - fSize * 0.5f;
    float fZ1 = pPos->fZ + fSize * 0.5f;
    
	// check if bbox is in node
	bool bCoordX = ( ( pModelInstance->mSmallest.fX >= fX0 && pModelInstance->mSmallest.fX <= fX1 ) ||
                    ( pModelInstance->mLargest.fX >= fX0 && pModelInstance->mLargest.fX <= fX1 ) ||
                    ( fX0 >= pModelInstance->mSmallest.fX && fX0 <= pModelInstance->mLargest.fX ) ||
                    ( fX1 >= pModelInstance->mSmallest.fX && fX1 <= pModelInstance->mLargest.fX ) );
    
	bool bCoordY = ( ( pModelInstance->mSmallest.fY >= fY0 && pModelInstance->mSmallest.fY <= fY1 ) ||
                    ( pModelInstance->mLargest.fY >= fY0 && pModelInstance->mLargest.fY <= fY1 ) ||
                    ( fY0 >= pModelInstance->mSmallest.fY && fY0 <= pModelInstance->mLargest.fY ) ||
                    ( fY1 >= pModelInstance->mSmallest.fY && fY1 <= pModelInstance->mLargest.fY ) );
    
	bool bCoordZ = ( ( pModelInstance->mSmallest.fZ >= fZ0 && pModelInstance->mSmallest.fZ <= fZ1 ) ||
                    ( pModelInstance->mLargest.fZ >= fZ0 && pModelInstance->mLargest.fZ <= fZ1 ) ||
                    ( fZ0 >= pModelInstance->mSmallest.fZ && fZ0 <= pModelInstance->mLargest.fZ ) ||
                    ( fZ1 >= pModelInstance->mSmallest.fZ && fZ1 <= pModelInstance->mLargest.fZ ) );
	if( !bCoordX || !bCoordY || !bCoordZ  )
	{
		return false;
	}
    
    // check if any vertex are in the node
    tVector4 const* aPos = pModelInstance->mpModel->maPos;
    for( int i = 0; i < pModelInstance->mpModel->miNumVerts; i++ )
    {
        // transform to world position
        tVector4 xformPos;
        Matrix44Transform( &xformPos, aPos, &pModelInstance->mXFormMat );
        
        if( xformPos.fX >= fX0 && xformPos.fX <= fX1 &&
           xformPos.fY >= fY0 && xformPos.fY <= fY1 &&
           xformPos.fZ >= fZ0 && xformPos.fZ <= fZ1 )
        {
            bRet = true;
            break;
        }
        
        ++aPos;
        
    }   // for i = 0 to num verts
    
    int iNumFaces = pModelInstance->mpModel->miNumFaces;
    tFace const* aFaces = pModelInstance->mpModel->maFaces;
    aPos = pModelInstance->mpModel->maPos;
    
    // check edges on the faces
    if( !bRet )
    {
        for( int i = 0; i < iNumFaces; i++ )
        {
            tVector4 const* pPt0 = &aPos[aFaces->maiV[0]];
            tVector4 const* pPt1 = &aPos[aFaces->maiV[1]];
            tVector4 const* pPt2 = &aPos[aFaces->maiV[2]];
            
            // transform to world position
            tVector4 xformPos0, xformPos1, xformPos2;
            Matrix44Transform( &xformPos0, pPt0, &pModelInstance->mXFormMat );
            Matrix44Transform( &xformPos1, pPt1, &pModelInstance->mXFormMat );
            Matrix44Transform( &xformPos2, pPt2, &pModelInstance->mXFormMat );
            
            bRet = nodeIntersect( pOctNode, &xformPos0, &xformPos1, &xformPos2 );
            if( bRet )
            {
                break;
            }
            
            ++aFaces;
        }
    }
    
    return bRet;
}

/*
 **
 */
static bool nodeIntersect( tOctNode const* pOctNode, tVector4 const* pPt0, tVector4 const* pPt1, tVector4 const* pPt2 )
{
    tVector4 const* pPos = pOctNode->mpCenter;
    float fSize = pOctNode->mfSize;
    
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
