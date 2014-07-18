#include "quaternion.h"
#include "matrix.h"

#include <string.h>

/*
**
*/
void quaternionInit( tQuaternion* pQ )
{
	pQ->mfX = 0.0f;
	pQ->mfY = 0.0f;
	pQ->mfZ = 0.0f;
	pQ->mfW = 1.0f;
}

/*
**
*/
void quaternionFromAxisAngle( tQuaternion* pResult, tVector4 const* pAxis, float fAngle )
{
	float fS = sinf( fAngle * 0.5f );
    
	pResult->mfX = pAxis->fX * fS;
	pResult->mfY = pAxis->fY * fS;
	pResult->mfZ = pAxis->fZ * fS;
	pResult->mfW = cosf( fAngle * 0.5f );
}

/*
**
*/
void quaternionMultiply( tQuaternion* pResult, tQuaternion const* pQ0, tQuaternion const* pQ1 )
{
    float fQ0X = pQ0->mfX, fQ1X = pQ1->mfX;
    float fQ0Y = pQ0->mfY, fQ1Y = pQ1->mfY;
    float fQ0Z = pQ0->mfZ, fQ1Z = pQ1->mfZ;
    float fQ0W = pQ0->mfW, fQ1W = pQ1->mfW;
    
    pResult->mfW = fQ0W * fQ1W - fQ0X * fQ1X - fQ0Y * fQ1Y - fQ0Z * fQ1Z;
    pResult->mfX = fQ0W * fQ1X + fQ0X * fQ1W + fQ0Y * fQ1Z - fQ0Z * fQ1Y;
    pResult->mfY = fQ0W * fQ1Y - fQ0X * fQ1Z + fQ0Y * fQ1W + fQ0Z * fQ1X;
    pResult->mfZ = fQ0W * fQ1Z + fQ0X * fQ1Y - fQ0Y * fQ1X + fQ0Z * fQ1W;
}

/*
**
*/
void quaternionToMatrix( tMatrix44* pResult, tQuaternion const* pQ )
{
	float fX2 = pQ->mfX * pQ->mfX;
	float fY2 = pQ->mfY * pQ->mfY;
	float fZ2 = pQ->mfZ * pQ->mfZ;
	float fXY = pQ->mfX * pQ->mfY;
	float fXZ = pQ->mfX * pQ->mfZ;
	float fYZ = pQ->mfY * pQ->mfZ;
	float fWX = pQ->mfW * pQ->mfX;
	float fWY = pQ->mfW * pQ->mfY;
	float fWZ = pQ->mfW * pQ->mfZ;
    
	pResult->M( 0, 0 ) = 1.0f - 2.0f * ( fY2 + fZ2 );
	pResult->M( 1, 0 ) = 2.0f * ( fXY - fWZ );
	pResult->M( 2, 0 ) = 2.0f * ( fXZ + fWY );
	pResult->M( 3, 0 ) = 0.0f;
	
	pResult->M( 0, 1 ) = 2.0f * ( fXY + fWZ );
	pResult->M( 1, 1 ) = 1.0f - 2.0f * ( fX2 + fZ2 );
	pResult->M( 2, 1 ) = 2.0f * ( fYZ - fWX );
	pResult->M( 3, 1 ) = 0.0f;
	
	pResult->M( 0, 2 ) = 2.0f * ( fXZ - fWY );
	pResult->M( 1, 2 ) = 2.0f * ( fYZ + fWX );
	pResult->M( 2, 2 ) = 1.0f - 2.0f * ( fX2 + fY2 );
	pResult->M( 3, 2 ) = 0.0f;
    
	pResult->M( 0, 3 ) = 0.0f;
	pResult->M( 1, 3 ) = 0.0f;
	pResult->M( 2, 3 ) = 0.0f;
	pResult->M( 3, 3 ) = 1.0f;
}

/*
**
*/
void quaternionSlerp( tQuaternion* pResult,
                      tQuaternion const* pQ0,
                      tQuaternion const* pQ1,
                      float fPct )
{
    float fCosHalfTheta = pQ0->mfX * pQ1->mfX +
                          pQ0->mfY * pQ1->mfY +
                          pQ0->mfZ * pQ1->mfZ +
                          pQ0->mfW * pQ1->mfW;
    
    if( fabs( fCosHalfTheta ) >= 1.0f )
    {
        memcpy( pResult, pQ0, sizeof( tQuaternion ) );
        return;
    }
    
    float fHalfTheta = acosf( fCosHalfTheta );
    float fSinHalfTheta = sinf( fHalfTheta );
    
    if( fabs( fSinHalfTheta ) < 0.001f )
    {
        pResult->mfX = pQ1->mfX + ( pQ1->mfX - pQ0->mfX ) * fPct;
        pResult->mfY = pQ1->mfY + ( pQ1->mfY - pQ0->mfY ) * fPct;
        pResult->mfZ = pQ1->mfZ + ( pQ1->mfZ - pQ0->mfZ ) * fPct;
        pResult->mfW = pQ1->mfW + ( pQ1->mfW - pQ0->mfW ) * fPct;
    }
    
    float fOneOverSinHalfTheta = 1.0f / fSinHalfTheta;
    float fRatio0 = sinf( 1.0f - fPct ) * fHalfTheta * fOneOverSinHalfTheta;
    float fRatio1 = sinf( fPct * fHalfTheta ) * fOneOverSinHalfTheta;
    
    pResult->mfW = pQ1->mfW * fRatio0 + pQ0->mfW * fRatio1;
    pResult->mfX = pQ1->mfX * fRatio0 + pQ0->mfX * fRatio1;
    pResult->mfY = pQ1->mfY * fRatio0 + pQ0->mfY * fRatio1;
    pResult->mfZ = pQ1->mfZ * fRatio0 + pQ0->mfZ * fRatio1;
}