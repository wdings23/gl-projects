//
//  hashutil.cpp
//  animtest
//
//  Created by Tony Peng on 7/25/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "hashutil.h"

/*
**
*/
int hash( const char* s )
{
    const char* p;
    unsigned int h,g;
    h=0;
    for(p=s; *p != '\0'; p++)
    {
        h = (h<<4) + *p;
        if((g = h & 0xF0000000))
        {
            h ^= g>>24;
            h ^= g;
        }
    }
    return h%211;
}