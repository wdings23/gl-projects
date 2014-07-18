#include "assetloader.h"
#include "jobmanager.h"
#include "model.h"
#include "texturemanager.h"

#include <vector>

struct LoadModelJobData
{
    tModel*     mpModel;
};

typedef struct LoadModelJobData tLoadModelJobData;

struct LoadTextureJobData
{
	char		mszTextureName[256];
};

typedef struct LoadTextureJobData tLoadTextureJobData;

struct MainQueueJobData
{
	void*		mpData;
	int			miType;
};

typedef struct MainQueueJobData tMainQueueJobData;

struct TextureMainQueueData
{
	unsigned char*		macImage;
	int					miWidth;
	int					miHeight;
	char				mszName[256];
};

typedef struct TextureMainQueueData tTextureMainQueueData;

std::vector<tMainQueueJobData *>			gMainThreadQueue;

static pthread_mutex_t						sMemLock;
static pthread_mutex_t						sQueueLock;

static void loadModel( void* pData, void* pDebugData );
static void loadTexture( void* pData, void* pDebugData );

/*
**
*/
void initAssetLoad( void )
{
	pthread_mutex_init( &sMemLock, NULL );
	pthread_mutex_init( &sQueueLock, NULL );
}

/*
**
*/
void assetLoad( void* pData, int iType )
{
	if( iType == ASSET_MODEL )
	{
		tJob loadModelJob;
		tLoadModelJobData jobData = { (tModel *)pData };
		loadModelJob.mpData = &jobData;
		loadModelJob.miDataSize = sizeof( tLoadModelJobData );
		loadModelJob.mpfnFunc = loadModel;

		jobManagerAddJob( gpJobManager, &loadModelJob );
	}
	else if( iType == ASSET_TEXTURE )
	{
		tJob loadTextureJob;
		tLoadTextureJobData jobData;
		strncpy( jobData.mszTextureName, (const char *)pData, sizeof( jobData.mszTextureName ) );

		loadTextureJob.mpData = &jobData;
		loadTextureJob.miDataSize = sizeof( tLoadTextureJobData );
		loadTextureJob.mpfnFunc = loadTexture;

		jobManagerAddJob( gpJobManager, &loadTextureJob );
	}
}

/*
**
*/
void processAssetOnMainThread( void )
{
	int iIndex = 0;

	pthread_mutex_lock( &sQueueLock );
	std::vector<tMainQueueJobData *>::iterator it = gMainThreadQueue.begin();
	for( ; it != gMainThreadQueue.end(); ++it )
	{
		tMainQueueJobData* pJobData = *it;
		if( pJobData->miType == ASSET_MODEL )
		{
			// using batch
#if 0
			// model
			tModel* pModel = (tModel *)pJobData->mpData;
			modelSetupGL( pModel );
#endif // #if 0
		}
		else if( pJobData->miType == ASSET_TEXTURE )
		{
			// texture
			char* szName = (char *)pJobData->mpData;
			//OUTPUT( "process on main thread name = %s\n", szName );

			CTextureManager::instance()->registerTexture( szName );
			++iIndex;
		}

		FREE( pJobData );

		++iIndex;
	}

	gMainThreadQueue.clear();
	pthread_mutex_unlock( &sQueueLock );
}

/*
**
*/
static void queueOnMainThread( tMainQueueJobData* pData )
{
	pthread_mutex_lock( &sQueueLock );
	gMainThreadQueue.push_back( pData );
	pthread_mutex_unlock( &sQueueLock );
}

/*
**
*/
static void loadModel( void* pData, void* pDebugData )
{
	tModel* pModel = ( (tLoadModelJobData *)pData )->mpModel;
    if( pModel->miLoadState != MODEL_LOADSTATE_LOADED )
	{
        modelLoad( pModel, pModel->mszFileName );
        
		// queue to process on main thread
		pthread_mutex_lock( &sMemLock );
		tMainQueueJobData* pMainQueueData = (tMainQueueJobData *)MALLOC( sizeof( tMainQueueJobData ) );
		pthread_mutex_unlock( &sMemLock );

		pMainQueueData->miType = ASSET_MODEL;
		pMainQueueData->mpData = pModel;

		queueOnMainThread( pMainQueueData );
    }
}

/*
**
*/
static void loadTexture( void* pData, void* pDebugData )
{
	const char* szFileName = (const char *)pData;
	
	tTexture* pTexture = CTextureManager::instance()->getTexture( szFileName );
	
	if( pTexture == NULL )
	{
		OUTPUT( "load %s on main thread\n", szFileName );
		
		// check duplicates
		bool bFound = false;
		std::vector<tMainQueueJobData *>::iterator it = gMainThreadQueue.begin();
		for( ; it != gMainThreadQueue.end(); ++it )
		{
			tMainQueueJobData* pJobData = *it;

			// texture
			char* szName = (char *)pJobData->mpData;
			if( !strcmp( szName, szFileName ) )
			{
				bFound = true;
				break;
			}
		}
		
		if( !bFound )
		{
			pthread_mutex_lock( &sMemLock );
			tMainQueueJobData* pMainQueueData = (tMainQueueJobData *)MALLOC( sizeof( tMainQueueJobData ) );
			pthread_mutex_unlock( &sMemLock );

			pMainQueueData->miType = ASSET_TEXTURE;
			pMainQueueData->mpData = (char *)szFileName;

			// queue to process on main thread
			queueOnMainThread( pMainQueueData );
		}
	}
}