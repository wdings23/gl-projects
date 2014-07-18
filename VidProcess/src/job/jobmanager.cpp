#include "jobmanager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined( ANDROID )
#include <unistd.h>
#endif // ANDROID

#if defined( IOS )
#include <sys/sysctl.h>
#endif // IOS

#define NUM_QUEUE_SLOTS_PER_ALLOC 250

tJobManager gJobManager;
tJobManager* gpJobManager = &gJobManager;

static void* jobManagerRunInThread( void* pData );

/*
**
*/
static void* execWorkFunc( void* pData )
{
	tJobData const* pJobData = (tJobData const*)pData;
	pthread_mutex_t* pWaitMutex = pJobData->mpWaitMutex;
	pthread_cond_t* pCondition = pJobData->mpCondition;
    int* piRunning = pJobData->miRunning;
    
    
	for( ;; )
	{
		// wait for activation
		pthread_mutex_lock( pWaitMutex );

        // worker is FREEd
        *piRunning = 0;
        while( gpJobManager->maiRunning[pJobData->miWorkerID] == 0 )
        {
            pthread_cond_wait( pCondition, pWaitMutex );
		}
        
		pthread_mutex_unlock( pWaitMutex );

		// worker is busy
		WTFASSERT2( pJobData->miWorkerID >= 0 && pJobData->miWorkerID < (int)gpJobManager->miNumWorkers, "invalid worker id" );
		*piRunning = 1;
        
		// execute job
		pthread_mutex_lock( &gpJobManager->mAddJobMutex );
		tJob* pJob = &gpJobManager->maJobs[pJobData->miJobQueueIndex];
		WTFASSERT2( pJob->mpfnFunc, "no function ptr in job manager" );

		gpJobManager->maDebugData[pJobData->miJobQueueIndex].miNumInQueue = gpJobManager->miNumJobs;
		gpJobManager->maDebugData[pJobData->miJobQueueIndex].miQueueIndex = pJobData->miJobQueueIndex;
		
		pJob->mpfnFunc( pJob->mpData, &gpJobManager->maDebugData[pJobData->miJobQueueIndex] );
		pJob->mpfnFunc = NULL;

		pthread_mutex_unlock( &gpJobManager->mAddJobMutex );
    }

	return NULL;
}

/*
**
*/
void jobManagerInit( tJobManager* pJobManager )
{
    unsigned int iNumCores = 8;
    
#if defined( IOS )
    size_t iLength = sizeof( iNumCores );
    
    // get number of cores
    sysctlbyname ( "hw.ncpu", &iNumCores, &iLength, NULL, 0 );
#elif defined( ANDROID )
    
    iNumCores = sysconf(_SC_NPROCESSORS_CONF);
#elif defined( WIN32 )
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );

	iNumCores = sysinfo.dwNumberOfProcessors;

#endif // WIN32
    
    // number of workers based on cores
	pJobManager->miNumWorkers = iNumCores;
	pJobManager->miStart = 0;
	pJobManager->miEnd = 0;
	
	pJobManager->maThreadID = (pthread_t *)MALLOC( sizeof( pthread_t ) * pJobManager->miNumWorkers );
	pJobManager->maCondition = (pthread_cond_t *)MALLOC( sizeof( pthread_cond_t ) * pJobManager->miNumWorkers );
	pJobManager->maWaitMutex = (pthread_mutex_t *)MALLOC( sizeof( pthread_mutex_t ) * pJobManager->miNumWorkers );
	pJobManager->maiRunning = (int *)MALLOC( sizeof( int ) * pJobManager->miNumWorkers );
	pJobManager->maWorkerData = (tJobData *)MALLOC( sizeof( tJobData ) * pJobManager->miNumWorkers );

    memset( pJobManager->maThreadID, 0, sizeof( pthread_t ) * pJobManager->miNumWorkers );
    memset( pJobManager->maCondition, 0, sizeof( pthread_cond_t ) * pJobManager->miNumWorkers );
    memset( pJobManager->maWaitMutex, 0, sizeof( pthread_mutex_t ) * pJobManager->miNumWorkers );
    memset( pJobManager->maiRunning, 0, sizeof( int ) * pJobManager->miNumWorkers );
    memset( pJobManager->maWorkerData, 0, sizeof( tJobData ) * pJobManager->miNumWorkers );
    
	pthread_mutex_init( &pJobManager->mAddJobMutex, NULL );

    memset( pJobManager->maiRunning, 0, sizeof( int ) * pJobManager->miNumWorkers );
    
	// init
	for( unsigned int i = 0; i < pJobManager->miNumWorkers; i++ )
	{
		pthread_mutex_init( &pJobManager->maWaitMutex[i], NULL );
		pthread_cond_init( &pJobManager->maCondition[i], NULL );
		
		pJobManager->maWorkerData[i].mpCondition = &pJobManager->maCondition[i];
		pJobManager->maWorkerData[i].mpWaitMutex = &pJobManager->maWaitMutex[i];
        pJobManager->maWorkerData[i].miRunning = &pJobManager->maiRunning[i];
		pthread_create( &pJobManager->maThreadID[i], NULL, execWorkFunc, &pJobManager->maWorkerData[i] );
        
        // set to highest priority
        struct sched_param param;
        memset( &param, 0, sizeof( param ) );
        param.sched_priority = sched_get_priority_max( SCHED_FIFO );
        pthread_setschedparam( pJobManager->maThreadID[i], SCHED_FIFO, &param );
	}
    
    pJobManager->miQueueSize = NUM_QUEUE_SLOTS_PER_ALLOC;
    pJobManager->miNumJobs = 0;
    pJobManager->maJobs = (tJob *)MALLOC( sizeof( tJob ) * pJobManager->miQueueSize );
    memset( pJobManager->maJobs, 0, sizeof( tJob ) * pJobManager->miQueueSize );
    pJobManager->miStart = pJobManager->miEnd = 0;
    
	pJobManager->maDebugData = (tJobDebugData *)MALLOC( sizeof( tJobDebugData ) * pJobManager->miQueueSize );
	memset( pJobManager->maDebugData, 0, sizeof( tJobDebugData ) * pJobManager->miQueueSize );

	for( size_t i = 0; i < pJobManager->miQueueSize; i++ )
	{
		tJob* pJob = &pJobManager->maJobs[i];
		pJob->miDataSize = 64;
		pJob->mpData = (void *)MALLOC( pJob->miDataSize );
	}

    pJobManager->mbRunning = true;
    
    pthread_create( &pJobManager->mThreadID, NULL, jobManagerRunInThread, pJobManager );
}


/*
**
*/
static void* jobManagerRunInThread( void* pData )
{
    tJobManager* pJobManager = (tJobManager *)pData;
    while( pJobManager->mbRunning )
    {
        jobManagerUpdate( pJobManager );
    }
    
    return NULL;
}

/*
**
*/
void jobManagerRelease( tJobManager* pJobManager )
{
	for( unsigned int i = 0; i < pJobManager->miNumWorkers; i++ )
	{
		pthread_cond_destroy( &pJobManager->maCondition[i] );
		pthread_mutex_destroy( &pJobManager->maWaitMutex[i] );
	}

	pthread_mutex_destroy( &pJobManager->mRunMutex );
	pthread_mutex_destroy( &pJobManager->mFileMutex );

	FREE( pJobManager->maThreadID );
	FREE( pJobManager->maCondition );
	FREE( pJobManager->maWaitMutex );
	FREE( pJobManager->maiRunning );
	FREE( pJobManager->maWorkerData );

}

/*
**
*/
void jobManagerUpdate( tJobManager* pJobManager )
{
	int iCount = 0;
	for( unsigned int i = 0; i < pJobManager->miQueueSize; i++ )
	{
		if( pJobManager->maJobs[i].mpfnFunc )
		{
			++iCount;
		}
	}

	if( pJobManager->miNumJobs > 0 )
	{
		// find available worker
		unsigned int iFreeWorker = pJobManager->miNumWorkers + 1;
		for( ;; )
		{
			for( unsigned int i = 0; i < pJobManager->miNumWorkers; i++ )
			{
				if( !pJobManager->maiRunning[i] )
				{
					iFreeWorker = i;
					break;
				}

			}	// for i = 0 to num workers
            
			if( iFreeWorker != pJobManager->miNumWorkers + 1 )
			{
				break;
			}

		}	// while waiting for FREE worker

		// still has jobs
		pthread_mutex_lock( &pJobManager->mAddJobMutex );
		WTFASSERT2( iFreeWorker >= 0 && iFreeWorker < pJobManager->miNumWorkers, "invalid free worker %d", iFreeWorker );
		
		// start the job
		pJobManager->maWorkerData[iFreeWorker].miWorkerID = iFreeWorker;
		pJobManager->maWorkerData[iFreeWorker].miJobQueueIndex = pJobManager->miStart;

        pthread_mutex_lock( &pJobManager->maWaitMutex[iFreeWorker] );
        pJobManager->maiRunning[iFreeWorker] = 1;
		
		// signal to start
		pthread_cond_signal( &pJobManager->maCondition[iFreeWorker] );
		
		pthread_mutex_unlock( &pJobManager->maWaitMutex[iFreeWorker] );
        
		// pop off queue
        gpJobManager->miStart = ( gpJobManager->miStart + 1 ) % gpJobManager->miQueueSize;

        if( pJobManager->miNumJobs > 0 )
        {
            --pJobManager->miNumJobs;
        }
        
		pthread_mutex_unlock( &pJobManager->mAddJobMutex );

	}	// if still has jobs
}

/*
**
*/
void jobManagerAddJob( tJobManager* pJobManager, tJob* pJob )
{
	// wait till there is FREE space
	while( pJobManager->miNumJobs + 1 >= pJobManager->miQueueSize )
	{
#if defined( WIN32 )
		Sleep( 100 );
#else
        usleep( 100 );
#endif // #if defined( WIN32 )
	}

	pthread_mutex_lock( &pJobManager->mAddJobMutex );
    tJob* pQueuedJob = &pJobManager->maJobs[pJobManager->miEnd];
    WTFASSERT2( pQueuedJob->mpfnFunc == NULL, "job is still running" );

    // resize job data
    if( pQueuedJob->miDataSize < pJob->miDataSize )
    {
        pQueuedJob->miDataSize = pJob->miDataSize;
        pQueuedJob->mpData = (void *)REALLOC( pQueuedJob->mpData, pJob->miDataSize );
    }

    // copy over data
	WTFASSERT2( pJob->mpfnFunc, "no job function" );
    pQueuedJob->mpfnFunc = pJob->mpfnFunc;
    memcpy( pQueuedJob->mpData, pJob->mpData, pJob->miDataSize );
    
	pJobManager->miEnd = ( pJobManager->miEnd + 1 ) % pJobManager->miQueueSize;
	++pJobManager->miNumJobs;

	pthread_mutex_unlock( &pJobManager->mAddJobMutex );
}

/*
**
*/
bool jobManagerRunning( tJobManager* pJobManager )
{
    bool bThreadRunning = false;
    for( int i = 0; i < (int)pJobManager->miNumWorkers; i++ )
    {
        if( pJobManager->maiRunning[i] )
        {
            bThreadRunning = true;
            break;
        }
    }
    
    return ( bThreadRunning || pJobManager->miNumJobs > 0 );
}

/*
**
*/
void jobManagerWait( tJobManager* pJobManager )
{
    while( jobManagerRunning( pJobManager ) )
    {
#if defined( WIN32 )
		Sleep( 1 );
#else
        usleep( 10 );
#endif // #if 0
        //jobManagerUpdate( pJobManager );
    }
}