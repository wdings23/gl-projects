//
//  timeutil.cpp
//  animtest
//
//  Created by Tony Peng on 7/30/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "timeutil.h"

#ifdef ANDROID
#include <time.h>
#else
#include <sys/time.h>
#include <mach/clock.h>
#include <mach/mach.h>
#endif // ANDROID

/*
**
*/
double getTime( void )
{
    struct timespec tspec;
    
#ifdef ANDROID 
    int time = clock_gettime( CLOCK_REALTIME, &tspec );
#else
    //struct timeb currTime;
    //ftime( &currTime );
    
    //double fMilliSec = (double)( currTime.time * 1000 + currTime.millitm );

    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service( mach_host_self(), CALENDAR_CLOCK, &cclock );
    clock_get_time( cclock, &mts );
    mach_port_deallocate( mach_task_self(), cclock );
    tspec.tv_sec = mts.tv_sec;
    tspec.tv_nsec = mts.tv_nsec;
    
#endif // ANDROID
    
    unsigned long long nanoSec = (unsigned long long)tspec.tv_sec * 1000000000 + (unsigned long long)tspec.tv_nsec;
    double fMilliSec = (double)( nanoSec / 1000000 );
    
    return fMilliSec;
}