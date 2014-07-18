#include "modelbatchmanager.h"
#include "shadermanager.h"
#include "timeutil.h"

#define NUM_BATCH_INIT 100

/*
**
*/
void modelBatchManagerInit( tModelBatchManager* pBatchManager )
{
	pBatchManager->miNumBatches = 0;
	pBatchManager->miNumBatchAlloc = NUM_BATCH_INIT;
	pBatchManager->maBatches = (tModelBatch *)MALLOC( sizeof( tModelBatch ) * pBatchManager->miNumBatchAlloc );
	memset( pBatchManager->maBatches, 0, sizeof( tModelBatch ) * pBatchManager->miNumBatchAlloc );

    for( unsigned int i = 0; i < pBatchManager->miNumBatchAlloc; i++ )
    {
        pBatchManager->maBatches[i].maiTextureID[0] = (GLuint)-1;
        pBatchManager->maBatches[i].maiTextureID[1] = (GLuint)-1;
        pBatchManager->maBatches[i].maiTextureID[2] = (GLuint)-1;
        pBatchManager->maBatches[i].maiTextureID[3] = (GLuint)-1;
    }
    
	pBatchManager->miCurrBatch = 0;
}

/*
**
*/
void modelBatchManagerRelease( tModelBatchManager* pBatchManager )
{
	FREE( pBatchManager->maBatches );
}

/*
**
*/
double gfTotalSearchTime = 0.0;
double gfTotalAddModelInstanceTime = 0.0;
void modelBatchManagerAddModelInstance( tModelBatchManagerInfo const* pInfo )
{
//double fTotalStart = getCurrTime();
//double fStart = getCurrTime();

	tModelBatchManager* pBatchManager = pInfo->mpBatchManager;
	
	// change to use texture atlas if provided with an atlas manager
	tModelBatchInstanceInfo modelBatchInfo;
	if( pInfo->mpTextureAtlasManager )
	{
		if( strlen( pInfo->mpModelInstance->mszTexture ) )
		{
			tTextureAtlasInfo* pAtlasInfo = pInfo->mpTextureAtlasManager->getTextureAtlasInfo( pInfo->mpModelInstance->mszTexture );
			CTextureManager::instance()->loadTextureInMem( pAtlasInfo->mpTexture, getTime() );
			pInfo->maiTextureID[0] = pAtlasInfo->mpTexture->miID;
		}
	}

	bool bFound = false;
	tModelBatch* pToInsert = NULL;
	for( unsigned int i = 0; i < pBatchManager->miNumBatchAlloc; i++ )
	{	
		// check if the albedo and draw texture atlas are the same
		tModelBatch const* pModelBatch = &pBatchManager->maBatches[i];
        if( pInfo->maiTextureID )
        {
            if( pModelBatch->maiTextureID[0] == pInfo->maiTextureID[0] &&
                pModelBatch->maiTextureID[1] == pInfo->maiTextureID[1] &&
                pModelBatch->maiTextureID[2] == pInfo->maiTextureID[2] &&
                pModelBatch->maiTextureID[3] == pInfo->maiTextureID[3] &&
                pModelBatch->mbBlend == pInfo->mbBlend && 
                pModelBatch->mpShaderProgram == pInfo->mpShaderProgram &&
                pModelBatch->miNumIndices + pInfo->mpModelInstance->mpModel->miNumVBOIndices < NUM_BATCH_VERT_PER_ALLOC )
            {
                pToInsert = &pBatchManager->maBatches[i];
                break;
            }
        }
        else
        {
            if( pModelBatch->mpShaderProgram == pInfo->mpShaderProgram &&
                pModelBatch->miNumIndices + pInfo->mpModelInstance->mpModel->miNumVBOIndices < NUM_BATCH_VERT_PER_ALLOC )
            {
                pToInsert = &pBatchManager->maBatches[i];
                break;
            }
        }
	}

//double fElapsed = getCurrTime() - fStart;
//gfTotalSearchTime += fElapsed;

	modelBatchInfo.mpModelBatch = NULL;
	modelBatchInfo.mpModelInstance = pInfo->mpModelInstance;
	modelBatchInfo.mpXFormMatrix = &pInfo->mpModelInstance->mXFormMat;
	modelBatchInfo.mpViewMatrix = pInfo->mpViewMatrix;
	modelBatchInfo.mpViewProjMatrix = pInfo->mpViewProjMatrix;
	modelBatchInfo.maLightViewProjMatrices = pInfo->maLightViewProjMatrices;
	modelBatchInfo.maiTextureID = pInfo->maiTextureID;
	modelBatchInfo.maiDepthTextures = pInfo->maiDepthTextureID;
	modelBatchInfo.mpColor = pInfo->mpColor;
	modelBatchInfo.mbBlend = pInfo->mbBlend;
	modelBatchInfo.mpProjectionTextureMatrix = pInfo->mProjectionTextureMatrix;
	modelBatchInfo.mpTextureAtlasManager = pInfo->mpTextureAtlasManager;
	modelBatchInfo.mpCloudUVOffset = pInfo->mpCloudUVOffset;
	modelBatchInfo.mfFurDistance = pInfo->mfFurDistance;

	if( pToInsert )
	{
		modelBatchInfo.mpModelBatch = pToInsert;

		// found the batch
		bFound = modelBatchAddModelInstance( &modelBatchInfo );
	}

	if( !bFound )
	{
		// initial new batch
		if( pBatchManager->miNumBatches >= pBatchManager->miNumBatchAlloc )
		{ 
			pBatchManager->miNumBatchAlloc += NUM_BATCH_INIT;
			pBatchManager->maBatches = (tModelBatch *)REALLOC( pBatchManager->maBatches, sizeof( tModelBatch ) * pBatchManager->miNumBatchAlloc );
			for( unsigned int i = pBatchManager->miNumBatchAlloc - NUM_BATCH_INIT; i < pBatchManager->miNumBatchAlloc; i++ )
			{
				memset( &pBatchManager->maBatches[i], 0, sizeof( tModelBatch ) );
            }
		}

		if( pBatchManager->maBatches[pBatchManager->miNumBatches].maInterleaveVerts == NULL )
		{
			modelBatchInit( &pBatchManager->maBatches[pBatchManager->miNumBatches] );
			
			// need to initialize later
			pBatchManager->maNeedInitBatches.push_back( pBatchManager->miNumBatches );
		}

		pBatchManager->maBatches[pBatchManager->miNumBatches].mpModel = pInfo->mpModelInstance->mpModel;
		modelBatchInfo.mpModelBatch = &pBatchManager->maBatches[pBatchManager->miNumBatches];
		bool bAdded = modelBatchAddModelInstance( &modelBatchInfo );

		pBatchManager->maBatches[pBatchManager->miNumBatches].mbBlend = pInfo->mbBlend;
		pBatchManager->maBatches[pBatchManager->miNumBatches].mpShaderProgram = pInfo->mpShaderProgram;

		// filled up, need new batch
		if( !bAdded )
		{
			++pBatchManager->miNumBatches;
			if( pBatchManager->maBatches[pBatchManager->miNumBatches].maInterleaveVerts == NULL )
			{
				modelBatchInit( &pBatchManager->maBatches[pBatchManager->miNumBatches] );

				// need to initialize later
				pBatchManager->maNeedInitBatches.push_back( pBatchManager->miNumBatches );
			}

			pBatchManager->maBatches[pBatchManager->miNumBatches].mpModel = pInfo->mpModelInstance->mpModel;
			modelBatchInfo.mpModelBatch = &pBatchManager->maBatches[pBatchManager->miNumBatches];
			modelBatchAddModelInstance( &modelBatchInfo );

			pBatchManager->maBatches[pBatchManager->miNumBatches].mbBlend = pInfo->mbBlend;
			pBatchManager->maBatches[pBatchManager->miNumBatches].mpShaderProgram = pInfo->mpShaderProgram;

		}	// if create new batch

		++pBatchManager->miNumBatches;

	}	// if !found


//gfTotalAddModelInstanceTime += ( getCurrTime() - fTotalStart );

}

/*
 **
 */
void modelBatchManagerAddAnimModelInstance( tModelBatchManagerInfo const* pInfo )
{
	tModelBatchManager* pBatchManager = pInfo->mpBatchManager;
    tAnimModel const* pAnimModel = pInfo->mpAnimModelInstance->mpAnimModel;
	
	// change to use texture atlas if provided with an atlas manager
	if( pInfo->mpTextureAtlasManager )
	{
		if( strlen( pInfo->mpAnimModelInstance->mszTexture ) )
		{
			tTextureAtlasInfo* pAtlasInfo = pInfo->mpTextureAtlasManager->getTextureAtlasInfo( pInfo->mpAnimModelInstance->mszTexture );
			CTextureManager::instance()->loadTextureInMem( pAtlasInfo->mpTexture, getTime() );
			pInfo->maiTextureID[0] = pAtlasInfo->mpTexture->miID;
		}
	}

	bool bFound = false;
	tModelBatch* pToInsert = NULL;
	for( unsigned int i = 0; i < pBatchManager->miNumBatchAlloc; i++ )
	{
		// check if the albedo and draw texture atlas are the same
        if( pBatchManager->maBatches[i].maiTextureID[0] == pInfo->maiTextureID[0] &&
            pBatchManager->maBatches[i].maiTextureID[2] == pInfo->maiTextureID[2] )
        {
            pToInsert = &pBatchManager->maBatches[i];
            break;
        }
	}
    
    //double fElapsed = getCurrTime() - fStart;
    //gfTotalSearchTime += fElapsed;
    
	tModelBatchInstanceInfo modelBatchInfo;
	memset( &modelBatchInfo, 0, sizeof( tModelBatchInstanceInfo ) );

	modelBatchInfo.mpAnimModelInstance = pInfo->mpAnimModelInstance;
	modelBatchInfo.mpViewMatrix = pInfo->mpViewMatrix;
	modelBatchInfo.mpViewProjMatrix = pInfo->mpViewProjMatrix;
	modelBatchInfo.maLightViewProjMatrices = pInfo->maLightViewProjMatrices;
	modelBatchInfo.maiTextureID = pInfo->maiTextureID;
	modelBatchInfo.maiDepthTextures = pInfo->maiDepthTextureID;
	modelBatchInfo.mbBlend = pInfo->mbBlend;
	modelBatchInfo.mpXFormMatrix = &pInfo->mpAnimModelInstance->mXFormMat;
	modelBatchInfo.mpProjectionTextureMatrix = pInfo->mProjectionTextureMatrix;
	modelBatchInfo.mpTextureAtlasManager = pInfo->mpTextureAtlasManager;
	modelBatchInfo.mpCloudUVOffset = pInfo->mpCloudUVOffset;

	if( pToInsert )
	{
		modelBatchInfo.mpModelBatch = pToInsert;

		// found the batch
		bFound = modelBatchAddAnimModel( &modelBatchInfo );
	}
    
	if( !bFound )
	{
		// initial new batch
		if( pBatchManager->miNumBatches >= pBatchManager->miNumBatchAlloc )
		{
			pBatchManager->miNumBatchAlloc += NUM_BATCH_INIT;
			pBatchManager->maBatches = (tModelBatch *)REALLOC( pBatchManager->maBatches, sizeof( tModelBatch ) * pBatchManager->miNumBatchAlloc );
			for( unsigned int i = pBatchManager->miNumBatchAlloc - NUM_BATCH_INIT; i < pBatchManager->miNumBatchAlloc; i++ )
			{
				memset( &pBatchManager->maBatches[i], 0, sizeof( tModelBatch ) );
			}
		}
        
		if( pBatchManager->maBatches[pBatchManager->miNumBatches].maInterleaveVerts == NULL )
		{
			modelBatchInit( &pBatchManager->maBatches[pBatchManager->miNumBatches] );
			
			// need to initialize later
			pBatchManager->maNeedInitBatches.push_back( pBatchManager->miNumBatches );
		}
        
        OUTPUT( "ADD MODEL: %s TO BATCH\n", pInfo->mpAnimModelInstance->mszName );
        
		pBatchManager->maBatches[pBatchManager->miNumBatches].mpAnimModel = pAnimModel;
		modelBatchInfo.mpModelBatch = &pBatchManager->maBatches[pBatchManager->miNumBatches];
		bool bAdded = modelBatchAddAnimModel( &modelBatchInfo );
		
		// filled up, need new batch
		if( !bAdded )
		{
			++pBatchManager->miNumBatches;
			if( pBatchManager->maBatches[pBatchManager->miNumBatches].maInterleaveVerts == NULL )
			{
				modelBatchInit( &pBatchManager->maBatches[pBatchManager->miNumBatches] );
                
				// need to initialize later
				pBatchManager->maNeedInitBatches.push_back( pBatchManager->miNumBatches );
			}
            
			pBatchManager->maBatches[pBatchManager->miNumBatches].mpAnimModel = pAnimModel;
			modelBatchInfo.mpModelBatch = &pBatchManager->maBatches[pBatchManager->miNumBatches];
			modelBatchAddAnimModel( &modelBatchInfo );
            
		}	// if create new batch
        
		++pBatchManager->miNumBatches;
        
	}	// if !found
}

/*
**
*/
void modelBatchManagerAddMorphTarget( tModelBatchManagerInfo const* pInfo )
{
    //double fTotalStart = getCurrTime();
    //double fStart = getCurrTime();
    
	tModelBatchManager* pBatchManager = pInfo->mpBatchManager;
    tModelBatchInstanceInfo modelBatchInfo;
    memset( &modelBatchInfo, 0, sizeof( tModelBatchInstanceInfo ) );

	// change to use texture atlas if provided with an atlas manager
	if( pInfo->mpTextureAtlasManager )
	{
		if( strlen( pInfo->mpMorphTarget->mszTexture ) )
		{
			tTextureAtlasInfo* pAtlasInfo = pInfo->mpTextureAtlasManager->getTextureAtlasInfo( pInfo->mpMorphTarget->mszTexture );
			CTextureManager::instance()->loadTextureInMem( pAtlasInfo->mpTexture, getTime() );
			pInfo->maiTextureID[0] = pAtlasInfo->mpTexture->miID;
		}
	}


	bool bFound = false;
	tModelBatch* pToInsert = NULL;
	for( unsigned int i = 0; i < pBatchManager->miNumBatchAlloc; i++ )
	{
		// check if the albedo and draw texture atlas are the same
		tModelBatch const* pModelBatch = &pBatchManager->maBatches[i];
        if( pModelBatch->maiTextureID[0] == pInfo->maiTextureID[0] &&
            pModelBatch->maiTextureID[1] == pInfo->maiTextureID[1] &&
            pModelBatch->maiTextureID[2] == pInfo->maiTextureID[2] &&
            pModelBatch->mbBlend == pInfo->mbBlend )
        {
            pToInsert = &pBatchManager->maBatches[i];
            break;
        }
	}
    
    //double fElapsed = getCurrTime() - fStart;
    //gfTotalSearchTime += fElapsed;
    
	modelBatchInfo.mpModelBatch = NULL;
	modelBatchInfo.mpMorphTarget = pInfo->mpMorphTarget;
	modelBatchInfo.mpXFormMatrix = &pInfo->mpMorphTarget->mXFormMatrix;
    modelBatchInfo.mpViewMatrix = pInfo->mpViewMatrix;
	modelBatchInfo.mpViewProjMatrix = pInfo->mpViewProjMatrix;
	modelBatchInfo.maLightViewProjMatrices = pInfo->maLightViewProjMatrices;
	modelBatchInfo.maiTextureID = pInfo->maiTextureID;
	modelBatchInfo.maiDepthTextures = pInfo->maiDepthTextureID;
	modelBatchInfo.mpColor = pInfo->mpColor;
	modelBatchInfo.mbBlend = pInfo->mbBlend;
	modelBatchInfo.mpProjectionTextureMatrix = pInfo->mProjectionTextureMatrix;
	modelBatchInfo.mpTextureAtlasManager = pInfo->mpTextureAtlasManager;
	modelBatchInfo.mpCloudUVOffset = pInfo->mpCloudUVOffset;

	if( pToInsert )
	{
		modelBatchInfo.mpModelBatch = pToInsert;
        
		// found the batch
		bFound = modelBatchAddMorphTarget( &modelBatchInfo );
	}
    
	if( !bFound )
	{
		// initial new batch
		if( pBatchManager->miNumBatches >= pBatchManager->miNumBatchAlloc )
		{
			pBatchManager->miNumBatchAlloc += NUM_BATCH_INIT;
			pBatchManager->maBatches = (tModelBatch *)REALLOC( pBatchManager->maBatches, sizeof( tModelBatch ) * pBatchManager->miNumBatchAlloc );
			for( unsigned int i = pBatchManager->miNumBatchAlloc - NUM_BATCH_INIT; i < pBatchManager->miNumBatchAlloc; i++ )
			{
				memset( &pBatchManager->maBatches[i], 0, sizeof( tModelBatch ) );
			}
		}
        
		if( pBatchManager->maBatches[pBatchManager->miNumBatches].maInterleaveVerts == NULL )
		{
			modelBatchInit( &pBatchManager->maBatches[pBatchManager->miNumBatches] );
			
			// need to initialize later
			pBatchManager->maNeedInitBatches.push_back( pBatchManager->miNumBatches );
		}
        
		modelBatchInfo.mpModelBatch = &pBatchManager->maBatches[pBatchManager->miNumBatches];
		bool bAdded = modelBatchAddMorphTarget( &modelBatchInfo );
        
		pBatchManager->maBatches[pBatchManager->miNumBatches].mbBlend = pInfo->mbBlend;
		
		// filled up, need new batch
		if( !bAdded )
		{
			++pBatchManager->miNumBatches;
			if( pBatchManager->maBatches[pBatchManager->miNumBatches].maInterleaveVerts == NULL )
			{
				modelBatchInit( &pBatchManager->maBatches[pBatchManager->miNumBatches] );
                
				// need to initialize later
				pBatchManager->maNeedInitBatches.push_back( pBatchManager->miNumBatches );
			}
            
			pBatchManager->maBatches[pBatchManager->miNumBatches].mpModel = pInfo->mpModelInstance->mpModel;
			modelBatchInfo.mpModelBatch = &pBatchManager->maBatches[pBatchManager->miNumBatches];
			modelBatchAddMorphTarget( &modelBatchInfo );
            
			pBatchManager->maBatches[pBatchManager->miNumBatches].mbBlend = pInfo->mbBlend;
            
		}	// if create new batch
        
		++pBatchManager->miNumBatches;
        
	}	// if !found
    
    
    //gfTotalAddModelInstanceTime += ( getCurrTime() - fTotalStart );
    
}

/*
**
*/
void modelBatchManagerResetBatches( tModelBatchManager* pBatchManager )
{	
	for( int i = 0; i < (int)pBatchManager->miNumBatchAlloc; i++ )
	{
		tModelBatch* pBatch = &pBatchManager->maBatches[i];
		pBatch->miNumVerts = 0;
		pBatch->miNumPos = 0;
		pBatch->miNumModels = 0;
		pBatch->miNumIndices = 0;
		pBatch->miNumModelJobInfo = 0;
	}

	pBatchManager->miCurrBatch = 0;
}

/*
**
*/
void modelBatchManagerDraw( tModelBatchManager* pBatchManager, GLint iShader )
{
	glUseProgram( iShader );

	// init gl
	std::vector<int>::iterator glInitIter = pBatchManager->maNeedInitBatches.begin();
	for( ; glInitIter != pBatchManager->maNeedInitBatches.end(); ++glInitIter )
	{
		int iBatchIndex = *glInitIter;
		tModelBatch* pBatch = &pBatchManager->maBatches[iBatchIndex];
		modelBatchSetupGL( pBatch, iShader );
	}

	pBatchManager->maNeedInitBatches.clear();
	for( unsigned int i = 0; i < pBatchManager->miNumBatches; i++ )
	{
		tModelBatch* pBatch = &pBatchManager->maBatches[i];
		if( pBatch->miNumIndices > 0 )
		{
			modelBatchDraw( pBatch,
							pBatch->maiTextureID[0] );
		}
	}

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

/*
**
*/
void modelBatchManagerDrawNormal( tModelBatchManager* pBatchManager, 
								  tVector4 const* pLightDir,
								  tVector4 const* pCameraPosition )
{
double fStart = getTime();
    
	// init gl
	std::vector<int>::iterator glInitIter = pBatchManager->maNeedInitBatches.begin();
	for( ; glInitIter != pBatchManager->maNeedInitBatches.end(); ++glInitIter )
	{
		int iBatchIndex = *glInitIter;
		tModelBatch* pBatch = &pBatchManager->maBatches[iBatchIndex];
		modelBatchSetupNormalGL( pBatch, pBatch->mpShaderProgram );
	}
    
double fElapsed = getTime() - fStart;
//OUTPUT( "\t\tDRAW NORMAL SETUP TIME = %f\n", fElapsed );
fStart = getTime();
    
	pBatchManager->maNeedInitBatches.clear();
	for( unsigned int i = 0; i < pBatchManager->miNumBatches; i++ )
	{
        //double fBatchStart = getTime();
        
		tModelBatch* pBatch = &pBatchManager->maBatches[i];
		if( pBatch->miNumIndices > 0 )
		{
			modelBatchDrawNormal( pBatch,
								  pBatch->maiTextureID,
								  pLightDir,
								  pCameraPosition );
		}
        
        //double fBatchElapsed = getTime() - fBatchStart;
        //OUTPUT( "\t\t\tBATCH %d TIME = %f num verts = %d\n", i, fBatchElapsed, pBatch->miNumIndices );
	}
    
fElapsed = getTime() - fStart;
//OUTPUT( "\t\tDRAW NORMAL TIME = %f num batches = %d\n", fElapsed, pBatchManager->miNumBatches );
fStart = getTime();

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

/*
**
*/
void modelBatchManagerDrawShadow( tModelBatchManager* pBatchManager, 
								  tShaderProgram const* pShaderProgram, 
								  tVector4 const* pLightDir,
								  tVector4 const* pCameraPosition,
								  tVector4 const* pCameraDir )
{
	GLint iShader = pShaderProgram->miID;
	glUseProgram( iShader );
	
	// init gl
	std::vector<int>::iterator glInitIter = pBatchManager->maNeedInitBatches.begin();
	for( ; glInitIter != pBatchManager->maNeedInitBatches.end(); ++glInitIter )
	{
		int iBatchIndex = *glInitIter;
		tModelBatch* pBatch = &pBatchManager->maBatches[iBatchIndex];
		modelBatchSetupShadowGL( pBatch, pShaderProgram );
	}

	pBatchManager->maNeedInitBatches.clear();
	for( unsigned int i = 0; i < pBatchManager->miNumBatches; i++ )
	{
		tModelBatch* pBatch = &pBatchManager->maBatches[i];
		if( pBatch->miNumIndices > 0 )
		{
			modelBatchDrawShadow( pBatch,
								  pBatch->maiTextureID,
								  pBatch->maiDepthTextureID,
								  pLightDir,
								  pCameraPosition,
								  pCameraDir );
		}
	}

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

}

/*
**
*/
void modelBatchManagerReInitGL( tModelBatchManager* pBatchManager )
{
	for( unsigned int i = 0; i < pBatchManager->miNumBatches; i++ )
	{
		pBatchManager->maNeedInitBatches.push_back( i );

	}	// for i = 0 to num batches
}

/*
**
*/
void modelBatchManagerExecJobs( tModelBatchManager* pBatchManager )
{
	for( unsigned int i = 0; i < pBatchManager->miNumBatches; i++ )
	{
		modelBatchExecJobs( &pBatchManager->maBatches[i] );
	}
}