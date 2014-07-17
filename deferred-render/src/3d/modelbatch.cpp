#include "modelbatch.h"
#include "model.h"
#include "factory.h"
#include "timeutil.h"

#define NUM_BATCH_VERT_PER_ALLOC	( 1<<16 )			

static tVector4* saTempPos = NULL;
static tVector4* saTempNorm = NULL;

/*
**
*/
void modelBatchInit( tModelBatch* pModelBatch )
{
	pModelBatch->miNumVertAlloc = 4;
	pModelBatch->miNumIndexAlloc = 4;

	//pModelBatch->maInterleaveVerts = (tInterleaveVert *)malloc( NUM_BATCH_VERT_PER_ALLOC * sizeof( tInterleaveVert ) );
	pModelBatch->maInterleaveVerts = (tInterleaveVert *)MALLOC( pModelBatch->miNumVertAlloc * sizeof( tInterleaveVert ) );
	WTFASSERT2( pModelBatch->maInterleaveVerts, "can't allocated interleave verts" );
	memset( pModelBatch->maInterleaveVerts, 0, sizeof( tInterleaveVert ) * pModelBatch->miNumIndexAlloc );

	pModelBatch->miNumVerts = 0;
	pModelBatch->maVertPos = NULL;
	pModelBatch->miNumModels = 0;

	//pModelBatch->maiIndices = (unsigned int *)malloc( NUM_BATCH_VERT_PER_ALLOC * sizeof( int ) );
	//pModelBatch->maVertPos = (tVector4 *)malloc( NUM_BATCH_VERT_PER_ALLOC * sizeof( tVector4 ) );

	pModelBatch->maiIndices = (unsigned int *)MALLOC( 4 * sizeof( int ) );
	pModelBatch->maVertPos = NULL;

	WTFASSERT2( pModelBatch->maiIndices, "can't allocated vbo indices" );
	
	for( int i = 0; i < sizeof( pModelBatch->maiDepthTextureID ) / sizeof( *pModelBatch->maiDepthTextureID ); i++ )
	{
		pModelBatch->maiDepthTextureID[i] = -1;
	}

	for( int i = 0; i < sizeof( pModelBatch->maiTextureID ) / sizeof( *pModelBatch->maiTextureID ); i++ )
	{
		pModelBatch->maiTextureID[i] = -1;
	}

	if( saTempPos == NULL )
	{	
		saTempPos = (tVector4 *)MALLOC( sizeof( tVector4 ) * NUM_BATCH_VERT_PER_ALLOC * 4 );
	}

	if( saTempNorm == NULL )
	{
		saTempNorm = (tVector4 *)MALLOC( sizeof( tVector4 ) * NUM_BATCH_VERT_PER_ALLOC );
	}
}

/*
**
*/
void modelBatchRelease( tModelBatch* pModelBatch )
{
	FREE( pModelBatch->maInterleaveVerts );
	FREE( pModelBatch->maiIndices );
	FREE( pModelBatch->maVertPos );

	memset( pModelBatch, 0, sizeof( tModelBatch ) );
}

/*
**
*/
double gTotalAddToBatchTime = 0.0;
bool modelBatchAddModelInstance( tModelBatchInstanceInfo const* pInfo )
{
//double fStart = getCurrTime();

	tModelBatch* pModelBatch = pInfo->mpModelBatch;
	tModelInstance const* pModelInstance = pInfo->mpModelInstance;
	tMatrix44 const* pXFormMatrix = pInfo->mpXFormMatrix;
	tMatrix44 const* pViewMatrix = pInfo->mpViewMatrix;
	tMatrix44 const* pViewProjMatrix = pInfo->mpViewProjMatrix;
	tMatrix44 const* aLightViewProjMatrices = pInfo->maLightViewProjMatrices;
	GLuint const* aiTextureID = pInfo->maiTextureID;
	GLuint const* aiDepthTextures = pInfo->maiDepthTextures;
	tVector4 const* pColor = pInfo->mpColor;
	
	tModel const* pModel = pModelInstance->mpModel;

	tMatrix44 modelViewMatrix;
	Matrix44Multiply( &modelViewMatrix, pViewMatrix, pXFormMatrix );

	// filled up
	if( pModelBatch->miNumIndices + pModel->miNumVBOIndices >= NUM_BATCH_VERT_PER_ALLOC )
	{
		return false;
	}
	
	if( aiDepthTextures )
	{
		for( int i = 0; i < NUM_DEPTH_TEXTURES_PER_BATCH; i++ )
		{
			pModelBatch->maiDepthTextureID[i] = aiDepthTextures[i];
		}
	}

	for( int i = 0; i < NUM_TEXTURES_PER_BATCH; i++ )
	{
		pModelBatch->maiTextureID[i] = aiTextureID[i];
	}

	// total data so far
	int iTotalVerts = pModelBatch->miNumVerts + pModel->miNumVBOVerts;
	int iTotalIndices = pModelBatch->miNumIndices + pModel->miNumVBOIndices;

	if( iTotalVerts > pModelBatch->miNumVertAlloc )
	{
		pModelBatch->maInterleaveVerts = (tInterleaveVert *)REALLOC( pModelBatch->maInterleaveVerts, sizeof( tInterleaveVert ) * iTotalVerts );
		WTFASSERT2( pModelBatch->maInterleaveVerts, "can't allocate interleave vertex" );
		pModelBatch->miNumVertAlloc = iTotalVerts;
	}

	if( iTotalIndices > pModelBatch->miNumIndexAlloc )
	{
		pModelBatch->maiIndices = (unsigned int *)REALLOC( pModelBatch->maiIndices, sizeof( int ) * iTotalIndices );
		WTFASSERT2( pModelBatch->maiIndices, "can't allocate indices" );
		pModelBatch->miNumIndexAlloc = iTotalIndices;
	}

	// model * view * proj matrix
	tMatrix44 totalMat;
	Matrix44Multiply( &totalMat, pViewProjMatrix, pXFormMatrix );
	
	// model * light view * light proj matrix
	tMatrix44 aTotalLightMatrices[3];
	if( aiDepthTextures )
	{
		for( int i = 0; i < NUM_DEPTH_TEXTURES_PER_BATCH; i++ )
		{
			if( aLightViewProjMatrices && aiDepthTextures[i] > 0 )
			{
				Matrix44Multiply( &aTotalLightMatrices[i], 
								  &aLightViewProjMatrices[i],
								  pXFormMatrix );
			}
		}
	}

	tVector4* pResultPos = saTempPos;
	tVector4 const* pModelVertPos = pModel->maPos;	
	
	// transform vertex positions
	for( int i = 0; i < pModel->miNumVerts; i++ )
	{
		Matrix44Transform( pResultPos, pModelVertPos, &totalMat );
		
		// transform position from light 
		if( aiDepthTextures )
		{
			for( int j = 0; j < NUM_DEPTH_TEXTURES_PER_BATCH; j++ )
			{
				int iIndex = ( j + 1 ) * NUM_BATCH_VERT_PER_ALLOC + i;
				if( aLightViewProjMatrices && aiDepthTextures[j] > 0 )
				{
					Matrix44Transform( &saTempPos[iIndex], pModelVertPos, &aTotalLightMatrices[j] );	
				}
			}
		}

		/*OUTPUT( "orig v %d ( %f, %f, %f )\n", 
				i,
				pModelVertPos->fX,
				pModelVertPos->fY,
				pModelVertPos->fZ );

		OUTPUT( "v %d ( %f, %f, %f )\n", 
				i, 
				pResultPos->fX / pResultPos->fW, 
				pResultPos->fY / pResultPos->fW, 
				pResultPos->fZ / pResultPos->fW ); */
		
		++pResultPos;
		++pModelVertPos;

	}	// for i = 0 to num vertices

	tVector4* pResultNorm = saTempNorm;
	tVector4 const* pModelNorm = pModel->maNorm;
	
	// transform normals
	for( int i = 0; i < pModel->miNumNormals; i++ )
	{
		Matrix44Transform( pResultNorm, pModelNorm, &pModelInstance->mRotationMatrix );
		pResultNorm->fW = 1.0f;

		++pResultNorm;
		++pModelNorm;
	}

	// update indices
	int iCurrIndexStart = pModelBatch->miNumIndices;
	int iCurrVertStart = pModelBatch->miNumVerts;
	unsigned int* aiResultIndices = &pModelBatch->maiIndices[iCurrIndexStart];
	WTFASSERT2( pModelBatch->miNumIndexAlloc >= pModel->miNumVBOIndices + iCurrIndexStart, "invalid index" );

	for( int i = 0; i < pModel->miNumVBOIndices; i++ )
	{
		*aiResultIndices = pModel->maiVBOIndices[i] + iCurrVertStart;
		WTFASSERT2( *aiResultIndices < (unsigned int)iTotalIndices, "index out of bounds" );

		++aiResultIndices;

	}	// for i = 0 to num vbo indices
	
	// copy the vbo verts from original
	WTFASSERT2( pModelBatch->miNumVertAlloc >= pModel->miNumVBOVerts + iCurrVertStart, "invalid index" );
	memcpy( &pModelBatch->maInterleaveVerts[iCurrVertStart],
			pModel->maVBOVerts,
			sizeof( tInterleaveVert ) * pModel->miNumVBOVerts );
	
	// update vbo vertex position with the xform
	tInterleaveVert* pResultVert = &pModelBatch->maInterleaveVerts[iCurrVertStart];
	for( int i = 0; i < pModel->miNumVBOVerts; i++ )
	{
		int iPosIndex = pModel->maVBOVertPtrs[i].miPos;
		memcpy( &pResultVert->mPos, 
				&saTempPos[iPosIndex],
				sizeof( tVector4 ) );  
		
		// light position
		if( aLightViewProjMatrices && aiDepthTextures )
		{
			for( int j = 0; j < NUM_DEPTH_TEXTURES_PER_BATCH; j++ )
			{
				int iIndex = ( j + 1 ) * NUM_BATCH_VERT_PER_ALLOC + iPosIndex;
				memcpy( &pResultVert->maLightPos[j],
						&saTempPos[iIndex],
						sizeof( tVector4 ) );
			}
		}

		int iNormIndex = pModel->maVBOVertPtrs[i].miNorm;
		memcpy( &pResultVert->mNorm,
				&saTempNorm[iNormIndex],
				sizeof( tVector4 ) );

		memcpy( &pResultVert->mOrigNorm,
				&saTempNorm[iNormIndex],
				sizeof( tVector4 ) );

		pResultVert->mColor.fW = 0.5f;

		++pResultVert;

	}	// for i = 0 to num vbo verts

	// TODO: OPTIMIZE THIS

	// transform vertex positions with only the model matrix
	pResultPos = saTempPos;
	pModelVertPos = pModel->maPos;	
	for( int i = 0; i < pModel->miNumVerts; i++ )
	{
		Matrix44Transform( pResultPos, pModelVertPos, pXFormMatrix );

		++pResultPos;
		++pModelVertPos;
	}

	tVector4 xformPos = { 0.0f, 0.0f, 0.0f, 1.0f };
	
	// copy over to orig vertex position
	pResultVert = &pModelBatch->maInterleaveVerts[iCurrVertStart];
	for( int i = 0; i < pModel->miNumVBOVerts; i++ )
	{
		int iPosIndex = pModel->maVBOVertPtrs[i].miPos;
		memcpy( &pResultVert->mOrigPos,
				&saTempPos[iPosIndex],
				sizeof( tVector4 ) );

		memcpy( &pResultVert->mColor, pColor, sizeof( tVector4 ) );
		
		// calculate projection texture uv
		if( pInfo->mpProjectionTextureMatrix )
		{
			Matrix44Transform( &xformPos, &saTempPos[iPosIndex], pInfo->mpProjectionTextureMatrix );
			float fOneOverW = 1.0f / xformPos.fW;
			pResultVert->maProjPos[0].fX = ( xformPos.fX * fOneOverW + 1.0f ) * 0.5f;
			pResultVert->maProjPos[0].fY = ( xformPos.fY * fOneOverW + 1.0f ) * 0.5f;
			pResultVert->maProjPos[0].fZ = ( xformPos.fZ * fOneOverW + 1.0f ) * 0.5f;
			pResultVert->maProjPos[0].fW = 1.0f;
		}

		++pResultVert;
	}
	
	pModelBatch->miNumPos += pModel->miNumVerts;
	pModelBatch->miNumVerts += pModel->miNumVBOVerts;
	pModelBatch->miNumIndices += pModel->miNumVBOIndices;
	++pModelBatch->miNumModels;

//double fCurrTime = getCurrTime();
//double fElapsed = fCurrTime - fStart;
//gTotalAddToBatchTime += fElapsed;

	return true;
}

/*
**
*/
bool modelBatchAddAnimModel( tModelBatchInstanceInfo const* pInfo )
{
	tModelBatch* pModelBatch = pInfo->mpModelBatch;
	tAnimModelInstance const* pAnimModelInstance = pInfo->mpAnimModelInstance;
	tMatrix44 const* pXFormMatrix = pInfo->mpXFormMatrix;
	tMatrix44 const* pViewProjMatrix = pInfo->mpViewProjMatrix;
	tMatrix44 const* aLightViewProjMatrices = pInfo->maLightViewProjMatrices;
	GLuint const* aiTextureID = pInfo->maiTextureID;
	GLuint const* aiDepthTextures = pInfo->maiDepthTextures;
	bool bBlend = pInfo->mbBlend;

    tAnimModel const* pAnimModel = pAnimModelInstance->mpAnimModel;
    

	// filled up
	if( pModelBatch->miNumIndices + pAnimModelInstance->miNumVBOIndices >= NUM_BATCH_VERT_PER_ALLOC )
	{
		return false;
	}
	
	if( aiDepthTextures )
	{
		for( int i = 0; i < NUM_DEPTH_TEXTURES_PER_BATCH; i++ )
		{
			pModelBatch->maiDepthTextureID[i] = aiDepthTextures[i];
		}
	}
    
	for( int i = 0; i < NUM_TEXTURES_PER_BATCH; i++ )
	{
		pModelBatch->maiTextureID[i] = aiTextureID[i];
	}
    
	// total data so far
	int iTotalVerts = pModelBatch->miNumVerts + pAnimModelInstance->miNumVBOVerts;
	int iTotalIndices = pModelBatch->miNumIndices + pAnimModelInstance->miNumVBOIndices;
    
	if( iTotalVerts > pModelBatch->miNumVertAlloc )
	{
		pModelBatch->maInterleaveVerts = (tInterleaveVert *)REALLOC( pModelBatch->maInterleaveVerts, sizeof( tInterleaveVert ) * iTotalVerts );
		WTFASSERT2( pModelBatch->maInterleaveVerts, "can't allocate interleave vertex" );
		pModelBatch->miNumVertAlloc = iTotalVerts;
	}
    
	if( iTotalIndices > pModelBatch->miNumIndexAlloc )
	{
		pModelBatch->maiIndices = (unsigned int *)REALLOC( pModelBatch->maiIndices, sizeof( int ) * iTotalIndices );
		WTFASSERT2( pModelBatch->maiIndices, "can't allocate indices" );
		pModelBatch->miNumIndexAlloc = iTotalIndices;
	}
    
	// model * view * proj matrix
	tMatrix44 totalMat;
	Matrix44Multiply( &totalMat, pViewProjMatrix, pXFormMatrix );
	
	// model * light view * light proj matrix
	tMatrix44 aTotalLightMatrices[3];
	if( aiDepthTextures )
	{
		for( int i = 0; i < NUM_DEPTH_TEXTURES_PER_BATCH; i++ )
		{
			if( aLightViewProjMatrices && aiDepthTextures[i] > 0 )
			{
				Matrix44Multiply( &aTotalLightMatrices[i],
                                 &aLightViewProjMatrices[i],
                                 pXFormMatrix );
			}
		}
	}
    

	tVector4* pResultPos = saTempPos;
	tVector4 const* pModelVertPos = pAnimModelInstance->maXFormPos;	
	
	// transform vertex positions
	for( int i = 0; i < pAnimModel->miNumVerts; i++ )
	{
		Matrix44Transform( pResultPos, pModelVertPos, &totalMat );
		
		// transform position from light 
		if( aiDepthTextures )
		{
			for( int j = 0; j < NUM_DEPTH_TEXTURES_PER_BATCH; j++ )
			{
				int iIndex = ( j + 1 ) * NUM_BATCH_VERT_PER_ALLOC + i;
				if( aLightViewProjMatrices && aiDepthTextures[j] > 0 )
				{
					Matrix44Transform( &saTempPos[iIndex], pModelVertPos, &aTotalLightMatrices[j] );	
				}
			}
		}

		++pResultPos;
		++pModelVertPos;

	}	// for i = 0 to num vertices
		

	tVector4* pResultNorm = saTempNorm;
	tVector4 const* pModelNorm = pAnimModelInstance->maXFormNorm;
	
	// transform normals
	for( int i = 0; i < pAnimModel->miNumNormals; i++ )
	{
		Matrix44Transform( pResultNorm, pModelNorm, &pAnimModelInstance->mRotationMatrix );
		pResultNorm->fW = 1.0f;
        
		++pResultNorm;
		++pModelNorm;
	}

	// update indices
	int iCurrIndexStart = pModelBatch->miNumIndices;
	int iCurrVertStart = pModelBatch->miNumVerts;
	unsigned int* aiResultIndices = &pModelBatch->maiIndices[iCurrIndexStart];
	WTFASSERT2( pModelBatch->miNumIndexAlloc >= pAnimModelInstance->miNumVBOIndices + iCurrIndexStart, "invalid index" );
    
	for( int i = 0; i < pAnimModelInstance->miNumVBOIndices; i++ )
	{
		*aiResultIndices = pAnimModelInstance->maiVBOIndices[i] + iCurrVertStart;
		WTFASSERT2( *aiResultIndices < (unsigned int)iTotalIndices, "index out of bounds" );
        
		++aiResultIndices;
        
	}	// for i = 0 to num vbo indices
	
	// copy the vbo verts from original
	WTFASSERT2( pModelBatch->miNumVertAlloc >= pAnimModel->miNumVBOVerts + iCurrVertStart, "invalid index" );
	memcpy( &pModelBatch->maInterleaveVerts[iCurrVertStart],
            pAnimModelInstance->maVBOVerts,
            sizeof( tInterleaveVert ) * pAnimModelInstance->miNumVBOVerts );
	
	// update vbo vertex position with the xform
	tInterleaveVert* pResultVert = &pModelBatch->maInterleaveVerts[iCurrVertStart];
	for( int i = 0; i < pAnimModelInstance->miNumVBOVerts; i++ )
	{
		int iPosIndex = pAnimModel->maVBOVertPtrs[i].miPos;
		memcpy( &pResultVert->mPos, 
				&saTempPos[iPosIndex],
				sizeof( tVector4 ) );  
		
		// light position
		if( aLightViewProjMatrices && aiDepthTextures )
		{
			for( int j = 0; j < NUM_DEPTH_TEXTURES_PER_BATCH; j++ )
			{
				int iIndex = ( j + 1 ) * NUM_BATCH_VERT_PER_ALLOC + iPosIndex;
				memcpy( &pResultVert->maLightPos[j],
						&saTempPos[iIndex],
						sizeof( tVector4 ) );
			}
		}
		
		// update normal
		int iNormIndex = pAnimModel->maVBOVertPtrs[i].miNorm;
		memcpy( &pResultVert->mNorm, 
				&saTempNorm[iNormIndex],
				sizeof( tVector4 ) );  

		memcpy( &pResultVert->mOrigNorm, 
				&saTempNorm[iNormIndex],
				sizeof( tVector4 ) );  

		++pResultVert;

	}	// for i = 0 to num vbo verts
    
	// TODO: OPTIMIZE THIS

	// transform vertex positions with only the model matrix
	pResultPos = saTempPos;
	pModelVertPos = pAnimModelInstance->maXFormPos;
	for( int i = 0; i < pAnimModel->miNumVerts; i++ )
	{
		Matrix44Transform( pResultPos, pModelVertPos, pXFormMatrix );

		++pResultPos;
		++pModelVertPos;
	}
	
	tVector4 xformPos = { 0.0f, 0.0f, 0.0f, 1.0f };

	// copy over to orig vertex position
	pResultVert = &pModelBatch->maInterleaveVerts[iCurrVertStart];
	for( int i = 0; i < pAnimModelInstance->miNumVBOVerts; i++ )
	{
		int iPosIndex = pAnimModel->maVBOVertPtrs[i].miPos;
		memcpy( &pResultVert->mOrigPos,
				&saTempPos[iPosIndex],
				sizeof( tVector4 ) );
		
		// calculate projection texture uv
		if( pInfo->mpProjectionTextureMatrix )
		{
			Matrix44Transform( &xformPos, &saTempPos[iPosIndex], pInfo->mpProjectionTextureMatrix );
			float fOneOverW = 1.0f / xformPos.fW;
			pResultVert->maProjPos[0].fX = ( xformPos.fX * fOneOverW + 1.0f ) * 0.5f;
			pResultVert->maProjPos[0].fY = ( xformPos.fY * fOneOverW + 1.0f ) * 0.5f;
			pResultVert->maProjPos[0].fZ = ( xformPos.fZ * fOneOverW + 1.0f ) * 0.5f;
			pResultVert->maProjPos[0].fW = 1.0f;
		}

		++pResultVert;
	}


	pModelBatch->miNumPos += pAnimModel->miNumVerts;
	pModelBatch->miNumVerts += pAnimModelInstance->miNumVBOVerts;
	pModelBatch->miNumIndices += pAnimModelInstance->miNumVBOIndices;
	++pModelBatch->miNumModels;
    pModelBatch->mbBlend = bBlend;

    //double fCurrTime = getCurrTime();
    //double fElapsed = fCurrTime - fStart;
    //gTotalAddToBatchTime += fElapsed;
    
	return true;
}

/*
**
*/
bool modelBatchAddMorphTarget( tModelBatchInstanceInfo const* pInfo )
{
    tModelBatch* pModelBatch = pInfo->mpModelBatch;
	tMorphTarget const* pMorphTarget = pInfo->mpMorphTarget;
	
    tMatrix44 const* pXFormMatrix = pInfo->mpXFormMatrix;
	tMatrix44 const* pViewMatrix = pInfo->mpViewMatrix;
	tMatrix44 const* pViewProjMatrix = pInfo->mpViewProjMatrix;
	tMatrix44 const* aLightViewProjMatrices = pInfo->maLightViewProjMatrices;
	GLuint const* aiTextureID = pInfo->maiTextureID;
	GLuint const* aiDepthTextures = pInfo->maiDepthTextures;
	tVector4 const* pColor = pInfo->mpColor;
	
	tModel const* pModel = pMorphTarget->mpModel;
    
	tMatrix44 modelViewMatrix;
	Matrix44Multiply( &modelViewMatrix, pViewMatrix, pXFormMatrix );
    
	// filled up
	if( pModelBatch->miNumIndices + pModel->miNumVBOIndices >= NUM_BATCH_VERT_PER_ALLOC )
	{
		return false;
	}
	
	if( aiDepthTextures )
	{
		for( int i = 0; i < NUM_DEPTH_TEXTURES_PER_BATCH; i++ )
		{
			pModelBatch->maiDepthTextureID[i] = aiDepthTextures[i];
		}
	}
    
	for( int i = 0; i < NUM_TEXTURES_PER_BATCH; i++ )
	{
		pModelBatch->maiTextureID[i] = aiTextureID[i];
	}
    
	// total data so far
	int iTotalVerts = pModelBatch->miNumVerts + pModel->miNumVBOVerts;
	int iTotalIndices = pModelBatch->miNumIndices + pModel->miNumVBOIndices;
    
	if( iTotalVerts > pModelBatch->miNumVertAlloc )
	{
		pModelBatch->maInterleaveVerts = (tInterleaveVert *)REALLOC( pModelBatch->maInterleaveVerts, sizeof( tInterleaveVert ) * iTotalVerts );
		WTFASSERT2( pModelBatch->maInterleaveVerts, "can't allocate interleave vertex" );
		pModelBatch->miNumVertAlloc = iTotalVerts;
	}
    
	if( iTotalIndices > pModelBatch->miNumIndexAlloc )
	{
		pModelBatch->maiIndices = (unsigned int *)REALLOC( pModelBatch->maiIndices, sizeof( int ) * iTotalIndices );
		WTFASSERT2( pModelBatch->maiIndices, "can't allocate indices" );
		pModelBatch->miNumIndexAlloc = iTotalIndices;
	}
    
	// model * view * proj matrix
	tMatrix44 totalMat;
	Matrix44Multiply( &totalMat, pViewProjMatrix, pXFormMatrix );
	
	// model * light view * light proj matrix
	tMatrix44 aTotalLightMatrices[3];
	if( aiDepthTextures )
	{
		for( int i = 0; i < NUM_DEPTH_TEXTURES_PER_BATCH; i++ )
		{
			if( aLightViewProjMatrices && aiDepthTextures[i] > 0 )
			{
				Matrix44Multiply( &aTotalLightMatrices[i],
                                 &aLightViewProjMatrices[i],
                                 pXFormMatrix );
			}
		}
	}
    
	tVector4* pResultPos = saTempPos;
	tVector4 const* pModelVertPos = pModel->maPos;
	
	// transform vertex positions
	for( int i = 0; i < pModel->miNumVerts; i++ )
	{
		Matrix44Transform( pResultPos, pModelVertPos, &totalMat );
		
		// transform position from light
		if( aiDepthTextures )
		{
			for( int j = 0; j < NUM_DEPTH_TEXTURES_PER_BATCH; j++ )
			{
				int iIndex = ( j + 1 ) * NUM_BATCH_VERT_PER_ALLOC + i;
				if( aLightViewProjMatrices && aiDepthTextures[j] > 0 )
				{
					Matrix44Transform( &saTempPos[iIndex], pModelVertPos, &aTotalLightMatrices[j] );
				}
			}
		}
		
		++pResultPos;
		++pModelVertPos;
        
	}	// for i = 0 to num vertices
    
	tVector4* pResultNorm = saTempNorm;
	tVector4 const* pModelNorm = pModel->maNorm;
	
	// transform normals
	for( int i = 0; i < pModel->miNumNormals; i++ )
	{
		Matrix44Transform( pResultNorm, pModelNorm, &pMorphTarget->mRotationMatrix );
		pResultNorm->fW = 1.0f;
        
		++pResultNorm;
		++pModelNorm;
	}
    
	// update indices
	int iCurrIndexStart = pModelBatch->miNumIndices;
	int iCurrVertStart = pModelBatch->miNumVerts;
	unsigned int* aiResultIndices = &pModelBatch->maiIndices[iCurrIndexStart];
	WTFASSERT2( pModelBatch->miNumIndexAlloc >= pModel->miNumVBOIndices + iCurrIndexStart, "invalid index" );
    
	for( int i = 0; i < pModel->miNumVBOIndices; i++ )
	{
		*aiResultIndices = pModel->maiVBOIndices[i] + iCurrVertStart;
		WTFASSERT2( *aiResultIndices < (unsigned int)iTotalIndices, "index out of bounds" );
        
		++aiResultIndices;
        
	}	// for i = 0 to num vbo indices
	
	// copy the vbo verts from original
	WTFASSERT2( pModelBatch->miNumVertAlloc >= pModel->miNumVBOVerts + iCurrVertStart, "invalid index" );
	memcpy( &pModelBatch->maInterleaveVerts[iCurrVertStart],
           pModel->maVBOVerts,
           sizeof( tInterleaveVert ) * pModel->miNumVBOVerts );
	
	// update vbo vertex position with the xform
	tInterleaveVert* pResultVert = &pModelBatch->maInterleaveVerts[iCurrVertStart];
	for( int i = 0; i < pModel->miNumVBOVerts; i++ )
	{
		int iPosIndex = pModel->maVBOVertPtrs[i].miPos;
		memcpy( &pResultVert->mPos,
               &saTempPos[iPosIndex],
               sizeof( tVector4 ) );
		
		// light position
		if( aLightViewProjMatrices && aiDepthTextures )
		{
			for( int j = 0; j < NUM_DEPTH_TEXTURES_PER_BATCH; j++ )
			{
				int iIndex = ( j + 1 ) * NUM_BATCH_VERT_PER_ALLOC + iPosIndex;
				memcpy( &pResultVert->maLightPos[j],
                       &saTempPos[iIndex],
                       sizeof( tVector4 ) );
			}
		}
        
		int iNormIndex = pModel->maVBOVertPtrs[i].miNorm;
		memcpy( &pResultVert->mNorm,
               &saTempNorm[iNormIndex],
               sizeof( tVector4 ) );
        
		memcpy( &pResultVert->mOrigNorm,
               &saTempNorm[iNormIndex],
               sizeof( tVector4 ) );
         
		++pResultVert;
        
	}	// for i = 0 to num vbo verts
    
	// TODO: OPTIMIZE THIS
    
	// transform vertex positions with only the model matrix
	pResultPos = saTempPos;
	pModelVertPos = pModel->maPos;
	for( int i = 0; i < pModel->miNumVerts; i++ )
	{
		Matrix44Transform( pResultPos, pModelVertPos, pXFormMatrix );
        
		++pResultPos;
		++pModelVertPos;
	}
    
	tVector4 xformPos = { 0.0f, 0.0f, 0.0f, 1.0f };
	
	// copy over to orig vertex position
	pResultVert = &pModelBatch->maInterleaveVerts[iCurrVertStart];
	for( int i = 0; i < pModel->miNumVBOVerts; i++ )
	{
		int iPosIndex = pModel->maVBOVertPtrs[i].miPos;
		memcpy( &pResultVert->mOrigPos,
               &saTempPos[iPosIndex],
               sizeof( tVector4 ) );
        
		memcpy( &pResultVert->mColor, pColor, sizeof( tVector4 ) );
		
		// calculate projection texture uv
		if( pInfo->mpProjectionTextureMatrix )
		{
			Matrix44Transform( &xformPos, &saTempPos[iPosIndex], pInfo->mpProjectionTextureMatrix );
			float fOneOverW = 1.0f / xformPos.fW;
			pResultVert->maProjPos[0].fX = ( xformPos.fX * fOneOverW + 1.0f ) * 0.5f;
			pResultVert->maProjPos[0].fY = ( xformPos.fY * fOneOverW + 1.0f ) * 0.5f;
			pResultVert->maProjPos[0].fZ = ( xformPos.fZ * fOneOverW + 1.0f ) * 0.5f;
			pResultVert->maProjPos[0].fW = 1.0f;
		}
        
		++pResultVert;
	}
	
	pModelBatch->miNumPos += pModel->miNumVerts;
	pModelBatch->miNumVerts += pModel->miNumVBOVerts;
	pModelBatch->miNumIndices += pModel->miNumVBOIndices;
	++pModelBatch->miNumModels;

	return true;
}

/*
**
*/
void modelBatchSetupGL( tModelBatch* pBatch, GLint iShader )
{
    // create vertex and index buffer
    glGenBuffers( 1, &pBatch->miVBO );
    glGenBuffers( 1, &pBatch->miIBO );
    
	// query attributes from shader
	pBatch->miShader = iShader;
   	pBatch->miLightProbe = glGetUniformLocation( iShader, "lightProbe" );
    pBatch->miAlbedo = glGetUniformLocation( iShader, "texture" );
    
	glEnableVertexAttribArray( SHADER_ATTRIB_POSITION );
	glEnableVertexAttribArray( SHADER_ATTRIB_UV );
	glEnableVertexAttribArray( SHADER_ATTRIB_NORM );
}

/*
**
*/
void modelBatchSetupNormalGL( tModelBatch* pBatch, tShaderProgram const* pShader )
{
    // create vertex and index buffer
	if( pBatch->miVBO <= 0 )
	{
		glGenBuffers( 1, &pBatch->miVBO );
	}

    WTFASSERT2( pBatch->miVBO > 0, "can't create vbo for batch" );
    
	if( pBatch->miIBO <= 0 )
	{
		glGenBuffers( 1, &pBatch->miIBO );
	}

#if 0
	// texture units
	pBatch->miAlbedo = glGetUniformLocation( iShader, "texture" );
	pBatch->miLightProbe = glGetUniformLocation( iShader, "lightProbe" );
	pBatch->miLightmap = glGetUniformLocation( iShader, "lightmap" );
    
	// light dir
	pBatch->miLightDir = glGetUniformLocation( iShader, "gLightDir" );

	// camera position
	pBatch->miCameraPos = glGetUniformLocation( iShader, "gCameraPos" );

	pBatch->miProjectedTexture = glGetUniformLocation( iShader, "projectiveCloudTexture" );
#endif // #if 0
	
	CShaderManager* pShaderManager = CShaderManager::instance();

	// texture units
	pBatch->miAlbedo = pShaderManager->getUniformLocation( pShader, "texture" );
	pBatch->miLightProbe = pShaderManager->getUniformLocation( pShader, "lightProbe" );
	pBatch->miLightmap = pShaderManager->getUniformLocation( pShader, "lightmap" );
    
	// light dir
	pBatch->miLightDir = pShaderManager->getUniformLocation( pShader, "gLightDir" );

	// camera position
	pBatch->miCameraPos = pShaderManager->getUniformLocation( pShader, "gCameraPos" );

	pBatch->miProjectedTexture = pShaderManager->getUniformLocation( pShader, "projectiveCloudTexture" );

	// texture units
    if( pBatch->miAlbedo >= 0 )
    {
        glUniform1i( pBatch->miAlbedo, 0 );
        glActiveTexture( GL_TEXTURE0 );
        glEnable( GL_TEXTURE_2D );
    }
    
    if( pBatch->miLightProbe >= 0 )
    {
        glUniform1i( pBatch->miLightProbe, 1 );
        glActiveTexture( GL_TEXTURE0 + 1 );
        glEnable( GL_TEXTURE_2D );
    }
    
    if( pBatch->miProjectedTexture >= 0 )
    {
        glUniform1i( pBatch->miProjectedTexture, 2 );
        glActiveTexture( GL_TEXTURE0 + 2 );
        glEnable( GL_TEXTURE_2D );
    }
    
    // lightmap
    if( pBatch->miLightmap >= 0 )
    {
        glUniform1i( pBatch->miLightmap, 3 );
        glActiveTexture( GL_TEXTURE0 + 3 );
        glEnable( GL_TEXTURE_2D );
    }
}

/*
**
*/
void modelBatchSetupShadowGL( tModelBatch* pBatch, tShaderProgram const* pShader )
{
	GLint iShader = pShader->miID;

	// create vertex and index buffer
	if( pBatch->miVBO <= 0 )
	{
		glGenBuffers( 1, &pBatch->miVBO );
	}

	if( pBatch->miIBO <= 0 )
	{
		glGenBuffers( 1, &pBatch->miIBO );
	}

	pBatch->miShader = iShader;
	
	CShaderManager* pShaderManager = CShaderManager::instance();

#if 0
	// texture units
	pBatch->miAlbedo = glGetUniformLocation( iShader, "texture" );
	pBatch->miLightProbe = glGetUniformLocation( iShader, "lightProbe" );
	pBatch->maiDepth[0] = glGetUniformLocation( iShader, "lightPOVDepth0" );
	pBatch->miProjectedShadow = glGetUniformLocation( iShader, "projectiveShadowTexture" );
	pBatch->miProjectedTexture = glGetUniformLocation( iShader, "projectiveCloudTexture" );

	// light dir
	pBatch->miLightDir = glGetUniformLocation( iShader, "gLightDir" );

	// camera position
	pBatch->miCameraPos = glGetUniformLocation( iShader, "gCameraPos" );

	pBatch->miLookDir = glGetUniformLocation( iShader, "gLookDir" );
#endif // #if 0

	pBatch->maiDepth[1] = -1;

	// texture units
	pBatch->miAlbedo = pShaderManager->getUniformLocation( pShader, "texture" );
	pBatch->miLightProbe = pShaderManager->getUniformLocation( pShader, "lightProbe" );
	pBatch->maiDepth[0] = pShaderManager->getUniformLocation( pShader, "lightPOVDepth0" );
	pBatch->miProjectedShadow = pShaderManager->getUniformLocation( pShader, "projectiveShadowTexture" );
	pBatch->miProjectedTexture = pShaderManager->getUniformLocation( pShader, "projectiveCloudTexture" );

	// light dir
	pBatch->miLightDir = pShaderManager->getUniformLocation( pShader, "gLightDir" );

	// camera position
	pBatch->miCameraPos = pShaderManager->getUniformLocation( pShader, "gCameraPos" );

	pBatch->miLookDir = pShaderManager->getUniformLocation( pShader, "gLookDir" );

    if( pBatch->miAlbedo >= 0 )
    {
        glUniform1i( pBatch->miAlbedo, 0 );
        glActiveTexture( GL_TEXTURE0 );
        glEnable( GL_TEXTURE_2D );
    }
    
    if( pBatch->miLightProbe >= 0 )
    {
        glUniform1i( pBatch->miLightProbe, 1 );
        glActiveTexture( GL_TEXTURE0 + 1 );
        glEnable( GL_TEXTURE_2D );
    }
    
    if( pBatch->miProjectedShadow >= 0 )
    {
        glUniform1i( pBatch->miProjectedShadow, 2 );
        glActiveTexture( GL_TEXTURE0 + 2 );
        glEnable( GL_TEXTURE_2D );
    }
    
	for( int i = 0; i < NUM_DEPTH_TEXTURES_PER_BATCH; i++ )
	{
		if( pBatch->maiDepth[i] >= 0 )
		{
			glUniform1i( pBatch->maiDepth[i], i + NUM_TEXTURES_PER_BATCH );
			glActiveTexture( GL_TEXTURE0 + i + NUM_TEXTURES_PER_BATCH );
			glEnable( GL_TEXTURE_2D );
		}
	}

    if( pBatch->miProjectedTexture >= 0 )
    {
        glUniform1i( pBatch->miProjectedTexture, NUM_TEXTURES_PER_BATCH + 1 );
        glActiveTexture( GL_TEXTURE0 + NUM_TEXTURES_PER_BATCH + 1 );
        glEnable( GL_TEXTURE_2D );
    }
}

/*
**
*/
void modelBatchUpdateDataGL( tModelBatch const* pBatch )
{
	// data array
    glBindBuffer( GL_ARRAY_BUFFER, pBatch->miVBO );
    glBufferData( GL_ARRAY_BUFFER, 
				  sizeof( tInterleaveVert ) * pBatch->miNumVerts, 
				  pBatch->maInterleaveVerts, 
				  GL_DYNAMIC_DRAW );
    
    // index array
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pBatch->miIBO );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, 
				  sizeof( int ) * pBatch->miNumIndices, 
				  pBatch->maiIndices, 
				  GL_DYNAMIC_DRAW );
}

/*
**
*/
void modelBatchDraw( tModelBatch const* pBatch,
					 GLint iTex )

{
	// texture
	if( iTex >= 0 )
	{
		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, iTex );
	}
	
	// vbo
    glBindBuffer( GL_ARRAY_BUFFER, pBatch->miVBO );
    glBufferData( GL_ARRAY_BUFFER, 
				  sizeof( tInterleaveVert ) * pBatch->miNumVerts, 
				  pBatch->maInterleaveVerts, 
				  GL_DYNAMIC_DRAW );

	// position, normal, and uv
    glVertexAttribPointer( SHADER_ATTRIB_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof( tInterleaveVert ), (void *)0 );
	glVertexAttribPointer( SHADER_ATTRIB_UV, 2, GL_FLOAT, GL_FALSE, sizeof( tInterleaveVert ), (void *)( sizeof( tVector4 ) ) );
    glVertexAttribPointer( SHADER_ATTRIB_NORM, 4, GL_FLOAT, GL_FALSE, sizeof( tInterleaveVert ), (void *)( sizeof( tVector4 ) + sizeof( tVector2 ) ) );
	
	// index buffer
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pBatch->miIBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 
				  sizeof( int ) * pBatch->miNumIndices, 
				  pBatch->maiIndices, 
				  GL_DYNAMIC_DRAW );

	// draw
    glDrawElements( GL_TRIANGLES, pBatch->miNumIndices, GL_UNSIGNED_INT, (void *)0 );
}

/*
**
*/
void modelBatchDrawNormal( tModelBatch const* pBatch,
						   GLuint const* aiTextureID,
						   tVector4 const* pLightDir,
						   tVector4 const* pCameraPos )
{
    WTFASSERT2( pBatch->miVBO > 0, "invalid vbo" );
    
	// texture
	for( int i = 0; i < NUM_TEXTURES_PER_BATCH; i++ )
	{
		if( aiTextureID[i] > 0 )
		{
			glActiveTexture( GL_TEXTURE0 + i );
			glBindTexture( GL_TEXTURE_2D, aiTextureID[i] );
		}
	}
	
	// light direction
	glUniform4f( pBatch->miLightDir, pLightDir->fX, pLightDir->fY, pLightDir->fZ, 1.0f );

	// camera position
	glUniform4f( pBatch->miCameraPos, pCameraPos->fX, pCameraPos->fY, pCameraPos->fZ, 1.0f );

	// vbo
    glBindBuffer( GL_ARRAY_BUFFER, pBatch->miVBO );
    glBufferData( GL_ARRAY_BUFFER, 
				  sizeof( tInterleaveVert ) * pBatch->miNumVerts, 
				  pBatch->maInterleaveVerts, 
				  GL_DYNAMIC_DRAW );

	// position, normal, and uv
    glVertexAttribPointer( SHADER_ATTRIB_POSITION, 
						   4, 
						   GL_FLOAT, 
						   GL_FALSE, 
						   sizeof( tInterleaveVert ), 
						   (void *)0 );
	glVertexAttribPointer( SHADER_ATTRIB_UV, 
						   2, 
						   GL_FLOAT, 
						   GL_FALSE, 
						   sizeof( tInterleaveVert ), 
						   (void *)( sizeof( tVector4 ) ) );
    glVertexAttribPointer( SHADER_ATTRIB_NORM, 
						   4, 
						   GL_FLOAT, 
						   GL_FALSE, 
						   sizeof( tInterleaveVert ), 
						   (void *)( sizeof( tVector4 ) + sizeof( tVector2 ) ) );
	glVertexAttribPointer( SHADER_ATTRIB_COLOR, 
						   4, 
						   GL_FLOAT, 
						   GL_FALSE, 
						   sizeof( tInterleaveVert ), 
						   (void *)( sizeof( tVector4 ) + sizeof( tVector2 ) + sizeof( tVector4 ) ) );

	// draw texture atlas
	glVertexAttribPointer( SHADER_ATTRIB_ORIG_POS, 
						   4, 
						   GL_FLOAT, 
						   GL_FALSE, 
						   sizeof( tInterleaveVert ), 
						   (void *)( sizeof( tVector4 ) + sizeof( tVector2 ) + sizeof( tVector4 ) + sizeof( tVector4 ) + sizeof( tVector4 ) ) );
	
	glVertexAttribPointer( SHADER_ATTRIB_ORIG_NORM, 
						   4, 
						   GL_FLOAT, 
						   GL_FALSE, 
						   sizeof( tInterleaveVert ), 
						   (void *)( sizeof( tVector4 ) + sizeof( tVector2 ) + sizeof( tVector4 ) + sizeof( tVector4 ) + sizeof( tVector4 ) + sizeof( tVector4 ) ) );

	// projected uv
	glVertexAttribPointer( SHADER_ATTRIB_PROJECTIVE_UV_0, 
						   4, 
						   GL_FLOAT, 
						   GL_FALSE, 
						   sizeof( tInterleaveVert ),
						   (void *)( sizeof( tVector4 ) + sizeof( tVector2 ) + sizeof( tVector4 ) + sizeof( tVector4 ) + sizeof( tVector4 ) + sizeof( tVector4 ) + sizeof( tVector4 ) ) );


	// index buffer
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pBatch->miIBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 
				  sizeof( int ) * pBatch->miNumIndices, 
				  pBatch->maiIndices, 
				  GL_DYNAMIC_DRAW );

	// draw
    glDrawElements( GL_TRIANGLES, pBatch->miNumIndices, GL_UNSIGNED_INT, (void *)0 );

	if( pBatch->mbBlend )
	{
		glDepthMask( GL_TRUE );
		glEnable( GL_DEPTH_TEST );
	}
}

/*
**
*/
void modelBatchDrawShadow( tModelBatch const* pBatch,
						   GLuint const* aiTextureID,
						   GLuint const* aiDepthTextureID,
						   tVector4 const* pLightDir,
						   tVector4 const* pCameraPos,
						   tVector4 const* pCameraDir )
{
	if( pBatch->mbBlend )
	{
		glEnable( GL_BLEND );
		glDepthMask( GL_FALSE );
		glDisable( GL_DEPTH_TEST );
	}
	else
	{
		glDisable( GL_BLEND );
	}

	// texture
	for( int i = 0; i < NUM_TEXTURES_PER_BATCH; i++ )
	{
		if( aiTextureID[i] > 0 )
		{
			glActiveTexture( GL_TEXTURE0 + i );
			glBindTexture( GL_TEXTURE_2D, aiTextureID[i] );
		}
	}
	
	
	glActiveTexture( GL_TEXTURE0 + NUM_TEXTURES_PER_BATCH );
	glBindTexture( GL_TEXTURE_2D, aiDepthTextureID[0] );
	
	glActiveTexture( GL_TEXTURE0 + NUM_TEXTURES_PER_BATCH + 1 );
	glBindTexture( GL_TEXTURE_2D, aiDepthTextureID[1] );

	// light direction
	glUniform4f( pBatch->miLightDir, pLightDir->fX, pLightDir->fY, pLightDir->fZ, 1.0f );

	// camera position
	glUniform4f( pBatch->miCameraPos, pCameraPos->fX, pCameraPos->fY, pCameraPos->fZ, 1.0f );

	// look direction
	glUniform4f( pBatch->miLookDir, pCameraDir->fX, pCameraDir->fY, pCameraDir->fZ, 1.0f );

	// vbo
    glBindBuffer( GL_ARRAY_BUFFER, pBatch->miVBO );
    glBufferData( GL_ARRAY_BUFFER, 
				  sizeof( tInterleaveVert ) * pBatch->miNumVerts, 
				  pBatch->maInterleaveVerts, 
				  GL_DYNAMIC_DRAW );

	// position, normal, and uv
    glVertexAttribPointer( SHADER_ATTRIB_POSITION, 
						   4, 
						   GL_FLOAT, 
						   GL_FALSE, 
						   sizeof( tInterleaveVert ), 
						   (void *)0 );
	glVertexAttribPointer( SHADER_ATTRIB_UV, 
						   2, 
						   GL_FLOAT, 
						   GL_FALSE, 
						   sizeof( tInterleaveVert ), 
						   (void *)( sizeof( tVector4 ) ) );
    glVertexAttribPointer( SHADER_ATTRIB_NORM, 
						   4, 
						   GL_FLOAT, 
						   GL_FALSE, 
						   sizeof( tInterleaveVert ), 
						   (void *)( sizeof( tVector4 ) + sizeof( tVector2 ) ) );
	glVertexAttribPointer( SHADER_ATTRIB_COLOR, 
						   4, 
						   GL_FLOAT, 
						   GL_FALSE, 
						   sizeof( tInterleaveVert ), 
						   (void *)( sizeof( tVector4 ) + sizeof( tVector2 ) + sizeof( tVector4 ) ) );

	// position from light's view
	glVertexAttribPointer( SHADER_ATTRIB_LIGHTPOS_0, 4, GL_FLOAT, GL_FALSE, sizeof( tInterleaveVert ), (void *)( sizeof( tVector4 ) + sizeof( tVector2 ) + sizeof( tVector4 ) + sizeof( tVector4 ) ) );
	//glVertexAttribPointer( aiLightPos[1], 4, GL_FLOAT, GL_FALSE, sizeof( tInterleaveVert ), (void *)( sizeof( tVector4 ) + sizeof( tVector2 ) + sizeof( tVector4 ) + sizeof( int ) + sizeof( tVector4 ) ) );
	
	// original position and normal
	glVertexAttribPointer( SHADER_ATTRIB_ORIG_POS, 
						   4, 
						   GL_FLOAT, 
						   GL_FALSE, 
						   sizeof( tInterleaveVert ),
						   (void *)( sizeof( tVector4 ) + sizeof( tVector2 ) + sizeof( tVector4 ) + sizeof( tVector4 ) + sizeof( tVector4 ) ) );

	glVertexAttribPointer( SHADER_ATTRIB_ORIG_NORM, 
						   4, 
						   GL_FLOAT, 
						   GL_FALSE, 
						   sizeof( tInterleaveVert ),
						   (void *)( sizeof( tVector4 ) + sizeof( tVector2 ) + sizeof( tVector4 ) + sizeof( tVector4 ) + sizeof( tVector4 ) + sizeof( tVector4 ) ) );

	// projected uv
	glVertexAttribPointer( SHADER_ATTRIB_PROJECTIVE_UV_0, 
						   4, 
						   GL_FLOAT, 
						   GL_FALSE, 
						   sizeof( tInterleaveVert ),
						   (void *)( sizeof( tVector4 ) + sizeof( tVector2 ) + sizeof( tVector4 ) + sizeof( tVector4 ) + sizeof( tVector4 ) + sizeof( tVector4 ) + sizeof( tVector4 ) ) );

	// index buffer
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pBatch->miIBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 
				  sizeof( int ) * pBatch->miNumIndices, 
				  pBatch->maiIndices, 
				  GL_DYNAMIC_DRAW );

	// draw
    glDrawElements( GL_TRIANGLES, pBatch->miNumIndices, GL_UNSIGNED_INT, (void *)0 );

	if( pBatch->mbBlend )
	{
		glDepthMask( GL_TRUE );
		glEnable( GL_DEPTH_TEST );
	}
}