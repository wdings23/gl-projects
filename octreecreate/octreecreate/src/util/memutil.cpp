//
//  memutil.cpp
//  animtest
//
//  Created by Tony Peng on 7/26/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "memutil.h"

/*
**
*/
void neonmemcpy( void* pDest, void* pSrc, size_t iSize )
{
#if defined( __ARM_NEON__ )
    
    // copies 16 32-bit values at once (16x4 bytes) = 64 bytes
    size_t iRemaining = iSize & 0x3F;
    
    asm volatile(
                 "mov        r0, %2                              \n\t"
                 "startcopy:                                     \n\t"
                 "pld        [%1, #0xC0]                         \n\t"      // preload memory to cache
                 "subs       r0, r0, #0x40                       \n\t"
                 "vldm       %1!, {q0-q3}                        \n\t"      // load src
                 "vstm       %0!, {q0-q3}                        \n\t"      // store dest
                 "bgt        startcopy                           \n\t"
                 
                 :
                 : "r"(pDest), "r"(pSrc), "r"(iSize)
                 : "q0", "q1", "q2", "q3", "cc", "memory"
                 );
    
    size_t iCopiedSize = iSize - iRemaining;
    void* pRemainSrc = (void *)( (unsigned int)pSrc + iCopiedSize );
    void* pRemainDest = (void *)( (unsigned int)pDest + iCopiedSize );
    memcpy(pRemainDest, pRemainSrc, iRemaining);
#else
    
    memcpy( pDest, pSrc, iSize );
#endif // __ARM_NEON__
}