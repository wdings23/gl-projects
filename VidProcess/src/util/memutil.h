//
//  memutil.h
//  animtest
//
//  Created by Tony Peng on 7/26/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#ifndef __MEMUTIL_H__
#define __MEMUTIL_H__

void neonmemcpy( void* pDest, void* pSrc, size_t iSize );

void debugFree( const char* szName, void* pData );
void* debugRealloc( const char* szName, void* pData, size_t iSize );
void* debugMalloc( const char* szName, size_t iSize );

#endif // __MEMUTIL_H__