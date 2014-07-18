#ifndef __JOB_H__
#define __JOB_H__

struct Job
{
	void*		mpData;
    size_t      miDataSize;
	void		(*mpfnFunc)( void* pData, void* pJobDebugData );
};

typedef struct Job tJob;

struct JobDebugData
{
	int			miQueueIndex;
	int			miNumInQueue;
};

typedef struct JobDebugData tJobDebugData;

inline void jobInit( tJob* pJob ) { pJob->mpData = NULL; pJob->mpfnFunc = NULL; }

#endif // __JOB_H__