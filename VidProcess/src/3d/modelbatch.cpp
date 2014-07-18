#include "modelbatch.h"
#include "model.h"
#include "factory.h"
#include "timeutil.h"
#include "jobmanager.h"
#include <pthread.h>

#define NUM_VERTS_TO_CREATE_JOBS 1000000

struct XFormJobData
{
    tVector4*           maResult0;
    tVector4*           maResult1;
    
    tVector4 const*     maOrig0;
    tVector4 const*     maOrig1;
    
    int                 miNumEntries0;
    int                 miNumEntries1;
    
    tMatrix44 const*    mpMatrix0;
    tMatrix44 const*    mpMatrix1;
};

typedef struct XFormJobData tXFormJobData;

static void transformVerts( tVector4* pResultNorm,
                           tVector4* pResultPos,
                           tVector4* pResultOrigPos,
                           int iNumVerts,
                           int iNumNormals,
                           tVector4 const* pOrigNorm,
                           tVector4 const* pOrigPos,
                           tMatrix44 const* pTotalMatrix,
                           tMatrix44 const* pXFormMatrix,
                           tMatrix44 const* pRotationMatrix,
                           GLuint const* aiDepthTextures,
                           tMatrix44 const* aLightViewProjMatrices,
                           tMatrix44 const* aTotalLightMatrices );

static void extrudeVerts( tModelBatch* pModelBatch,
						  tModel const* pModel,
						  tModelInstance const* pModelInstance,
						  tMatrix44 const* pTotalMatrix,
						  tMatrix44 const* pXFormMatrix,
						  tVector4 const* pColor,
						  float fFurDistance );

static void copyToVBO( tModelBatch* pModelBatch,
                       tInterleaveVert const* aVBOVerts,
                       tVector4 const* pXFormPos,
                       tVector4 const* pXFormNorms,
                       tVector4 const* pXFormOrigPos,
                       tVector4 const* pColor,
                       unsigned int const* aiVBOIndices,
                       int iNumVBOVerts,
                       int iNumVBOIndices,
                       int iNumVerts,
                       tMatrix44 const* aLightViewProjMatrices,
                       tMatrix44 const* pProjectionTextureMatrix,
                       GLuint const* aiDepthTextures,
                       tInterleaveVertMap const* aVBOVertPtrs,
                       CTextureAtlasManager const* pTextureAtlasManager,
                       const char* szTexture,
					   tVector2 const* pCloudUVOffset,
					   int iCurrIndexStart,
					   int iCurrVertStart,
					   float fFurDistance );

static void transformAndCopyToVBO( void* pData, void* pDebugData );

static tVector4* saTempPos = NULL;
static tVector4* saTempNorm = NULL;
static tVector4* saTempOrigPos = NULL;
static pthread_mutex_t sMutex;

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

	pModelBatch->miNumModelJobInfo = 0;

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
		saTempPos = (tVector4 *)MALLOC( sizeof( tVector4 ) * NUM_BATCH_VERT_PER_ALLOC * 4 * gpJobManager->miNumWorkers );
	}

	if( saTempNorm == NULL )
	{
		saTempNorm = (tVector4 *)MALLOC( sizeof( tVector4 ) * NUM_BATCH_VERT_PER_ALLOC * gpJobManager->miNumWorkers );
	}
    
    if( saTempOrigPos == NULL )
	{
		saTempOrigPos = (tVector4 *)MALLOC( sizeof( tVector4 ) * NUM_BATCH_VERT_PER_ALLOC * gpJobManager->miNumWorkers );
	}

	pthread_mutex_init( &sMutex, NULL );
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
double fStart = getCurrTime();

	tModelBatch* pModelBatch = pInfo->mpModelBatch;
	tModelInstance const* pModelInstance = pInfo->mpModelInstance;
	tMatrix44 const* pXFormMatrix = pInfo->mpXFormMatrix;
	tMatrix44 const* pViewMatrix = pInfo->mpViewMatrix;
	tMatrix44 const* pViewProjMatrix = pInfo->mpViewProjMatrix;
	tMatrix44 const* aLightViewProjMatrices = pInfo->maLightViewProjMatrices;
	GLuint const* aiTextureID = pInfo->maiTextureID;
	GLuint const* aiDepthTextures = pInfo->maiDepthTextures;
	tVector4 const* pColor = pInfo->mpColor;
	float fFurDistance = pInfo->mfFurDistance;

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

    if( aiTextureID )
    {
        for( int i = 0; i < NUM_TEXTURES_PER_BATCH; i++ )
        {
            pModelBatch->maiTextureID[i] = aiTextureID[i];
        }
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
	
	// copy over job data to be used later
	WTFASSERT2( pModelBatch->miNumModelJobInfo >= 0 && pModelBatch->miNumModelJobInfo < sizeof( pModelBatch->maModelBatchJobInfo ) / sizeof( *(pModelBatch->maModelBatchJobInfo) ), "too many job info in batch" );
	tModelBatchJobInfo* pJobInfo = &pModelBatch->maModelBatchJobInfo[pModelBatch->miNumModelJobInfo++];
	
	memset( pJobInfo, 0, sizeof( tModelBatchJobInfo ) );
	pJobInfo->mpModelBatch = pModelBatch;
	pJobInfo->maResultNorm = saTempNorm;
	pJobInfo->maResultPos = saTempPos;
	pJobInfo->maResultOrigPos = saTempOrigPos;
	pJobInfo->miNumVerts = pModel->miNumVerts;
	pJobInfo->miNumNormals = pModel->miNumNormals;
	pJobInfo->maOrigNorm = pModel->maNorm;
	pJobInfo->maOrigPos = pModel->maPos;
	
	memcpy( &pJobInfo->mTotalMatrix, &totalMat, sizeof( tMatrix44 ) );

	pJobInfo->mpXFormMatrix = pXFormMatrix;
	pJobInfo->mpRotationMatrix = &pModelInstance->mRotationMatrix;
	pJobInfo->maiDepthTextures  = aiDepthTextures;
	pJobInfo->maLightViewProjMatrices = aLightViewProjMatrices;
	
	memcpy( &pJobInfo->maTotalLightMatrices[0], &aTotalLightMatrices[0], sizeof( tMatrix44 ) );
	memcpy( &pJobInfo->maTotalLightMatrices[1], &aTotalLightMatrices[1], sizeof( tMatrix44 ) );
	memcpy( &pJobInfo->maTotalLightMatrices[2], &aTotalLightMatrices[2], sizeof( tMatrix44 ) );

	pJobInfo->maVBOVerts = pModel->maVBOVerts;
	pJobInfo->maiVBOIndices = pModel->maiVBOIndices;
	pJobInfo->miNumVBOVerts = pModel->miNumVBOVerts;
	pJobInfo->miNumVBOIndices = pModel->miNumVBOIndices;
	pJobInfo->maLightViewProjMatrices = aLightViewProjMatrices;
	pJobInfo->mpProjectionTextureMatrix = pInfo->mpProjectionTextureMatrix;
	pJobInfo->maVBOVertPtrs = pModel->maVBOVertPtrs;
	pJobInfo->mpTextureAtlasManager = pInfo->mpTextureAtlasManager;
	pJobInfo->mszTexture = pModelInstance->mszTexture;
	pJobInfo->mpCloudUVOffset = pInfo->mpCloudUVOffset;

	pJobInfo->miCurrBatchNumPos = pModelBatch->miNumPos;
	pJobInfo->miCurrBatchNumVBOVerts = pModelBatch->miNumVerts;
	pJobInfo->miCurrBatchNumVBOIndices = pModelBatch->miNumIndices;

	memcpy( &pJobInfo->mColor, pColor, sizeof( tVector4 ) );

	pJobInfo->mpModelInstance = pModelInstance;
	pJobInfo->mpAnimModelInstance = NULL;
	pJobInfo->mpMorphTarget = NULL;

	pJobInfo->mpOrigInfo = pJobInfo;
	pJobInfo->miIndexInBatch = pModelBatch->miNumModelJobInfo - 1;

	extrudeVerts( pModelBatch,
				  pModel,
				  pModelInstance,
				  &totalMat,
				  pXFormMatrix,
				  pColor,
				  fFurDistance );


				 
					

#if 0
    transformVerts( saTempNorm,
                    saTempPos,
                    saTempOrigPos,
                    pModel->miNumVerts,
                    pModel->miNumNormals,
                    pModel->maNorm,
                    pModel->maPos,
                    &totalMat,
                    pXFormMatrix,
                    &pModelInstance->mRotationMatrix,
                    aiDepthTextures,
                    aLightViewProjMatrices,
                    aTotalLightMatrices );

    copyToVBO( pModelBatch,
               pModel->maVBOVerts,
               saTempPos,
               saTempNorm,
               saTempOrigPos,
               pColor,
               pModel->maiVBOIndices,
               pModel->miNumVBOVerts,
               pModel->miNumVBOIndices,
               pModel->miNumVerts,
               aLightViewProjMatrices,
               pInfo->mpProjectionTextureMatrix,
               aiDepthTextures,
               pModel->maVBOVertPtrs,
               pInfo->mpTextureAtlasManager,
               pModelInstance->mszTexture,
			   pInfo->mpCloudUVOffset,
			   pModelBatch->miNumIndices,
			   pModelBatch->miNumVerts,
			   fFurDistance );
#endif // #if 0

	pModelBatch->miNumPos += pModel->miNumVerts;
	pModelBatch->miNumVerts += pModel->miNumVBOVerts;
	pModelBatch->miNumIndices += pModel->miNumVBOIndices;
	++pModelBatch->miNumModels;

double fCurrTime = getCurrTime();
double fElapsed = fCurrTime - fStart;
gTotalAddToBatchTime += fElapsed;

	return true;
}

/*
**
*/
bool modelBatchAddAnimModel( tModelBatchInstanceInfo const* pInfo )
{
double fStart = getCurrTime();
    
	tModelBatch* pModelBatch = pInfo->mpModelBatch;
	tAnimModelInstance const* pAnimModelInstance = pInfo->mpAnimModelInstance;
	tMatrix44 const* pXFormMatrix = pInfo->mpXFormMatrix;
	tMatrix44 const* pViewProjMatrix = pInfo->mpViewProjMatrix;
	tMatrix44 const* aLightViewProjMatrices = pInfo->maLightViewProjMatrices;
	GLuint const* aiTextureID = pInfo->maiTextureID;
	GLuint const* aiDepthTextures = pInfo->maiDepthTextures;
    tVector4 const* pColor = pInfo->mpColor;
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
    
	// copy over job data to be used later
	WTFASSERT2( pModelBatch->miNumModelJobInfo >= 0 && pModelBatch->miNumModelJobInfo < sizeof( pModelBatch->maModelBatchJobInfo ) / sizeof( *(pModelBatch->maModelBatchJobInfo) ), "too many job info in batch" );
	tModelBatchJobInfo* pJobInfo = &pModelBatch->maModelBatchJobInfo[pModelBatch->miNumModelJobInfo++];

	memset( pJobInfo, 0, sizeof( tModelBatchJobInfo ) );
	pJobInfo->mpModelBatch = pModelBatch;
	pJobInfo->maResultNorm = saTempNorm;
	pJobInfo->maResultPos = saTempPos;
	pJobInfo->maResultOrigPos = saTempOrigPos;
	pJobInfo->miNumVerts = pAnimModel->miNumVerts;
	pJobInfo->miNumNormals = pAnimModel->miNumNormals;
	pJobInfo->maOrigNorm = pAnimModelInstance->maXFormNorm;
	pJobInfo->maOrigPos = pAnimModelInstance->maXFormPos;
	
	memcpy( &pJobInfo->mTotalMatrix, &totalMat, sizeof( tMatrix44 ) );

	pJobInfo->mpXFormMatrix = pXFormMatrix;
	pJobInfo->mpRotationMatrix = &pAnimModelInstance->mRotationMatrix;
	pJobInfo->maiDepthTextures  = aiDepthTextures;
	pJobInfo->maLightViewProjMatrices = aLightViewProjMatrices;
	
	memcpy( &pJobInfo->maTotalLightMatrices[0], &aTotalLightMatrices[0], sizeof( tMatrix44 ) );
	memcpy( &pJobInfo->maTotalLightMatrices[1], &aTotalLightMatrices[1], sizeof( tMatrix44 ) );
	memcpy( &pJobInfo->maTotalLightMatrices[2], &aTotalLightMatrices[2], sizeof( tMatrix44 ) );

	pJobInfo->maVBOVerts = pAnimModelInstance->maVBOVerts;
	pJobInfo->maiVBOIndices = pAnimModel->maiVBOIndices;
	pJobInfo->miNumVBOVerts = pAnimModel->miNumVBOVerts;
	pJobInfo->miNumVBOIndices = pAnimModelInstance->miNumVBOIndices;
	pJobInfo->maLightViewProjMatrices = aLightViewProjMatrices;
	pJobInfo->mpProjectionTextureMatrix = pInfo->mpProjectionTextureMatrix;
	pJobInfo->maVBOVertPtrs = pAnimModel->maVBOVertPtrs;
	pJobInfo->mpTextureAtlasManager = pInfo->mpTextureAtlasManager;
	pJobInfo->mszTexture = pAnimModelInstance->mszTexture;
	pJobInfo->mpCloudUVOffset = pInfo->mpCloudUVOffset;
	
	if( pColor )
	{
		memcpy( &pJobInfo->mColor, pColor, sizeof( tVector4 ) );
	}
	else
	{
		pJobInfo->mColor.fX = pJobInfo->mColor.fY = pJobInfo->mColor.fZ = pJobInfo->mColor.fW = 1.0f; 
	}

	pJobInfo->miCurrBatchNumPos = pModelBatch->miNumPos;
	pJobInfo->miCurrBatchNumVBOVerts = pModelBatch->miNumVerts;
	pJobInfo->miCurrBatchNumVBOIndices = pModelBatch->miNumIndices;
	
	pJobInfo->mpModelInstance = NULL;
	pJobInfo->mpAnimModelInstance = pAnimModelInstance;
	pJobInfo->mpMorphTarget = NULL;

	pJobInfo->mpOrigInfo = pJobInfo;
	pJobInfo->miIndexInBatch = pModelBatch->miNumModelJobInfo - 1;

    transformVerts( saTempNorm,
                    saTempPos,
                    saTempOrigPos,
                    pAnimModel->miNumVerts,
                    pAnimModel->miNumNormals,
                    pAnimModelInstance->maXFormNorm,
                    pAnimModelInstance->maXFormPos,
                    &totalMat,
                    pXFormMatrix,
                    &pAnimModelInstance->mRotationMatrix,
                    aiDepthTextures,
                    aLightViewProjMatrices,
                    aTotalLightMatrices );
    
    copyToVBO( pModelBatch,
               pAnimModelInstance->maVBOVerts,
               saTempPos,
               saTempNorm,
               saTempOrigPos,
               pColor,
               pAnimModel->maiVBOIndices,
               pAnimModel->miNumVBOVerts,
               pAnimModelInstance->miNumVBOIndices,
               pAnimModel->miNumVerts,
               aLightViewProjMatrices,
               pInfo->mpProjectionTextureMatrix,
               aiDepthTextures,
               pAnimModel->maVBOVertPtrs,
               pInfo->mpTextureAtlasManager,
               pAnimModelInstance->mszTexture,
			   pInfo->mpCloudUVOffset,
			   pModelBatch->miNumIndices,
			   pModelBatch->miNumVerts,
			   pInfo->mfFurDistance );   

	pModelBatch->miNumPos += pAnimModel->miNumVerts;
	pModelBatch->miNumVerts += pAnimModelInstance->miNumVBOVerts;
	pModelBatch->miNumIndices += pAnimModelInstance->miNumVBOIndices;
	++pModelBatch->miNumModels;
    pModelBatch->mbBlend = bBlend;

double fCurrTime = getCurrTime();
double fElapsed = fCurrTime - fStart;
gTotalAddToBatchTime += fElapsed;
    
	return true;
}

/*
**
*/
bool modelBatchAddMorphTarget( tModelBatchInstanceInfo const* pInfo )
{
double fStart = getCurrTime();
    
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
    
	// copy over job data to be used later
	WTFASSERT2( pModelBatch->miNumModelJobInfo >= 0 && pModelBatch->miNumModelJobInfo < sizeof( pModelBatch->maModelBatchJobInfo ) / sizeof( *(pModelBatch->maModelBatchJobInfo) ), "too many job info in batch" );
	tModelBatchJobInfo* pJobInfo = &pModelBatch->maModelBatchJobInfo[pModelBatch->miNumModelJobInfo++];

	memset( pJobInfo, 0, sizeof( tModelBatchJobInfo ) );
	pJobInfo->mpModelBatch = pModelBatch;
	pJobInfo->maResultNorm = saTempNorm;
	pJobInfo->maResultPos = saTempPos;
	pJobInfo->maResultOrigPos = saTempOrigPos;
	pJobInfo->miNumVerts = pModel->miNumVerts;
	pJobInfo->miNumNormals = pModel->miNumNormals;
	pJobInfo->maOrigNorm = pModel->maNorm;
	pJobInfo->maOrigPos = pModel->maPos;
	
	memcpy( &pJobInfo->mTotalMatrix, &totalMat, sizeof( tMatrix44 ) );

	pJobInfo->mpXFormMatrix = pXFormMatrix;
	pJobInfo->mpRotationMatrix = &pMorphTarget->mRotationMatrix;
	pJobInfo->maiDepthTextures  = aiDepthTextures;
	pJobInfo->maLightViewProjMatrices = aLightViewProjMatrices;
	
	memcpy( &pJobInfo->maTotalLightMatrices[0], &aTotalLightMatrices[0], sizeof( tMatrix44 ) );
	memcpy( &pJobInfo->maTotalLightMatrices[1], &aTotalLightMatrices[1], sizeof( tMatrix44 ) );
	memcpy( &pJobInfo->maTotalLightMatrices[2], &aTotalLightMatrices[2], sizeof( tMatrix44 ) );

	pJobInfo->maVBOVerts = pModel->maVBOVerts;
	pJobInfo->maiVBOIndices = pModel->maiVBOIndices;
	pJobInfo->miNumVBOVerts = pModel->miNumVBOVerts;
	pJobInfo->miNumVBOIndices = pModel->miNumVBOIndices;
	pJobInfo->maLightViewProjMatrices = aLightViewProjMatrices;
	pJobInfo->mpProjectionTextureMatrix = pInfo->mpProjectionTextureMatrix;
	pJobInfo->maVBOVertPtrs = pModel->maVBOVertPtrs;
	pJobInfo->mpTextureAtlasManager = pInfo->mpTextureAtlasManager;
	pJobInfo->mszTexture = pMorphTarget->mszTexture;
	pJobInfo->mpCloudUVOffset = pInfo->mpCloudUVOffset;

	memcpy( &pJobInfo->mColor, pColor, sizeof( tVector4 ) );

	pJobInfo->miCurrBatchNumPos = pModelBatch->miNumPos;
	pJobInfo->miCurrBatchNumVBOVerts = pModelBatch->miNumVerts;
	pJobInfo->miCurrBatchNumVBOIndices = pModelBatch->miNumIndices;

	pJobInfo->mpModelInstance = NULL;
	pJobInfo->mpAnimModelInstance = NULL;
	pJobInfo->mpMorphTarget = pMorphTarget;

	pJobInfo->mpOrigInfo = pJobInfo;
	pJobInfo->miIndexInBatch = pModelBatch->miNumModelJobInfo - 1;

    transformVerts( saTempNorm,
                    saTempPos,
                    saTempOrigPos,
                    pModel->miNumVerts,
                    pModel->miNumNormals,
                    pModel->maNorm,
                    pModel->maPos,
                    &totalMat,
                    pXFormMatrix,
                    &pMorphTarget->mRotationMatrix,
                    aiDepthTextures,
                    aLightViewProjMatrices,
                    aTotalLightMatrices );
    
    copyToVBO( pModelBatch,
               pModel->maVBOVerts,
               saTempPos,
               saTempNorm,
               saTempOrigPos,
               pColor,
               pModel->maiVBOIndices,
               pModel->miNumVBOVerts,
               pModel->miNumVBOIndices,
               pModel->miNumVerts,
               aLightViewProjMatrices,
               pInfo->mpProjectionTextureMatrix,
               aiDepthTextures,
               pModel->maVBOVertPtrs,
               pInfo->mpTextureAtlasManager,
               pMorphTarget->mszTexture,
			   pInfo->mpCloudUVOffset,
			   pModelBatch->miNumIndices,
			   pModelBatch->miNumVerts,
			   pInfo->mfFurDistance );

	pModelBatch->miNumPos += pModel->miNumVerts;
	pModelBatch->miNumVerts += pModel->miNumVBOVerts;
	pModelBatch->miNumIndices += pModel->miNumVBOIndices;
	++pModelBatch->miNumModels;

double fCurrTime = getCurrTime();
double fElapsed = fCurrTime - fStart;
gTotalAddToBatchTime += fElapsed;
    
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
	glUseProgram( pShader->miID );

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
	pBatch->miLightProbe = pShaderManager->getUniformLocation( pShader, "furPlacementTexture" );
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
	glUseProgram( pBatch->mpShaderProgram->miID );

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
double fStart = getTime();
    
    WTFASSERT2( pBatch->miVBO > 0, "invalid vbo" );
    
	glUseProgram( pBatch->mpShaderProgram->miID );

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


	if( pBatch->mbBlend )
	{
		glEnable( GL_BLEND );
		glCullFace( GL_BACK );
		glEnable( GL_CULL_FACE );
		glDepthMask( GL_FALSE );
	}
	else
	{
		glDisable( GL_BLEND );
		glCullFace( GL_BACK );
		glEnable( GL_CULL_FACE );
		glDepthMask( GL_TRUE );
	}

	// index buffer
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, pBatch->miIBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 
				  sizeof( int ) * pBatch->miNumIndices, 
				  pBatch->maiIndices, 
				  GL_DYNAMIC_DRAW );

double fElapsed = getTime() - fStart;
//OUTPUT( "\t\t\t\tDRAW NORMAL SCENE SETUP TIME = %f\n", fElapsed );
fStart = getTime();
    
	// draw
    glDrawElements( GL_TRIANGLES, pBatch->miNumIndices, GL_UNSIGNED_INT, (void *)0 );

fElapsed = getTime() - fStart;
//OUTPUT( "\t\t\t\tDRAW NORMAL SCENE glDrawElements() TIME = %f\n", fElapsed );
fStart = getTime();
    
	if( pBatch->mbBlend )
	{
		glDepthMask( GL_TRUE );
		glEnable( GL_DEPTH_TEST );
		glDisable( GL_BLEND );
		glEnable( GL_CULL_FACE );
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

/*
**
*/
static void transformVerts( tVector4* pResultNorm,
                            tVector4* pResultPos,
                            tVector4* pResultOrigPos,
                            int iNumVerts,
                            int iNumNormals,
                            tVector4 const* pOrigNorm,
                            tVector4 const* pOrigPos,
                            tMatrix44 const* pTotalMatrix,
                            tMatrix44 const* pXFormMatrix,
                            tMatrix44 const* pRotationMatrix,
                            GLuint const* aiDepthTextures,
                            tMatrix44 const* aLightViewProjMatrices,
                            tMatrix44 const* aTotalLightMatrices )
{
	tVector4* pXFormPos = pResultOrigPos;
    tVector4 const* pModelVertPos = pOrigPos;

    // transform vertex positions with only the model matrix
    for( int i = 0; i < iNumVerts; i++ )
    {
        Matrix44Transform( pXFormPos, pModelVertPos, pXFormMatrix );
        
        ++pXFormPos;
        ++pModelVertPos;
    }  

    // transform normals
	tVector4 const* pModelNorm = pOrigNorm;
	tVector4* pXFormNorm = pResultNorm;
    for( int i = 0; i < iNumNormals; i++ )
    {
        Matrix44Transform( pXFormNorm, pModelNorm, pRotationMatrix );
        pResultNorm->fW = 1.0f;
        
        ++pXFormNorm;
        ++pModelNorm;
    }
    
    pXFormPos = pResultPos;
    pModelVertPos = pOrigPos;
    
    // transform vertex positions
    for( int i = 0; i < iNumVerts; i++ )
    {
        Matrix44Transform( pXFormPos, pModelVertPos, pTotalMatrix );
        
        // transform position from light
        if( aiDepthTextures )
        {
            for( int j = 0; j < NUM_DEPTH_TEXTURES_PER_BATCH; j++ )
            {
                int iIndex = ( j + 1 ) * NUM_BATCH_VERT_PER_ALLOC + i;
                if( aLightViewProjMatrices && aiDepthTextures[j] > 0 )
                {
                    Matrix44Transform( &pResultPos[iIndex], pModelVertPos, &aTotalLightMatrices[j] );
                }
            }
        }
        
        ++pXFormPos;
        ++pModelVertPos;
        
    }	// for i = 0 to num vertices
}

/*
**
*/
static void copyToVBO( tModelBatch* pModelBatch,
                       tInterleaveVert const* aVBOVerts,
                       tVector4 const* pXFormPos,
                       tVector4 const* pXFormNorms,
                       tVector4 const* pXFormOrigPos,
                       tVector4 const* pColor,
                       unsigned int const* aiVBOIndices,
                       int iNumVBOVerts,
                       int iNumVBOIndices,
                       int iNumVerts,
                       tMatrix44 const* aLightViewProjMatrices,
                       tMatrix44 const* pProjectionTextureMatrix,
                       GLuint const* aiDepthTextures,
                       tInterleaveVertMap const* aVBOVertPtrs,
                       CTextureAtlasManager const* pTextureAtlasManager,
                       const char* szTexture,
					   tVector2 const* pCloudUVOffset,
					   int iCurrIndexStart,
					   int iCurrVertStart,
					   float fFurDistance )
{
    // update indices
	unsigned int* aiResultIndices = &pModelBatch->maiIndices[iCurrIndexStart];
	WTFASSERT2( pModelBatch->miNumIndexAlloc >= iNumVBOIndices + iCurrIndexStart, "invalid index" );
    
	for( int i = 0; i < iNumVBOIndices; i++ )
	{
		*aiResultIndices = aiVBOIndices[i] + iCurrVertStart;
		++aiResultIndices;
        
	}	// for i = 0 to num vbo indices

	// copy the vbo verts from original
	WTFASSERT2( pModelBatch->miNumVertAlloc >= iNumVBOVerts + iCurrVertStart, "invalid index" );
	memcpy( &pModelBatch->maInterleaveVerts[iCurrVertStart],
            aVBOVerts,
            sizeof( tInterleaveVert ) * iNumVBOVerts );

	// applied offset and scale for atlas
	float fWidthScale = 1.0f;
	float fHeightScale = 1.0f;
	if( pTextureAtlasManager )
	{
		if( strlen( szTexture ) )
		{
			tTextureAtlasInfo* pInfo = pTextureAtlasManager->getTextureAtlasInfo( szTexture );
			fWidthScale = ( pInfo->mBottomRight.fX - pInfo->mTopLeft.fX ) / (float)pInfo->mpTexture->miGLWidth;
			fHeightScale = ( pInfo->mBottomRight.fY - pInfo->mTopLeft.fY ) / (float)pInfo->mpTexture->miGLHeight;
            
			tInterleaveVert* pResultVert = &pModelBatch->maInterleaveVerts[iCurrVertStart];
			for( int i = 0; i < iNumVBOVerts; i++ )
			{
				pResultVert->mUV.fX = pResultVert->mUV.fX * fWidthScale + pInfo->mTopLeft.fX / 2048.0f;
				pResultVert->mUV.fY = pResultVert->mUV.fY * fWidthScale + pInfo->mTopLeft.fY / 2048.0f;
				++pResultVert;
			}
		}
	}

	// update vbo vertex position with the xform
	tInterleaveVert* pResultVert = &pModelBatch->maInterleaveVerts[iCurrVertStart];
	for( int i = 0; i < iNumVBOVerts; i++ )
	{
		int iPosIndex = aVBOVertPtrs[i].miPos;
		int iNormIndex = aVBOVertPtrs[i].miNorm;

		WTFASSERT2( iPosIndex >= 0 && iPosIndex < iNumVerts, "vbo ptr out of bounds" );
		
		memcpy( &pResultVert->mPos,
                &pXFormPos[iPosIndex],
                sizeof( tVector4 ) );
		
		// light position
		if( aLightViewProjMatrices && aiDepthTextures )
		{
			for( int j = 0; j < NUM_DEPTH_TEXTURES_PER_BATCH; j++ )
			{
				int iIndex = ( j + 1 ) * NUM_BATCH_VERT_PER_ALLOC + iPosIndex;
				memcpy( &pResultVert->maLightPos[j],
                        &pXFormPos[iIndex],
                        sizeof( tVector4 ) );
			}
		}
        
		memcpy( &pResultVert->mNorm,
                &pXFormNorms[iNormIndex],
                sizeof( tVector4 ) );
        
		memcpy( &pResultVert->mOrigNorm,
                &pXFormNorms[iNormIndex],
                sizeof( tVector4 ) );
        
		pResultVert->mColor.fW = pColor->fW;

		++pResultVert;
        
	}	// for i = 0 to num vbo verts
    
	// TODO: OPTIMIZE THIS
	tVector4 xformPos = { 0.0f, 0.0f, 0.0f, 1.0f };
	
	// copy over to orig vertex position
	pResultVert = &pModelBatch->maInterleaveVerts[iCurrVertStart];
	for( int i = 0; i < iNumVBOVerts; i++ )
	{
		int iPosIndex = aVBOVertPtrs[i].miPos;
		memcpy( &pResultVert->mOrigPos,
                &pXFormOrigPos[iPosIndex],
                sizeof( tVector4 ) );
        
        if( pColor )
        {
            memcpy( &pResultVert->mColor, pColor, sizeof( tVector4 ) );
		}
        else
        {
            pResultVert->mColor.fX = pResultVert->mColor.fY = pResultVert->mColor.fZ = pResultVert->mColor.fW = 1.0f;
        }
        
		// calculate projection texture uv
		if( pProjectionTextureMatrix )
		{
			Matrix44Transform( &xformPos, &pXFormOrigPos[iPosIndex], pProjectionTextureMatrix );
			float fOneOverW = 1.0f / xformPos.fW;
			pResultVert->maProjPos[0].fX = ( xformPos.fX * fOneOverW + 1.0f ) * 0.5f;
			pResultVert->maProjPos[0].fY = ( xformPos.fY * fOneOverW + 1.0f ) * 0.5f;
			pResultVert->maProjPos[0].fZ = ( xformPos.fZ * fOneOverW + 1.0f ) * 0.5f;
			pResultVert->maProjPos[0].fW = 1.0f;
			
			if( pCloudUVOffset )
			{
				pResultVert->maProjPos[0].fX += pCloudUVOffset->fX;
				pResultVert->maProjPos[0].fY += pCloudUVOffset->fY;
			}
		}
        
		++pResultVert;
	}
}

/*
**
*/
void modelBatchExecJobs( tModelBatch* pModelBatch )
{	
	tJob job;
	job.mpfnFunc = transformAndCopyToVBO;
	job.miDataSize = sizeof( tModelBatchJobInfo );
	
	// start the jobs
	for( int i = 0; i < pModelBatch->miNumModelJobInfo; i++ )
	{
		job.mpData = &pModelBatch->maModelBatchJobInfo[i];
		jobManagerAddJob( gpJobManager, &job );

	}	// for i = 0 to num model job info

	for( ;; )
	{
		// check if all the jobs are done
		bool bDone = true;
		for( int i = 0; i < pModelBatch->miNumModelJobInfo; i++ )
		{
			tModelBatchJobInfo* pBatchJobInfo = &pModelBatch->maModelBatchJobInfo[i];
			if( !pBatchJobInfo->mbFinishExec )
			{
				bDone = false;
				break;
			}

		}	// fo i = 0 to num model job info
			
		if( bDone ) 
		{
			break;
		}

		// copy to vbo
		for( int i = 0; i < pModelBatch->miNumModelJobInfo; i++ )
		{
			tModelBatchJobInfo* pBatchJobInfo = &pModelBatch->maModelBatchJobInfo[i];
			if( pBatchJobInfo->mbFinishXForm &&
				!pBatchJobInfo->mbFinishExec )
			{
				copyToVBO( pBatchJobInfo->mpModelBatch,
						   pBatchJobInfo->maVBOVerts,
						   &saTempPos[pBatchJobInfo->miCurrBatchNumVBOVerts],
						   &saTempNorm[pBatchJobInfo->miCurrBatchNumVBOVerts],
						   &saTempOrigPos[pBatchJobInfo->miCurrBatchNumVBOVerts],
						   &pBatchJobInfo->mColor,
						   pBatchJobInfo->maiVBOIndices,
						   pBatchJobInfo->miNumVBOVerts,
						   pBatchJobInfo->miNumVBOIndices,
						   pBatchJobInfo->miNumVerts,
						   pBatchJobInfo->maLightViewProjMatrices,
						   pBatchJobInfo->mpProjectionTextureMatrix,
						   pBatchJobInfo->maiDepthTextures,
						   pBatchJobInfo->maVBOVertPtrs,
						   pBatchJobInfo->mpTextureAtlasManager,
						   pBatchJobInfo->mszTexture,
						   pBatchJobInfo->mpCloudUVOffset,
						   pBatchJobInfo->miCurrBatchNumVBOIndices,
						   pBatchJobInfo->miCurrBatchNumVBOVerts,
						   pBatchJobInfo->mfFurDistance );

				pBatchJobInfo->mbFinishExec = true;
			}

		}	// fo i = 0 to num model job info

	}	// for ;;
}

/*
**
*/
static void transformAndCopyToVBO( void* pData, void* pJobDebugData )
{
	int iJobQueueIndex = ((tJobDebugData *)pJobDebugData)->miQueueIndex;
	int iNumInQueue = ((tJobDebugData *)pJobDebugData)->miNumInQueue;

	tModelBatchJobInfo* pBatchJobInfo = (tModelBatchJobInfo *)pData;
	
	int iFreeIndex = pBatchJobInfo->miCurrBatchNumVBOVerts;

	tVector4* pResultNorm = &saTempNorm[iFreeIndex];
	tVector4* pResultPos = &saTempPos[iFreeIndex];
	tVector4* pResultOrigPos = &saTempOrigPos[iFreeIndex];

	//OUTPUT( "\t\t\tSTART transformAndCopyToVBO batch = %d queue index = %d num in queue = %d\n", 
	//		pBatchJobInfo->miIndexInBatch,
	//		iJobQueueIndex,
	//		iNumInQueue );

	transformVerts( pResultNorm,
                    pResultPos,
                    pResultOrigPos,
                    pBatchJobInfo->miNumVerts,
                    pBatchJobInfo->miNumNormals,
                    pBatchJobInfo->maOrigNorm,
                    pBatchJobInfo->maOrigPos,
                    &pBatchJobInfo->mTotalMatrix,
                    pBatchJobInfo->mpXFormMatrix,
                    pBatchJobInfo->mpRotationMatrix,
                    pBatchJobInfo->maiDepthTextures,
                    pBatchJobInfo->maLightViewProjMatrices,
                    pBatchJobInfo->maTotalLightMatrices );

	pBatchJobInfo->mpOrigInfo->mbFinishXForm = true;
	
	OUTPUT( "\t\t\tEND transformAndCopyToVBO batch = %d queue index = %d num in queue = %d\n", 
			pBatchJobInfo->miIndexInBatch,
			iJobQueueIndex,
			iNumInQueue );

    /*copyToVBO( pBatchJobInfo->mpModelBatch,
               pBatchJobInfo->maVBOVerts,
               pResultPos,
               pResultNorm,
               pResultOrigPos,
               &pBatchJobInfo->mColor,
               pBatchJobInfo->maiVBOIndices,
               pBatchJobInfo->miNumVBOVerts,
               pBatchJobInfo->miNumVBOIndices,
               pBatchJobInfo->miNumVerts,
               pBatchJobInfo->maLightViewProjMatrices,
               pBatchJobInfo->mpProjectionTextureMatrix,
               pBatchJobInfo->maiDepthTextures,
               pBatchJobInfo->maVBOVertPtrs,
               pBatchJobInfo->mpTextureAtlasManager,
               pBatchJobInfo->mszTexture,
			   pBatchJobInfo->mpCloudUVOffset,
			   pBatchJobInfo->miCurrBatchNumVBOIndices,
			   pBatchJobInfo->miCurrBatchNumVBOVerts );*/
}

/*
**
*/
static void extrudeVerts( tModelBatch* pModelBatch,
						  tModel const* pModel,
						  tModelInstance const* pModelInstance,
						  tMatrix44 const* pTotalMatrix,
						  tMatrix44 const* pXFormMatrix,
						  tVector4 const* pColor,
						  float fFurDistance )
{
	unsigned int const* aiIndices = pModel->maiVBOIndices;
	int iNumIndices = pModel->miNumVBOIndices;
	int iNumVerts = pModel->miNumVBOVerts;

	tInterleaveVert const* aOrigVerts = pModel->maVBOVerts;
	tInterleaveVertMap const* aVBOVertPtrs = pModel->maVBOVertPtrs;

	int iCurrVertStart = pModelBatch->miNumVerts;
	int iCurrIndexStart = pModelBatch->miNumIndices;
	
	tVector4 const* aOrigPos = pModel->maPos;
	tVector4 const* aOrigNorm = pModel->maNorm;

	tMatrix44 const* pRotationMatrix = &pModelInstance->mRotationMatrix;

	 // update indices
	unsigned int* aiResultIndices = &pModelBatch->maiIndices[iCurrIndexStart];
	WTFASSERT2( pModelBatch->miNumIndexAlloc >= iNumIndices + iCurrIndexStart, "invalid index" );
    
	for( int i = 0; i < iNumIndices; i++ )
	{
		*aiResultIndices = aiIndices[i] + iCurrVertStart;
		++aiResultIndices;
        
	}	// for i = 0 to num vbo indices

	// copy the vbo verts from original
	WTFASSERT2( pModelBatch->miNumVertAlloc >= iNumVerts + iCurrVertStart, "invalid index" );
	memcpy( &pModelBatch->maInterleaveVerts[iCurrVertStart],
            aOrigVerts,
            sizeof( tInterleaveVert ) * iNumVerts );

	tInterleaveVert* pResultVert = &pModelBatch->maInterleaveVerts[iCurrVertStart];
	tInterleaveVertMap const* pMapping = aVBOVertPtrs;

	for( int i = 0; i < iNumVerts; i++ )
	{
		int iPosIndex = pMapping->miPos;
		int iNormIndex = pMapping->miNorm;

		tVector4 const* pOrigPos = &aOrigPos[iPosIndex];
		tVector4 const* pOrigNorm = &aOrigNorm[iNormIndex];
		
		tVector4 extrudePos = { 0.0f, 0.0f, 0.0f, 1.0f };

		extrudePos.fX = pOrigPos->fX + pOrigNorm->fX * fFurDistance;
		extrudePos.fY = pOrigPos->fY + pOrigNorm->fY * fFurDistance; // - powf( fLayer, 3.0f );
		extrudePos.fZ = pOrigPos->fZ + pOrigNorm->fZ * fFurDistance;
		
		Matrix44Transform( &pResultVert->mPos,
						   &extrudePos,
						   pTotalMatrix );

		Matrix44Transform( &pResultVert->mNorm,
						   pOrigNorm,
						   pXFormMatrix );

		Matrix44Transform( &pResultVert->mOrigPos,
						   &extrudePos,
						   pRotationMatrix );

		memcpy( &pResultVert->mColor, pColor, sizeof( tVector4 ) );

		++pResultVert;
		++pMapping;
	
	}	// for i = 0 to num verts
}