#ifndef __VECTOR_H__
#define __VECTOR_H__

#include <math.h>

#define USE_SHORT	1

typedef struct 
{
	float fX;
	float fY;
	float fZ;
	float fW;
} tVector4;

typedef struct
{
    float fX;
    float fY;
    float fZ;
} tVector3;

struct Vector3i
{
	short iX;
	short iY;
	short iZ;
	//short iW;
};

typedef struct Vector3i tVector3i;

struct Vector2
{
	float fX;
	float fY;
};

struct Vector2i
{
	short iX;
	short iY;
};

typedef struct Vector2 tVector2;
typedef struct Vector2i tVector2i;

void Vector4ConvToShort( tVector3i* pResult, tVector4* pV, float fVertexScale );
void Vector4ConvToFloat( tVector4* pResult, tVector3i* pV, float fOneOverVertexScale );

void Vector4Add( tVector4* pResult, tVector4 const* pV0, tVector4 const* pV1 );
void Vector4Normalize( tVector4* pResult, tVector4 const* pV );
void Vector4Subtract( tVector4* pResult, tVector4 const* pV1, tVector4 const* pV2 );
void Vector4MultScalar( tVector4* pResult, tVector4 const* pV, float fScalar );
float Vector4Magnitude( tVector4 const* pV );
void Vector4Cross( tVector4* pResult, tVector4 const* pV1, tVector4 const* pV2 );
void Vector4Lerp( tVector4* pResult, tVector4 const* pV1, tVector4 const* pV2, float fPct );
float Vector4Angle( tVector4 const* pV1, tVector4 const* pV2 );
float Vector4Dot( const tVector4* pV1, const tVector4* pV2 );

void Vector2Normalize( tVector2* pResult, tVector2 const* pV );
inline float Vector2Length( tVector2 const* pV ) { return sqrtf( pV->fX * pV->fX + pV->fY * pV->fY ); }
inline float Vector2Dot( tVector2 const* pV0, tVector2 const* pV1 ) { return ( pV0->fX * pV1->fX + pV0->fY * pV1->fY ); }
inline void Vector2Add( tVector2* pResult, tVector2 const* pV0, tVector2 const* pV1 ) { pResult->fX = pV0->fX + pV1->fX; pResult->fY = pV0->fY + pV1->fY; }

void Vector4MultScalarNEON( tVector4* pResult, tVector4 const* pV, float fScalar );
float Vector4DotNEON( tVector4 const* pV1, tVector4 const* pV2 );
void Vector4SubtractNEON( tVector4* pResult, tVector4 const* pV1, tVector4 const* pV2 );
void Vector4AddNEON( tVector4* pResult, tVector4 const* pV0, tVector4 const* pV1 );

extern tVector4 gXAxis;
extern tVector4 gYAxis;
extern tVector4 gZAxis;

#endif // __VECTOR_H__