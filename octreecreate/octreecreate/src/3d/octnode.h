//
//  Header.h
//  Game4
//
//  Created by Dingwings on 7/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef __OCTNODE_H__
#define __OCTNODE_H__

#include "vector.h"
#include "model.h"
#include "camera.h"

#include <stdio.h>
#include <string.h>

enum
{
	OCT_TOP_LEFT_FRONT = 0,
	OCT_TOP_RIGHT_FRONT,
	OCT_TOP_LEFT_BACK,
	OCT_TOP_RIGHT_BACK,
	
	OCT_BOTTOM_LEFT_FRONT,
	OCT_BOTTOM_RIGHT_FRONT,
	OCT_BOTTOM_LEFT_BACK,
	OCT_BOTTOM_RIGHT_BACK,
	
	NUM_OCTS,
};

class COctNode
{
public:
    COctNode( void );
    COctNode( tVector4 const* pPosition, float fSize, COctNode* pParent );
    
    ~COctNode( void );
    
    void addObject( void* pObject );
    
    inline COctNode* getChild( int iIndex ) { return mapChildren[iIndex]; }
    inline void setChild( COctNode* pNode, int iIndex ) { mapChildren[iIndex] = pNode; }
    
    inline COctNode* getSibling( int iIndex ) { return mapSiblings[iIndex]; }
    inline void setSibling( COctNode* pNode, int iIndex ) { mapSiblings[iIndex] = pNode; }
    
    inline COctNode* getParent( void ) { return mpParent; }
    inline void setParent( COctNode* pParent ) { mpParent = pParent; }
    
    inline tVector4 const* getPosition( void ) const { return &mCenter; }
    inline float getSize( void ) const { return mfSize; }
    
    inline int getNumObjects( void ) { return miNumObjects; }
    inline void** getObjects( void ) { return mapObjects; }
    
    inline void setExtent( tVector4 const* pExtent ) { memcpy( &mExtent, pExtent, sizeof( tVector4 ) ); }
    inline void setPosition( tVector4 const* pCenter ) { memcpy( &mCenter, pCenter, sizeof( tVector4 ) ); }
    inline void setSize( float fSize ) { mfSize = fSize; }
    
    void draw( void );
    
    void computeCameraDistance( CCamera const* pCamera );
    inline float getCameraDistance( void ) { return mfCameraDistance; }
    
protected:
    tVector4    mCenter;
    tVector4    mExtent;
    float       mfSize;
    
    int         miNumObjectsAlloc;
    int         miNumObjects;
    void**       mapObjects;
    
    COctNode*       mapChildren[NUM_OCTS];
    COctNode*       mpParent;
    
    COctNode*       mapSiblings[NUM_OCTS];
    
    float           mfCameraDistance;
    
public:
    static bool intersect( tVector4 const* pPos, float fSize, tVector4 const* pPt0, tVector4 const* pPt1 );
};



#endif // __OCTNODE_H__

