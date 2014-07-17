#ifndef __MATRIX_H__
#define __MATRIX_H__

#include "vector.h"

#define M( X, Y ) afEntries[(X<<2)+Y]

typedef struct 
{
	float afEntries[16];
} tMatrix44;

void Matrix44Identity( tMatrix44* pMat );
void Matrix44Scale( tMatrix44* pResult, float fXScale, float fYScale, float fZScale );
void Matrix44Transpose( tMatrix44* pResult, tMatrix44 const* pMat );
void Matrix44Inverse( tMatrix44* pResult, tMatrix44 const* pMat );
void Matrix44Transform( tVector4* pResultV, tVector4 const* pV, tMatrix44 const* pMat );
void Matrix44Multiply( tMatrix44* pResult, tMatrix44 const* pMat1, tMatrix44 const* pMat2 );
void Matrix44RotateX( tMatrix44* pResult, float fXAngle ); 
void Matrix44RotateY( tMatrix44* pResult, float fYAngle ); 
void Matrix44RotateZ( tMatrix44* pResult, float fZAngle ); 
void Matrix44Translate( tMatrix44* pResult, tVector4 const* pV );

void Matrix44LookAt( tMatrix44* pResult, 
					tVector4* pEye, 
					tVector4* pLookAtPt, 
					tVector4* pUp, 
					tMatrix44* pInvRotMatrix,
					int iWideScreen );
void Matrix44Perspective( tMatrix44* pResult,
                          float fFOV,
                          unsigned int iWidth,
                          unsigned int iHeight,
                          float fFar,
                          float fNear );

void Matrix44Print( tMatrix44* pMat );

void Matrix44MultiplyNEON( tMatrix44* pResult, tMatrix44 const* pMat1, tMatrix44 const* pMat2 );
void Matrix44TransposeNEON( tMatrix44* pResult, tMatrix44 const* pMat );
void Matrix44TransformNEON( tVector4* pResultV, tVector4 const* pV, tMatrix44 const* pMat );

#endif // __MATRIX_H__