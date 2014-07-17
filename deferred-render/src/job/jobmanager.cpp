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

#define NUM_QUEUE_SLOTS_PER_ALLOC 1000

tJobManager gJobManager;
tJobManager* gpJobManager = &gJobManager;

#if 0
static void* jobManagerRunInThread( void* pData );
#endif // #if 0

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
    
/*OUTPUT( "0 worker %d job manager running ( %d, %d, %d, %d )\n",
        pJobData->miWorkerID,
        gpJobManager->maiRunning[0],
        gpJobManager->maiRunning[1],
        gpJobManager->maiRunning[2],
        gpJobManager->maiRunning[3] );
*/
       // wait for activation
       pthread_mutex_lock( pWaitMutex );
        
        // worker is FREEd
        *piRunning = 0;
        pthread_cond_wait( pCondition, pWaitMutex );
		
/*OUTPUT( "1 worker %d job manager running ( %d, %d, %d, %d )\n",
       pJobData->miWorkerID,
       gpJobManager->maiRunning[0],
       gpJobManager->maiRunning[1],
       gpJobManager->maiRunning[2],
       gpJobManager->maiRunning[3] );
*/
        
       // worker is busy
       WTFASSERT2( pJobData->miWorkerID >= 0 && pJobData->miWorkerID < (int)gpJobManager->miNumWorkers, "invalid worker id" );
       *piRunning = 1;
        
       pthread_mutex_unlock( pWaitMutex );
    
//OUTPUT( "START execute worker %d\n", pJobData->miWorkerID );
        
/*OUTPUT( "2 worker %d job manager running ( %d, %d, %d, %d )\n",
       pJobData->miWorkerID,
       gpJobManager->maiRunning[0],
       gpJobManager->maiRunning[1],
       gpJobManager->maiRunning[2],
       gpJobManager->maiRunning[3] );
*/
        // execute job
        tJob const* pJob = pJobData->mpJob;
		WTFASSERT2( pJob->mpfnFunc, "no function ptr in job manager" );
        pJob->mpfnFunc( pJob->mpData );
        

//OUTPUT( "END execute worker %d running = %d\n", pJobData->miWorkerID, gpJobManager->maiRunning[pJobData->miWorkerID] );
    
/*OUTPUT( "3 worker %d job manager running ( %d, %d, %d, %d )\n",
       pJobData->miWorkerID,
       gpJobManager->maiRunning[0],
       gpJobManager->maiRunning[1],
       gpJobManager->maiRunning[2],
       gpJobManager->maiRunning[3] );
*/
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

	pthread_mutex_init( &pJobManager->mAddJobMutex, NULL );
	
	WTFASSERT2( pJobManager->maiRunning, "can't allocate running flags" );
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
        
/*OUTPUT( "worker %d condition = 0x%8X mutex = 0x%8X data = 0x%8X\n",
        i,
        (unsigned int)pJobManager->maWorkerData[i].mpCondition,
        (unsigned int)pJobManager->maWorkerData[i].mpWaitMutex,
        (unsigned int)&pJobManager->maWorkerData[i] );
*/
	}
    
    pJobManager->miQueueSize = NUM_QUEUE_SLOTS_PER_ALLOC;
    pJobManager->miNumJobs = 0;
    pJobManager->maJobs = (tJob *)MALLOC( sizeof( tJob ) * pJobManager->miQueueSize );
    memset( pJobManager->maJobs, 0, sizeof( tJob ) * pJobManager->miQueueSize );
    pJobManager->miStart = pJobManager->miEnd = 0;
    
	for( size_t i = 0; i < pJobManager->miQueueSize; i++ )
	{
		tJob* pJob = &pJobManager->maJobs[i];
		pJob->miDataSize = 64;
		pJob->mpData = (void *)MALLOC( pJob->miDataSize );
	}

    pJobManager->mbRunning = true;
}

#if 0
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
#endif // #if 0

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
	if( pJobManager->miNumJobs > 0 )
	{
		// still has jobs
		tJob* pJob = &pJobManager->maJobs[pJobManager->miStart];
     
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
            
            /*OUTPUT( "running ( %d, %d, %d, %d )\n",
                    pJobManager->maiRunning[0],
                    pJobManager->maiRunning[1],
                    pJobManager->maiRunning[2],
                    pJobManager->maiRunning[3] );*/
            
			if( iFreeWorker != pJobManager->miNumWorkers + 1 )
			{
				break;
			}
            
//OUTPUT( "WAITING FOR FREE SLOT...\n" );

		}	// while waiting for FREE worker

		// start the job
		assert( iFreeWorker >= 0 && iFreeWorker < pJobManager->miNumWorkers );
		pJobManager->maWorkerData[iFreeWorker].mpJob = pJob;
        pJobManager->maWorkerData[iFreeWorker].miWorkerID = iFreeWorker;
        
        pthread_mutex_lock( &pJobManager->maWaitMutex[iFreeWorker] );
        pJobManager->maiRunning[iFreeWorker] = 1;
/*OUTPUT( "MANAGER PICKED worker %d running ( %d, %d, %d, %d )\n",
        iFreeWorker,
        pJobManager->maiRunning[0],
        pJobManager->maiRunning[1],
        pJobManager->maiRunning[2],
        pJobManager->maiRunning[3] );*/

		pthread_mutex_lock( &pJobManager->mAddJobMutex );
		pthread_cond_signal( &pJobManager->maCondition[iFreeWorker] );
		pthread_mutex_unlock( &pJobManager->mAddJobMutex );
		
		pthread_mutex_unlock( &pJobManager->maWaitMutex[iFreeWorker] );
        
        
/*OUTPUT( "FREE = %d worker data = 0x%8X job = 0x%8X worker job = 0x%8X num jobs = %d\n",
        iFreeWorker,
        (unsigned int)&pJobManager->maWorkerData[iFreeWorker],
        (unsigned int)pJob,
        (unsigned int)&(pJobManager->maWorkerData[iFreeWorker].mpJob),
        pJobManager->miNumJobs );
*/
        // pop off queue
        pJobManager->miStart = ( pJobManager->miStart + 1 ) % pJobManager->miQueueSize;
        
        if( pJobManager->miNumJobs > 0 )
        {
            --pJobManager->miNumJobs;
        }
        
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

#if 0
    // resize queue
    if( pJobManager->miNumJobs + 1 >= pJobManager->miQueueSize )
    {
		// CAN'T LAUNCH JOB INSIDE JOB

        int iPrevQueueSize = pJobManager->miQueueSize;
        pJobManager->miQueueSize += NUM_QUEUE_SLOTS_PER_ALLOC;
        pJobManager->maJobs = (tJob *)realloc( pJobManager->maJobs, pJobManager->miQueueSize * sizeof( tJob ) );
        
        memset( &pJobManager->maJobs[iPrevQueueSize], 0, sizeof( tJob ) * NUM_QUEUE_SLOTS_PER_ALLOC );
        
        // move from 0 to the newly allocated slots
        int iEnd = pJobManager->miEnd;
        tJob* pMoveFrom = &pJobManager->maJobs[0];
        int iToIndex = iPrevQueueSize % pJobManager->miQueueSize;
        for( int i = 0; i <= iEnd; i++ )
        {
            iToIndex = ( iPrevQueueSize + i ) % pJobManager->miQueueSize;
            tJob* pMoveTo = &pJobManager->maJobs[iToIndex];
            
            // set data and function
            pMoveTo->miDataSize = pMoveFrom->miDataSize;
            pMoveTo->mpData = (void *)malloc( pMoveTo->miDataSize );
            
            memcpy( pMoveTo->mpData, pMoveFrom->mpData, pMoveTo->miDataSize );
            
            pMoveTo->mpfnFunc = pMoveFrom->mpfnFunc;
            ++pMoveFrom;
            
            WTFASSERT2( iPrevQueueSize + i < (int)pJobManager->miQueueSize, "array out of bounds" );
            
        }   // for i = 0 to end of queue
        
        // set the new end
        pJobManager->miEnd = iToIndex;
    }
#endif // #if 0

	pthread_mutex_lock( &pJobManager->mAddJobMutex );

	//OUTPUT( "START ADD JOB\n" );

    tJob* pQueuedJob = &pJobManager->maJobs[pJobManager->miEnd];
    
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
    
/*OUTPUT( "job jobData = 0x%8X fn = 0x%8X data = 0x%8X\n",
        (unsigned int)&pJobManager->maJobs[pJobManager->miEnd],
        (unsigned int)pJobManager->maJobs[pJobManager->miEnd].mpfnFunc,
        (unsigned int)pJobManager->maJobs[pJobManager->miEnd].mpData );
*/
    
    pJobManager->miEnd = ( pJobManager->miEnd + 1 ) % pJobManager->miQueueSize;
    //WTFASSERT2( pJobManager->miEnd != pJobManager->miStart, "invalid end index for job" );
    ++pJobManager->miNumJobs;

	//OUTPUT( "END ADD JOB\n\n" );

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