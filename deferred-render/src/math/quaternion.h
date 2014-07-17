#ifndef __QUATERNION_H__
#define __QUATERNION_H__

#include "vector.h"
#include "matrix.h"

struct Quaternion
{
	float mfX;
	float mfY;
	float mfZ;
	float mfW;
};

typedef struct Quaternion tQuaternion;

void quaternionInit( tQuaternion* pQ );
void quaternionFromAxisAngle( tQuaternion* pResult, tVector4 const* pAxis, float fAngle );
void quaternionMultiply( tQuaternion* pResult, tQuaternion const* pQ0, tQuaternion const* pQ1 );
void quaternionToMatrix( tMatrix44* pResult, tQuaternion const* pQ );
void quaternionSlerp( tQuaternion* pResult,
                      tQuaternion const* pQ0,
                      tQuaternion const* pQ1,
                      float fPct );

#endif // __QUATERNION_H__