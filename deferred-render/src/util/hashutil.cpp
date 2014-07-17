//
//  hashutil.cpp
//  animtest
//
//  Created by Tony Peng on 7/25/13.
//  Copyright (c) 2013 Tony Peng. All rights reserved.
//

#include "hashutil.h"

#define A 54059 /* a prime */
#define B 76963 /* another prime */
#define C 86969 /* yet another prime */

/*
**
*/
int hash( const char* s )
{
   unsigned h = 31 /* also prime */;
   while (*s) 
   {
     h = (h * A) ^ (s[0] * B);
     s++;
   }
   return h; // or return h % C;
}