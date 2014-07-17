//
//  matrixpool.cpp
//  Game6
//
//  Created by Dingwings on 1/5/13.
//  Copyright (c) 2013 Dingwings. All rights reserved.
//

#include "matrixpool.h"



#define NUM_MATRIX_PER_ALLOC 5000

/*
**
*/
CMatrixPool* CMatrixPool::mpInstance = NULL;
CMatrixPool* CMatrixPool::instance( void )
{
    if( mpInstance == NULL )
    {
        mpInstance = new CMatrixPool();
    }
    
    return mpInstance;
}

/*
**
*/
CMatrixPool::CMatrixPool( void )
{
    maMatrices = (tMatrix44 *)MALLOC( sizeof( tMatrix44 ) * NUM_MATRIX_PER_ALLOC );
    miNumMatrices = 0;
    miNumMatrixAlloc = NUM_MATRIX_PER_ALLOC;
}

/*
**
*/
CMatrixPool::~CMatrixPool( void )
{
    FREE( maMatrices );
}

/*
**
*/
tMatrix44* CMatrixPool::allocMatrix( int iOffsetFromCurrent )
{
    WTFASSERT2( miNumMatrices + 1 < miNumMatrixAlloc, "need more space for matrix" );
    tMatrix44* pRet = &maMatrices[miNumMatrices+iOffsetFromCurrent];
    ++miNumMatrices;
    
    return pRet;
}