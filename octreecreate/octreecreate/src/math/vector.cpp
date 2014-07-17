#include <math.h>
#include <assert.h>

#include "vector.h"

tVector4 gXAxis = { 1.0f, 0.0f, 0.0f, 1.0f };
tVector4 gYAxis = { 0.0f, 1.0f, 0.0f, 1.0f };
tVector4 gZAxis = { 0.0f, 0.0f, 1.0f, 1.0f };

/*
**
*/
void Vector4Normalize( tVector4* pResult, tVector4 const* pV )
{
    float fLength = Vector4Magnitude( pV );
    
    if( fabs( fLength ) > 0.0f )
    {
        float fOneOverLength = 1.0f / Vector4Magnitude( pV );
        Vector4MultScalar( pResult, pV, fOneOverLength );
    }
}

/*
**
*/
float Vector4Magnitude( tVector4 const* pV )
{
	return sqrtf( pV->fX * pV->fX + pV->fY * pV->fY + pV->fZ * pV->fZ );
}

/*
**
*/
void Vector4MultScalar( tVector4* pResult, tVector4 const* pV, float fScalar )
{
#if defined( __ARM_NEON__ )
	Vector4MultScalarNEON( pResult, pV, fScalar );
#else
    pResult->fX = pV->fX * fScalar;
	pResult->fY = pV->fY * fScalar;
	pResult->fZ = pV->fZ * fScalar;
#endif // _SIMULATOR
}

/*
**
*/
void Vector4Subtract( tVector4* pResult, tVector4 const* pV1, tVector4 const* pV2 )
{
#if defined( __ARM_NEON__ )
    Vector4SubtractNEON( pResult, pV1, pV2 );
#else
	pResult->fX = pV1->fX - pV2->fX;
	pResult->fY = pV1->fY - pV2->fY;
	pResult->fZ = pV1->fZ - pV2->fZ;
#endif // __ARM_NEON__
}

/*
**
*/
void Vector4Cross( tVector4* pResult, tVector4 const* pV1, tVector4 const* pV2 )
{
	pResult->fX = pV1->fY * pV2->fZ - pV1->fZ * pV2->fY;
	pResult->fY = pV1->fZ * pV2->fX - pV1->fX * pV2->fZ;
	pResult->fZ = pV1->fX * pV2->fY - pV1->fY * pV2->fX;
}

/*
**
*/
void Vector4Lerp( tVector4* pResult, tVector4 const* pV1, tVector4 const* pV2, float fPct )
{
	assert( pResult );
	assert( pV1 );
	assert( pV2 );
	
	pResult->fX = pV1->fX + fPct * ( pV2->fX - pV1->fX );
	pResult->fY = pV1->fY + fPct * ( pV2->fY - pV1->fY );
	pResult->fZ = pV1->fZ + fPct * ( pV2->fZ - pV1->fZ );
    pResult->fW = 1.0f;
}

/*
**
*/
float Vector4Angle( tVector4 const* pV1, tVector4 const* pV2 )
{
	tVector4 v;
	
	v.fX = pV1->fX - pV2->fX;
	v.fY = pV1->fY - pV2->fY;
	v.fZ = pV1->fZ - pV2->fZ;
	
	if( v.fX == 0.0f && 
	    v.fY == 0.0f && 
	    v.fZ == 0.0f )
	{
		return 0.0f;
	}
	
	Vector4Normalize( &v, &v );
	if( v.fX != 0.0f )
	{
		return atan2( v.fZ, v.fX );
	}
	
	return 0.0f;
}

/*
**
*/
float Vector4Dot( tVector4 const* pV1, tVector4 const* pV2 )
{
#if defined( __ARM_NEON__ )
    return Vector4DotNEON( pV1, pV2 );
#else
	return ( pV1->fX * pV2->fX + pV1->fY * pV2->fY + pV1->fZ * pV2->fZ );
#endif // __ARM_NEON__
}

void Vector4ConvToShort( tVector3i* pResult, tVector4 const* pV, float fVertexScale )
{
	pResult->iX = (short)( pV->fX * fVertexScale );
	pResult->iY = (short)( pV->fY * fVertexScale );
	pResult->iZ = (short)( pV->fZ * fVertexScale );	
}

void Vector4ConvToFloat( tVector4* pResult, tVector3i* pV, float fOneOverVertexScale )
{
	pResult->fX = (float)pV->iX * fOneOverVertexScale;
	pResult->fY = (float)pV->iY * fOneOverVertexScale;
	pResult->fZ = (float)pV->iZ * fOneOverVertexScale;
	pResult->fW = 1.0f;
}

/*
**
*/
void Vector2Normalize( tVector2* pResult, tVector2 const* pV )
{
	float fLength = sqrtf( pV->fX * pV->fX + pV->fY * pV->fY );
	pResult->fX = pV->fX / fLength;
	pResult->fY = pV->fY / fLength;
}

/*
**
*/
void Vector4Add( tVector4* pResult, tVector4 const* pV0, tVector4 const* pV1 )
{
#if defined( __ARM_NEON__ )
    Vector4AddNEON( pResult, pV0, pV1 );
#else
    pResult->fX = pV0->fX + pV1->fX;
    pResult->fY = pV0->fY + pV1->fY;
    pResult->fZ = pV0->fZ + pV1->fZ;
    pResult->fW = 1.0f;
#endif // __ARM_NEON__
}

#if defined( __ARM_NEON__ )

/*
**
*/
void Vector4MultScalarNEON( tVector4* pResult, tVector4 const* pV, float fScalar )
{
    float afScalar[] = { fScalar, fScalar, fScalar, 1.0f };
    asm volatile (
      "vldmia     %1, {q0}                     \n\t"        // load vector
      "vldmia     %2, {q1}                              \n\t"        // load scalar
      
      "vmul.F32   q2, q0, q1                            \n\t"
      
      "vstmia     %0, {q2}                              \n\t"        // store into result
      
      :
      : "r" (pResult), "r" (pV), "r" (afScalar)
      : "q0","q1","q2","memory"
    );
}

/*
**
*/
float Vector4DotNEON( tVector4 const* pV1, tVector4 const* pV2 )
{
    float afResult[4];
    
    asm volatile (
      "vldmia     %1, {q0}                     \n\t"        // load vector 1
      "vldmia     %2, {q1}                              \n\t"        // load vector 2
      
      "vmul.F32   q2, q0, q1                            \n\t"   // v0 * v1
      "vadd.F32   s13, s8, s9                           \n\t"
      "vadd.F32   s12, s13, s10                         \n\t"
        
      "vstmia     %0, {q3}                              \n\t"        // store into result
                  
      :
      : "r" (afResult), "r" (pV1), "r" (pV2)
      : "q0","q1","q2","memory"
    );
    
    return afResult[0];
}

/*
**
*/
void Vector4AddNEON( tVector4* pResult, tVector4 const* pV0, tVector4 const* pV1 )
{
    asm volatile (
        "vldmia             %1, {q0}        \n\t"
        "vldmia             %2, {q1}        \n\t"
                  
        "vadd.F32           q2, q0, q1      \n\t"
        "vstmia             %0, {q2}        \n\t"
                  
        :
        :   "r" (pResult), "r" (pV0), "r" (pV1)
        : "q0", "q1", "q2", "memory"
    );
}

/*
**
*/
void Vector4SubtractNEON( tVector4* pResult, tVector4 const* pV1, tVector4 const* pV2 )
{
    asm volatile (
      "vldmia     %1, {q0}                     \n\t"        // load vector 1
      "vldmia     %2, {q1}                              \n\t"        // load vector 2
      
      "vsub.F32   q2, q0, q1                            \n\t"   // v0 * v1
      "vstmia     %0, {q2}                              \n\t"        // store into result
      
      :
      : "r" (pResult), "r" (pV1), "r" (pV2)
      : "q0","q1","q2","memory"
    );
}

/*
**
*/
float Vector4MagnitudeNEON( tVector4 const* pV )
{
    float afResult[4];
    asm volatile (
        "vldm       %1, {q0}                \n\t"
        
        "vmul.F32   q1, q0, q0              \n\t"   // v * v
        "vadd.F32   s0, s4, s5              \n\t"   // x + y
        "vadd.F32   s0, s0, s6              \n\t"   // x + y + z
        "vsqrt.F32  s4, s0                  \n\t"   // sqrt
                  
        "vstm       %0, {q1}                \n\t"
    :
    : "r" (afResult), "r" (pV)
    : "q0", "q1", "memory"
    );
    
    return afResult[0];
}

#endif // __ARM_NEON__
