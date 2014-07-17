#ifndef __GAMECAMERA_H__
#define __GAMECAMERA_H__

#include "vector.h"

void gameCameraInit( tVector4 const* pPos, tVector4 const* pLookAt );
void gameCameraUpdate( float fDLookX, float fDLookY, float fDPosX, float fDPosY, float fDPosZ );


#endif // __GAMECAMERA_H__