#ifndef __JOBMANAGER_H__
#define __JOBMANAGER_H__

#include "job.h"
#include <pthread.h>

struct JobData
{
	tJob*					mpJob;
	pthread_mutex_t*		mpWaitMutex;
	pthread_cond_t*			mpCondition;
    int*                    miRunning;
    int                     miWorkerID;
};

typedef struct JobData tJobData;

struct JobManager
{
    // job queue
	tJob*					maJobs;
	unsigned int			miNumJobs;
	unsigned int			miStart;
	unsigned int			miEnd;
	unsigned int            miQueueSize;
    
	// workers
	pthread_t*				maThreadID;
	pthread_cond_t*			maCondition;
	pthread_mutex_t*		maWaitMutex;
	unsigned int			miNumWorkers;
	int*					maiRunning;
	tJobData*				maWorkerData;
    
    // mutices
	pthread_mutex_t			mRunMutex;
	pthread_mutex_t			mFileMutex;
    
    // job manager thread 
    pthread_t               mThreadID;
    bool                    mbRunning;
};

typedef struct JobManager tJobManager;



void jobManagerInit( tJobManager* pJobManager );
void jobManagerRelease( tJobManager* pJobManager );
void jobManagerUpdate( tJobManager* pJobManager );
void jobManagerAddJob( tJobManager* pJobManager, tJob* pJob );
bool jobManagerRunning( tJobManager* pJobManager );
void jobManagerWait( tJobManager* pJobManager );

extern tJobManager* gpJobManager;

#endif // __JOBMANAGER_H__