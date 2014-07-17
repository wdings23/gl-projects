//
//  matrixpool.h
//  Game6
//
//  Created by Dingwings on 1/5/13.
//  Copyright (c) 2013 Dingwings. All rights reserved.
//

#ifndef __MATRIXPOOL_H__
#define __MATRIXPOOL_H__

#include "matrix.h"

class CMatrixPool
{
public:
    CMatrixPool( void );
    virtual ~CMatrixPool( void );
    
    tMatrix44* allocMatrix( int iOffsetFromCurrent );
    
protected:
    tMatrix44*          maMatrices;
    int                 miNumMatrices;
    int                 miNumMatrixAlloc;
  
public:
    static CMatrixPool* instance( void );
    
protected:
    static CMatrixPool* mpInstance;
};

#endif // __MATRIXPOOL_H__
