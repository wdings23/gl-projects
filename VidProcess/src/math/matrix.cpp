#include <stdio.h>
#include <memory.h>
#include <math.h>

#include "vector.h"
#include "matrix.h"

/*
 **
 */
void Matrix44Identity( tMatrix44* pMat )
{
	memset( pMat->afEntries, 0, sizeof( pMat->afEntries ) );
	pMat->M( 0, 0 ) = pMat->M( 1, 1 ) = pMat->M( 2, 2 ) = pMat->M( 3, 3 ) = 1.0f;
}

/*
 **
 */
void Matrix44Transpose( tMatrix44* pResult, tMatrix44 const* pMat )
{
#if defined( NEON )
    Matrix44TransposeNEON( pResult, pMat );
#else
	float* pafResultEntries = pResult->afEntries;
	float const* pafEntries = pMat->afEntries;
    
    *(pafResultEntries++) = *pafEntries;
	*(pafResultEntries++) = *(pafEntries+4);
	*(pafResultEntries++) = *(pafEntries+8);
	*(pafResultEntries++) = *(pafEntries+12);
	
	*(pafResultEntries++) = *(pafEntries+1);
	*(pafResultEntries++) = *(pafEntries+5);
	*(pafResultEntries++) = *(pafEntries+9);
	*(pafResultEntries++) = *(pafEntries+13);
	
	*(pafResultEntries++) = *(pafEntries+2);
	*(pafResultEntries++) = *(pafEntries+6);
	*(pafResultEntries++) = *(pafEntries+10);
	*(pafResultEntries++) = *(pafEntries+14);
	
	*(pafResultEntries++) = *(pafEntries+3);
	*(pafResultEntries++) = *(pafEntries+7);
	*(pafResultEntries++) = *(pafEntries+11);
	*(pafResultEntries++) = *(pafEntries+15);
#endif // NEON
}

/*
 **
 */
void Matrix44Inverse( tMatrix44* pResult, tMatrix44 const* pMat )
{
	tMatrix44 tempMat0, tempMat1;
	Matrix44Identity( &tempMat0 );
	Matrix44Identity( &tempMat1 );
	Matrix44Transpose( &tempMat1, pMat );
	
	tempMat0.M( 0, 0 ) = tempMat1.M( 2, 2 ) * tempMat1.M( 3, 3 );
	tempMat0.M( 0, 1 ) = tempMat1.M( 2, 3 ) * tempMat1.M( 3, 2 );
	tempMat0.M( 0, 2 ) = tempMat1.M( 2, 1 ) * tempMat1.M( 3, 3 );
	tempMat0.M( 0, 3 ) = tempMat1.M( 2, 3 ) * tempMat1.M( 3, 1 );
	tempMat0.M( 1, 0 ) = tempMat1.M( 2, 1 ) * tempMat1.M( 3, 2 );
	tempMat0.M( 1, 1 ) = tempMat1.M( 2, 2 ) * tempMat1.M( 3, 1 );
	tempMat0.M( 1, 2 ) = tempMat1.M( 2, 0 ) * tempMat1.M( 3, 3 );
	tempMat0.M( 1, 3 ) = tempMat1.M( 2, 3 ) * tempMat1.M( 3, 0 );
	tempMat0.M( 2, 0 ) = tempMat1.M( 2, 0 ) * tempMat1.M( 3, 2 );
	tempMat0.M( 2, 1 ) = tempMat1.M( 2, 2 ) * tempMat1.M( 3, 0 );
	tempMat0.M( 2, 2 ) = tempMat1.M( 2, 0 ) * tempMat1.M( 3, 1 );
	tempMat0.M( 2, 3 ) = tempMat1.M( 2, 1 ) * tempMat1.M( 3, 0 );
	
	pResult->M( 0, 0 ) = tempMat0.M( 0, 0 ) * tempMat1.M( 1, 1 ) + tempMat0.M( 0, 3 ) * tempMat1.M( 1, 2 ) + tempMat0.M( 1, 0 ) * tempMat1.M( 1, 3 );
	pResult->M( 0, 0 ) -= tempMat0.M( 0, 1 ) * tempMat1.M( 1, 1 ) + tempMat0.M( 0, 2 ) * tempMat1.M( 1, 2 ) + tempMat0.M( 1, 1 ) * tempMat1.M( 1, 3 );
	pResult->M( 0, 1 ) = tempMat0.M( 0, 1 ) * tempMat1.M( 1, 0 ) + tempMat0.M( 1, 2 ) * tempMat1.M( 1, 2 ) + tempMat0.M( 2, 1 ) * tempMat1.M( 1, 3 );
	pResult->M( 0, 1 ) -= tempMat0.M( 0, 0 ) * tempMat1.M( 1, 0 ) + tempMat0.M( 1, 3 ) * tempMat1.M( 1, 2 ) + tempMat0.M( 2, 0 ) * tempMat1.M( 1, 3 );
	pResult->M( 0, 2 ) = tempMat0.M( 0, 2 ) * tempMat1.M( 1, 0 ) + tempMat0.M( 1, 3 ) * tempMat1.M( 1, 1 ) + tempMat0.M( 2, 2 ) * tempMat1.M( 1, 3 );
	pResult->M( 0, 2 ) -= tempMat0.M( 0, 3 ) * tempMat1.M( 1, 0 ) + tempMat0.M( 1, 2 ) * tempMat1.M( 1, 1 ) + tempMat0.M( 2, 3 ) * tempMat1.M( 1, 3 );
	pResult->M( 0, 3 ) = tempMat0.M( 1, 1 ) * tempMat1.M( 1, 0 ) + tempMat0.M( 2, 0 ) * tempMat1.M( 1, 1 ) + tempMat0.M( 2, 3 ) * tempMat1.M( 1, 2 );
	pResult->M( 0, 3 ) -= tempMat0.M( 1, 0 ) * tempMat1.M( 1, 0 ) + tempMat0.M( 2, 1 ) * tempMat1.M( 1, 1 ) + tempMat0.M( 2, 2 ) * tempMat1.M( 1, 2 );
	pResult->M( 1, 0 ) = tempMat0.M( 0, 1 ) * tempMat1.M( 0, 1 ) + tempMat0.M( 0, 2 ) * tempMat1.M( 0, 2 ) + tempMat0.M( 1, 1 ) * tempMat1.M( 0, 3 );
	pResult->M( 1, 0 ) -= tempMat0.M( 0, 0 ) * tempMat1.M( 0, 1 ) + tempMat0.M( 0, 3 ) * tempMat1.M( 0, 2 ) + tempMat0.M( 1, 0 ) * tempMat1.M( 0, 3 );
	pResult->M( 1, 1 ) = tempMat0.M( 0, 0 ) * tempMat1.M( 0, 0 ) + tempMat0.M( 1, 3 ) * tempMat1.M( 0, 2 ) + tempMat0.M( 2, 0 ) * tempMat1.M( 0, 3 );
	pResult->M( 1, 1 ) -= tempMat0.M( 0, 1 ) * tempMat1.M( 0, 0 ) + tempMat0.M( 1, 2 ) * tempMat1.M( 0, 2 ) + tempMat0.M( 2, 1 ) * tempMat1.M( 0, 3 );
	pResult->M( 1, 2 ) = tempMat0.M( 0, 3 ) * tempMat1.M( 0, 0 ) + tempMat0.M( 1, 2 ) * tempMat1.M( 0, 1 ) + tempMat0.M( 2, 3 ) * tempMat1.M( 0, 3 );
	pResult->M( 1, 2 ) -= tempMat0.M( 0, 2 ) * tempMat1.M( 0, 0 ) + tempMat0.M( 1, 3 ) * tempMat1.M( 0, 1 ) + tempMat0.M( 2, 2 ) * tempMat1.M( 0, 3 );
	pResult->M( 1, 3 ) = tempMat0.M( 1, 0 ) * tempMat1.M( 0, 0 ) + tempMat0.M( 2, 1 ) * tempMat1.M( 0, 1 ) + tempMat0.M( 2, 2 ) * tempMat1.M( 0, 2 );
	pResult->M( 1, 3 ) -= tempMat0.M( 1, 1 ) * tempMat1.M( 0, 0 ) + tempMat0.M( 2, 0 ) * tempMat1.M( 0, 1 ) + tempMat0.M( 2, 3 ) * tempMat1.M( 0, 2 );
	
	tempMat0.M( 0, 0 ) = tempMat1.M( 0, 2 ) * tempMat1.M( 1, 3 );
	tempMat0.M( 0, 1 ) = tempMat1.M( 0, 3 ) * tempMat1.M( 1, 2 );
	tempMat0.M( 0, 2 ) = tempMat1.M( 0, 1 ) * tempMat1.M( 1, 3 );
	tempMat0.M( 0, 3 ) = tempMat1.M( 0, 3 ) * tempMat1.M( 1, 1 );
	tempMat0.M( 1, 0 ) = tempMat1.M( 0, 1 ) * tempMat1.M( 1, 2 );
	tempMat0.M( 1, 1 ) = tempMat1.M( 0, 2 ) * tempMat1.M( 1, 1 );
	tempMat0.M( 1, 2 ) = tempMat1.M( 0, 0 ) * tempMat1.M( 1, 3 );
	tempMat0.M( 1, 3 ) = tempMat1.M( 0, 3 ) * tempMat1.M( 1, 0 );
	tempMat0.M( 2, 0 ) = tempMat1.M( 0, 0 ) * tempMat1.M( 1, 2 );
	tempMat0.M( 2, 1 ) = tempMat1.M( 0, 2 ) * tempMat1.M( 1, 0 );
	tempMat0.M( 2, 2 ) = tempMat1.M( 0, 0 ) * tempMat1.M( 1, 1 );
	tempMat0.M( 2, 3 ) = tempMat1.M( 0, 1 ) * tempMat1.M( 1, 0 );
	
	pResult->M( 2, 0 ) = tempMat0.M( 0, 0 ) * tempMat1.M( 3, 1 ) + tempMat0.M( 0, 3 ) * tempMat1.M( 3, 2 ) + tempMat0.M( 1, 0 ) * tempMat1.M( 3, 3 );
	pResult->M( 2, 0 ) -= tempMat0.M( 0, 1 ) * tempMat1.M( 3, 1 ) + tempMat0.M( 0, 2 ) * tempMat1.M( 3, 2 ) + tempMat0.M( 1, 1 ) * tempMat1.M( 3, 3 );
	pResult->M( 2, 1 ) = tempMat0.M( 0, 1 ) * tempMat1.M( 3, 0 ) + tempMat0.M( 1, 2 ) * tempMat1.M( 3, 2 ) + tempMat0.M( 2, 1 ) * tempMat1.M( 3, 3 );
	pResult->M( 2, 1 ) -= tempMat0.M( 0, 0 ) * tempMat1.M( 3, 0 ) + tempMat0.M( 1, 3 ) * tempMat1.M( 3, 2 ) + tempMat0.M( 2, 0 ) * tempMat1.M( 3, 3 );
	pResult->M( 2, 2 ) = tempMat0.M( 0, 2 ) * tempMat1.M( 3, 0 ) + tempMat0.M( 1, 3 ) * tempMat1.M( 3, 1 ) + tempMat0.M( 2, 2 ) * tempMat1.M( 3, 3 );
	pResult->M( 2, 2 ) -= tempMat0.M( 0, 3 ) * tempMat1.M( 3, 0 ) + tempMat0.M( 1, 2 ) * tempMat1.M( 3, 1 ) + tempMat0.M( 2, 3 ) * tempMat1.M( 3, 3 );
	pResult->M( 2, 3 ) = tempMat0.M( 1, 1 ) * tempMat1.M( 3, 0 ) + tempMat0.M( 2, 0 ) * tempMat1.M( 3, 1 ) + tempMat0.M( 2, 3 ) * tempMat1.M( 3, 2 );
	pResult->M( 2, 3 ) -= tempMat0.M( 1, 0 ) * tempMat1.M( 3, 0 ) + tempMat0.M( 2, 1 ) * tempMat1.M( 3, 1 ) + tempMat0.M( 2, 2 ) * tempMat1.M( 3, 2 );
	pResult->M( 3, 0 ) = tempMat0.M( 0, 2 ) * tempMat1.M( 2, 2 ) + tempMat0.M( 1, 1 ) * tempMat1.M( 2, 3 ) + tempMat0.M( 0, 1 ) * tempMat1.M( 2, 1 );
	pResult->M( 3, 0 ) -= tempMat0.M( 1, 0 ) * tempMat1.M( 2, 3 ) + tempMat0.M( 0, 0 ) * tempMat1.M( 2, 1 ) + tempMat0.M( 0, 3 ) * tempMat1.M( 2, 2 );
	pResult->M( 3, 1 ) = tempMat0.M( 2, 0 ) * tempMat1.M( 2, 3 ) + tempMat0.M( 0, 0 ) * tempMat1.M( 2, 0 ) + tempMat0.M( 1, 3 ) * tempMat1.M( 2, 2 );
	pResult->M( 3, 1 ) -= tempMat0.M( 1, 2 ) * tempMat1.M( 2, 2 ) + tempMat0.M( 2, 1 ) * tempMat1.M( 2, 3 ) + tempMat0.M( 0, 1 ) * tempMat1.M( 2, 0 );
	pResult->M( 3, 2 ) = tempMat0.M( 1, 2 ) * tempMat1.M( 2, 1 ) + tempMat0.M( 2, 3 ) * tempMat1.M( 2, 3 ) + tempMat0.M( 0, 3 ) * tempMat1.M( 2, 0 );
	pResult->M( 3, 2 ) -= tempMat0.M( 2, 2 ) * tempMat1.M( 2, 3 ) + tempMat0.M( 0, 2 ) * tempMat1.M( 2, 0 ) + tempMat0.M( 1, 3 ) * tempMat1.M( 2, 1 );
	pResult->M( 3, 3 ) = tempMat0.M( 2, 2 ) * tempMat1.M( 2, 2 ) + tempMat0.M( 1, 0 ) * tempMat1.M( 2, 0 ) + tempMat0.M( 2, 1 ) * tempMat1.M( 2, 1 );
	pResult->M( 3, 3 ) -= tempMat0.M( 2, 0 ) * tempMat1.M( 2, 1 ) + tempMat0.M( 2, 3 ) * tempMat1.M( 2, 2 ) + tempMat0.M( 1, 1 ) * tempMat1.M( 2, 0 );
	
	float fDeterminant = tempMat1.M( 0, 0 ) * pResult->M( 0, 0 ) + tempMat1.M( 0, 1 ) * pResult->M( 0, 1 ) +
    tempMat1.M( 0, 2 ) * pResult->M( 0, 2 ) + tempMat1.M( 0, 3 ) * pResult->M( 0, 3 );
    
	float fOneOverDeterminant = 1.0f / fDeterminant;
	
	pResult->M( 0, 0 ) *= fOneOverDeterminant; pResult->M( 0, 1 ) *= fOneOverDeterminant; pResult->M( 0, 2 ) *= fOneOverDeterminant; pResult->M( 0, 3 ) *= fOneOverDeterminant;
	pResult->M( 1, 0 ) *= fOneOverDeterminant; pResult->M( 1, 1 ) *= fOneOverDeterminant; pResult->M( 1, 2 ) *= fOneOverDeterminant; pResult->M( 1, 3 ) *= fOneOverDeterminant;
	pResult->M( 2, 0 ) *= fOneOverDeterminant; pResult->M( 2, 1 ) *= fOneOverDeterminant; pResult->M( 2, 2 ) *= fOneOverDeterminant; pResult->M( 2, 3 ) *= fOneOverDeterminant;
	pResult->M( 3, 0 ) *= fOneOverDeterminant; pResult->M( 3, 1 ) *= fOneOverDeterminant; pResult->M( 3, 2 ) *= fOneOverDeterminant; pResult->M( 3, 3 ) *= fOneOverDeterminant;
    
}

/*
 **
 */
void Matrix44Transform( tVector4* pResultV, tVector4 const* pV, tMatrix44 const* pMat )
{
#if defined( NEON )
    Matrix44TransformNEON( pResultV, pV, pMat );
#else
	const float* pEntries = &pMat->afEntries[0];
	pResultV->fX = pEntries[0] * pV->fX + pEntries[1] * pV->fY + pEntries[2] * pV->fZ + pEntries[3] * pV->fW;
	pResultV->fY = pEntries[4] * pV->fX + pEntries[5] * pV->fY + pEntries[6] * pV->fZ + pEntries[7] * pV->fW;
	pResultV->fZ = pEntries[8] * pV->fX + pEntries[9] * pV->fY + pEntries[10] * pV->fZ + pEntries[11] * pV->fW;
	pResultV->fW = pEntries[12] * pV->fX + pEntries[13] * pV->fY + pEntries[14] * pV->fZ + pEntries[15] * pV->fW;
#endif // __ARM_NEON__
}

/*
**
*/
void Matrix44Multiply( tMatrix44* pResult, tMatrix44 const* pMat1, tMatrix44 const* pMat2 )
{
    
#if defined( NEON )
    
    Matrix44MultiplyNEON( pResult, pMat1, pMat2 );
#else
	// row 0
	pResult->M( 0, 0 ) = pMat1->M( 0, 0 ) * pMat2->M( 0, 0 ) + 
	pMat1->M( 0, 1 ) * pMat2->M( 1, 0 ) +
	pMat1->M( 0, 2 ) * pMat2->M( 2, 0 ) + 
	pMat1->M( 0, 3 ) * pMat2->M( 3, 0 );
	
	pResult->M( 0, 1 ) = pMat1->M( 0, 0 ) * pMat2->M( 0, 1 ) + 
	pMat1->M( 0, 1 ) * pMat2->M( 1, 1 ) +
	pMat1->M( 0, 2 ) * pMat2->M( 2, 1 ) + 
	pMat1->M( 0, 3 ) * pMat2->M( 3, 1 );	
	
	pResult->M( 0, 2 ) = pMat1->M( 0, 0 ) * pMat2->M( 0, 2 ) + 
	pMat1->M( 0, 1 ) * pMat2->M( 1, 2 ) +
	pMat1->M( 0, 2 ) * pMat2->M( 2, 2 ) + 
	pMat1->M( 0, 3 ) * pMat2->M( 3, 2 );
	
	pResult->M( 0, 3 ) = pMat1->M( 0, 0 ) * pMat2->M( 0, 3 ) + 
	pMat1->M( 0, 1 ) * pMat2->M( 1, 3 ) +
	pMat1->M( 0, 2 ) * pMat2->M( 2, 3 ) + 
	pMat1->M( 0, 3 ) * pMat2->M( 3, 3 );
	
	// row 1
	pResult->M( 1, 0 ) = pMat1->M( 1, 0 ) * pMat2->M( 0, 0 ) + 
	pMat1->M( 1, 1 ) * pMat2->M( 1, 0 ) +
	pMat1->M( 1, 2 ) * pMat2->M( 2, 0 ) + 
	pMat1->M( 1, 3 ) * pMat2->M( 3, 0 );
	
	pResult->M( 1, 1 ) = pMat1->M( 1, 0 ) * pMat2->M( 0, 1 ) + 
	pMat1->M( 1, 1 ) * pMat2->M( 1, 1 ) +
	pMat1->M( 1, 2 ) * pMat2->M( 2, 1 ) + 
	pMat1->M( 1, 3 ) * pMat2->M( 3, 1 );	
	
	pResult->M( 1, 2 ) = pMat1->M( 1, 0 ) * pMat2->M( 0, 2 ) + 
	pMat1->M( 1, 1 ) * pMat2->M( 1, 2 ) +
	pMat1->M( 1, 2 ) * pMat2->M( 2, 2 ) + 
	pMat1->M( 1, 3 ) * pMat2->M( 3, 2 );
	
	pResult->M( 1, 3 ) = pMat1->M( 1, 0 ) * pMat2->M( 0, 3 ) + 
	pMat1->M( 1, 1 ) * pMat2->M( 1, 3 ) +
	pMat1->M( 1, 2 ) * pMat2->M( 2, 3 ) + 
	pMat1->M( 1, 3 ) * pMat2->M( 3, 3 );
	
	// row 2
	pResult->M( 2, 0 ) = pMat1->M( 2, 0 ) * pMat2->M( 0, 0 ) + 
	pMat1->M( 2, 1 ) * pMat2->M( 1, 0 ) +
	pMat1->M( 2, 2 ) * pMat2->M( 2, 0 ) + 
	pMat1->M( 2, 3 ) * pMat2->M( 3, 0 );
	
	pResult->M( 2, 1 ) = pMat1->M( 2, 0 ) * pMat2->M( 0, 1 ) + 
	pMat1->M( 2, 1 ) * pMat2->M( 1, 1 ) +
	pMat1->M( 2, 2 ) * pMat2->M( 2, 1 ) + 
	pMat1->M( 2, 3 ) * pMat2->M( 3, 1 );	
	
	pResult->M( 2, 2 ) = pMat1->M( 2, 0 ) * pMat2->M( 0, 2 ) + 
	pMat1->M( 2, 1 ) * pMat2->M( 1, 2 ) +
	pMat1->M( 2, 2 ) * pMat2->M( 2, 2 ) + 
	pMat1->M( 2, 3 ) * pMat2->M( 3, 2 );
	
	pResult->M( 2, 3 ) = pMat1->M( 2, 0 ) * pMat2->M( 0, 3 ) + 
	pMat1->M( 2, 1 ) * pMat2->M( 1, 3 ) +
	pMat1->M( 2, 2 ) * pMat2->M( 2, 3 ) + 
	pMat1->M( 2, 3 ) * pMat2->M( 3, 3 );
	
	// row 3
	pResult->M( 3, 0 ) = pMat1->M( 3, 0 ) * pMat2->M( 0, 0 ) + 
	pMat1->M( 3, 1 ) * pMat2->M( 1, 0 ) +
	pMat1->M( 3, 2 ) * pMat2->M( 2, 0 ) + 
	pMat1->M( 3, 3 ) * pMat2->M( 3, 0 );
	
	pResult->M( 3, 1 ) = pMat1->M( 3, 0 ) * pMat2->M( 0, 1 ) + 
	pMat1->M( 3, 1 ) * pMat2->M( 1, 1 ) +
	pMat1->M( 3, 2 ) * pMat2->M( 2, 1 ) + 
	pMat1->M( 3, 3 ) * pMat2->M( 3, 1 );	
	
	pResult->M( 3, 2 ) = pMat1->M( 3, 0 ) * pMat2->M( 0, 2 ) + 
	pMat1->M( 3, 1 ) * pMat2->M( 1, 2 ) +
	pMat1->M( 3, 2 ) * pMat2->M( 2, 2 ) + 
	pMat1->M( 3, 3 ) * pMat2->M( 3, 2 );
	
	pResult->M( 3, 3 ) = pMat1->M( 3, 0 ) * pMat2->M( 0, 3 ) + 
	pMat1->M( 3, 1 ) * pMat2->M( 1, 3 ) +
	pMat1->M( 3, 2 ) * pMat2->M( 2, 3 ) + 
	pMat1->M( 3, 3 ) * pMat2->M( 3, 3 );
#endif // __ARM_NEON__
}

/*
 **
 */
void Matrix44RotateX( tMatrix44* pResult, float fXAngle )
{
	float fCos = cos( fXAngle );
	float fSin = sin( fXAngle );
	
	pResult->M( 0, 0 ) = 1.0f; pResult->M( 0, 1 ) = pResult->M( 0, 2 ) = pResult->M( 0, 3 ) = 0.0f;
	pResult->M( 1, 0 ) = 0.0f; pResult->M( 1, 1 ) = fCos/*cos( fXAngle )*/; pResult->M( 1, 2 ) = -fSin/*-sin( fXAngle )*/; pResult->M( 1, 3 ) = 0.0f;
	pResult->M( 2, 0 ) = 0.0f; pResult->M( 2, 1 ) = fSin/*sin( fXAngle )*/; pResult->M( 2, 2 ) = fCos/*cos( fXAngle )*/; pResult->M( 2, 3 ) = 0.0f;
	pResult->M( 3, 0 ) = pResult->M( 3, 1 ) = pResult->M( 3, 2 ) = 0.0f; pResult->M( 3, 3 ) = 1.0f;
}

/*
 **
 */
void Matrix44RotateY( tMatrix44* pResult, float fYAngle )
{
	float fCos = cos( fYAngle );
	float fSin = sin( fYAngle );
	
	pResult->M( 0, 0 ) = fCos/*cos( fYAngle )*/; pResult->M( 0, 1 ) = 0.0f; pResult->M( 0, 2 ) = fSin/*sin( fYAngle )*/; pResult->M( 0, 3 ) = 0.0f;
	pResult->M( 1, 0 ) = 0.0f; pResult->M( 1, 1 ) = 1.0f; pResult->M( 1, 2 ) = pResult->M( 1, 3 ) = 0.0f;
	pResult->M( 2, 0 ) = -fSin/*-sin( fYAngle )*/; pResult->M( 2, 1 ) = 0.0f; pResult->M( 2, 2 ) = fCos/*cos( fYAngle )*/; pResult->M( 2, 3 ) = 0.0f;
	pResult->M( 3, 0 ) = pResult->M( 3, 1 ) = pResult->M( 3, 2 ) = 0.0f; pResult->M( 3, 3 ) = 1.0f;
}

/*
 **
 */
void Matrix44RotateZ( tMatrix44* pResult, float fZAngle )
{
	float fCos = cos( fZAngle );
	float fSin = sin( fZAngle );
	
	pResult->M( 0, 0 ) = fCos/*cos( fZAngle )*/; pResult->M( 0, 1 ) = -fSin/*-sin( fZAngle )*/; pResult->M( 0, 2 ) = pResult->M( 0, 3 ) = 0.0f;
	pResult->M( 1, 0 ) = fSin/*sin( fZAngle )*/; pResult->M( 1, 1 ) = fCos/*cos( fZAngle )*/; pResult->M( 1, 2 ) = pResult->M( 1, 3 ) = 0.0f;
	pResult->M( 2, 0 ) = pResult->M( 2, 1 ) = 0.0f; pResult->M( 2, 2 ) = 1.0f; pResult->M( 2, 3 ) = 0.0f;
	pResult->M( 3, 0 ) = pResult->M( 3, 1 ) = pResult->M( 3, 2 ) = 0.0f; pResult->M( 3, 3 ) = 1.0f;
}

/*
 **
 */
void Matrix44RotateXRH( tMatrix44* pResult, float fXAngle )
{
	pResult->M( 0, 0 ) = 1.0f; pResult->M( 0, 1 ) = pResult->M( 0, 2 ) = pResult->M( 0, 3 ) = 0.0f;
	pResult->M( 1, 0 ) = 0.0f; pResult->M( 1, 1 ) = cos( fXAngle ); pResult->M( 1, 2 ) = sin( fXAngle ); pResult->M( 1, 3 ) = 0.0f;
	pResult->M( 2, 0 ) = 0.0f; pResult->M( 2, 1 ) = -sin( fXAngle ); pResult->M( 2, 2 ) = cos( fXAngle ); pResult->M( 2, 3 ) = 0.0f;
	pResult->M( 3, 0 ) = pResult->M( 3, 1 ) = pResult->M( 3, 2 ) = 0.0f; pResult->M( 3, 3 ) = 1.0f;
}

/*
 **
 */
void Matrix44RotateYRH( tMatrix44* pResult, float fYAngle )
{
	pResult->M( 0, 0 ) = cos( fYAngle ); pResult->M( 0, 1 ) = 0.0f; pResult->M( 0, 2 ) = -sin( fYAngle ); pResult->M( 0, 3 ) = 0.0f;
	pResult->M( 1, 0 ) = 0.0f; pResult->M( 1, 1 ) = 1.0f; pResult->M( 1, 2 ) = pResult->M( 1, 3 ) = 0.0f;
	pResult->M( 2, 0 ) = sin( fYAngle ); pResult->M( 2, 1 ) = 0.0f; pResult->M( 2, 2 ) = cos( fYAngle ); pResult->M( 2, 3 ) = 0.0f;
	pResult->M( 3, 0 ) = pResult->M( 3, 1 ) = pResult->M( 3, 2 ) = 0.0f; pResult->M( 3, 3 ) = 1.0f;
}

/*
 **
 */
void Matrix44RotateZRH( tMatrix44* pResult, float fZAngle )
{
	pResult->M( 0, 0 ) = cos( fZAngle ); pResult->M( 0, 1 ) = sin( fZAngle ); pResult->M( 0, 2 ) = pResult->M( 0, 3 ) = 0.0f;
	pResult->M( 1, 0 ) = -sin( fZAngle ); pResult->M( 1, 1 ) = cos( fZAngle ); pResult->M( 1, 2 ) = pResult->M( 1, 3 ) = 0.0f;
	pResult->M( 2, 0 ) = pResult->M( 2, 1 ) = 0.0f; pResult->M( 2, 2 ) = 1.0f; pResult->M( 2, 3 ) = 0.0f;
	pResult->M( 3, 0 ) = pResult->M( 3, 1 ) = pResult->M( 3, 2 ) = 0.0f; pResult->M( 3, 3 ) = 1.0f;
}


/*
 **
 */
void Matrix44LookAt( tMatrix44* pResult, 
                    tVector4* pEye, 
                    tVector4* pLookAtPt, 
                    tVector4* pUp, 
                    tMatrix44* pInvRotMatrix,
                    int iWideScreen )
{
	tVector4 fVec, sVec, uVec;
	tMatrix44 transform, translation;
	
	Matrix44Identity( &transform );
	Matrix44Identity( &translation );
    
	Vector4Subtract( &fVec, pLookAtPt, pEye );
	Vector4Normalize( &fVec, &fVec );
	Vector4Normalize( pUp, pUp );
    
	Vector4Cross( &sVec, pUp, &fVec );
	Vector4Normalize( &sVec, &sVec );
	Vector4Cross( &uVec, &fVec, &sVec );
	Vector4Normalize( &uVec, &uVec );
    
	transform.M( 0, 0 ) = sVec.fX; transform.M( 0, 1 ) = sVec.fY; transform.M( 0, 2 ) = sVec.fZ;
	transform.M( 1, 0 ) = uVec.fX; transform.M( 1, 1 ) = uVec.fY; transform.M( 1, 2 ) = uVec.fZ;
	transform.M( 2, 0 ) = -fVec.fX; transform.M( 2, 1 ) = -fVec.fY; transform.M( 2, 2 ) = -fVec.fZ;
	
	Matrix44Inverse( pInvRotMatrix, &transform );
	
	Matrix44Identity( &translation );
	tVector4 negEye = { -pEye->fX, -pEye->fY, -pEye->fZ, 1.0f };
	Matrix44Translate( &translation, &negEye );
	
	if( iWideScreen )
	{
		// rotate the screen
		
		tMatrix44 lookAt, rotate; 
		Matrix44Multiply( &lookAt, &transform, &translation );
		Matrix44RotateZ( &rotate, 1.57f );
		Matrix44Multiply( pResult, &rotate, &lookAt );
	}
	else
	{
		Matrix44Multiply( pResult, &transform, &translation );
	}
}

/*
 **
 */
void Matrix44Translate( tMatrix44* pResult, tVector4 const* pV )
{
	Matrix44Identity( pResult );
	pResult->M( 0, 3 ) = pV->fX;
	pResult->M( 1, 3 ) = pV->fY;
	pResult->M( 2, 3 ) = pV->fZ;
}

/*
 **
 */
void Matrix44Scale( tMatrix44* pResult, float fXScale, float fYScale, float fZScale )
{
	Matrix44Identity( pResult );
	pResult->M( 0, 0 ) = fXScale;
	pResult->M( 1, 1 ) = fYScale;
	pResult->M( 2, 2 ) = fZScale;
}
//#if 0
/*
 **
 */
void Matrix44Perspective( tMatrix44* pResult,
                         float fFOV,
                         unsigned int iWidth,
                         unsigned int iHeight,
                         float fFar,
                         float fNear )
{
    //#if 0
    float fFD = 1.0f / tan( fFOV * 0.5f );
    float fAspect = (float)iWidth / (float)iHeight;
    float fOneOverAspect = 1.0f / fAspect;
    float fOneOverFarMinusNear = 1.0f / ( fFar - fNear );
    
    pResult->M( 0, 0 ) = fFD * fOneOverAspect;
    pResult->M( 0, 1 ) = 0.0f;
    pResult->M( 0, 2 ) = 0.0f;
    pResult->M( 0, 3 ) = 0.0f;
    
    pResult->M( 1, 0 ) = 0.0f;
    pResult->M( 1, 1 ) = fFD;
    pResult->M( 1, 2 ) = 0.0f;
    pResult->M( 1, 3 ) = 0.0f;
    
    pResult->M( 2, 0 ) = 0.0f;
    pResult->M( 2, 1 ) = 0.0f;
    pResult->M( 2, 2 ) = -( fFar + fNear ) * fOneOverFarMinusNear;
    pResult->M( 2, 3 ) = -1.0f;
    
    pResult->M( 3, 0 ) = 0.0f;
    pResult->M( 3, 1 ) = 0.0f;
    pResult->M( 3, 2 ) = -1.0f * fFar * fNear * fOneOverFarMinusNear;
    pResult->M( 3, 3 ) = 0.0f;
    //#endif // #if 0
    
#if 0
	float fAspect = (float)iWidth / (float)iHeight;
	float fYScale = atan( fFOV * 0.5f );
	float fXScale = fYScale / fAspect;
	
	pResult->M( 0, 0 ) = fXScale;
    pResult->M( 0, 1 ) = 0.0f;
    pResult->M( 0, 2 ) = 0.0f;
    pResult->M( 0, 3 ) = 0.0f;
    
    pResult->M( 1, 0 ) = 0.0f;
    pResult->M( 1, 1 ) = fYScale;
    pResult->M( 1, 2 ) = 0.0f;
    pResult->M( 1, 3 ) = 0.0f;
    
    pResult->M( 2, 0 ) = 0.0f;
    pResult->M( 2, 1 ) = 0.0f;
    pResult->M( 2, 2 ) = fFar / ( fNear - fFar );
    pResult->M( 2, 3 ) = -1.0f;
    
    pResult->M( 3, 0 ) = 0.0f;
    pResult->M( 3, 1 ) = 0.0f;
    pResult->M( 3, 2 ) = fNear * fFar / ( fNear - fFar );
    pResult->M( 3, 3 ) = 0.0f;
#endif // #if 0
}
//#endif // #if 0

/*
**
*/
void Matrix44Orthographic( tMatrix44* pResult, 
						   float fLeft, 
						   float fRight,
						   float fBottom,
						   float fTop,
						   float fNear,
						   float fFar )
{
	// near <= z <= far 
	// 0 <= z - near <= far - near
	// 0 <= 2 * ( z - near ) <= 2 * ( far - near )
	// 0 <= ( 2 * ( z - near ) ) / ( far - near ) <= 2
	// -1 <= ( 2 * ( z - near ) ) / ( far - near ) - 1 <= 1
	// -1 <= ( 2 * ( z - near ) ) / ( far - near ) - ( far - near ) / ( far - near ) <= 1
	// -1 <= ( 2 * z / ( far - near ) ) - 
	//		 ( ( 2 * near ) / ( far - near ) ) - 
	//		 ( ( far - near ) / ( far - near ) ) <= 1
	// -1 <= 2 * z / ( far - near ) - ( far + near ) / ( far - near ) <= 1

	Matrix44Identity( pResult );

	pResult->M( 0, 0 ) = 2.0f / ( fRight - fLeft );
	pResult->M( 0, 3 ) = -( fRight + fLeft ) / ( fRight - fLeft );

	pResult->M( 1, 1 ) = 2.0f / ( fTop - fBottom );
	pResult->M( 1, 3 ) = -( fTop + fBottom ) / ( fTop - fBottom );

	pResult->M( 2, 2 ) = -2.0f / ( fFar - fNear );
	pResult->M( 2, 3 ) = -( fFar + fNear ) / ( fFar - fNear );
}

#if 0
/*
 **
 */
#define PI_OVER_360 0.008726646f
void Matrix44Perspective( tMatrix44* pResult, 
                         float fFOV,
                         unsigned int iWidth,
                         unsigned int iHeight,
                         float fZNear,
                         float fZFar )
{
	float fAspect = (float)iWidth / (float)iHeight;
	float fXYMax = fZNear * tan( fFOV * ( 3.14159f / 360.0f ) );
	float fNegDepth = fZNear - fZFar;
	float fOneOverNegDepth = 1.0f / fNegDepth;
	pResult->M( 0, 0 ) = fXYMax / fAspect;
	pResult->M( 0, 1 ) = 0.0f;
	pResult->M( 0, 2 ) = 0.0f;
	pResult->M( 0, 3 ) = 0.0f;
    
	pResult->M( 1, 0 ) = 0.0f;
	pResult->M( 1, 1 ) = fXYMax;
	pResult->M( 1, 2 ) = 0.0f;
	pResult->M( 1, 3 ) = 0.0f;
    
	pResult->M( 2, 0 ) = 0.0f;
	pResult->M( 2, 1 ) = 0.0f;
	pResult->M( 2, 2 ) = ( fZFar + fZNear ) * fOneOverNegDepth;
	pResult->M( 2, 3 ) = -1.0f;
    
	pResult->M( 3, 0 ) = 0.0f;
	pResult->M( 3, 1 ) = 0.0f;
	pResult->M( 3, 2 ) = 2.0f * ( fZNear * fZFar ) * fOneOverNegDepth;
	pResult->M( 3, 3 ) = 0.0f;
}
#endif // #if 0

/*
 **
 */
void Matrix44Print( tMatrix44 const* pMat )
{
	int i, j;
    
	OUTPUT( "\n" );
	for( i = 0; i < 4; i++ )
	{
		for( j = 0; j < 4; j++ )
		{
			OUTPUT( "M( %d, %d ):%f ", i, j, pMat->M( i, j ) );
		}
		OUTPUT( "\n" );
	}
	
	OUTPUT( "\n" );
}

#if defined( NEON )

/*
**
*/
void Matrix44MultiplyNEON( tMatrix44* pResult,
                           tMatrix44 const* pMat1,
                           tMatrix44 const* pMat2 )
{
    asm volatile (
        "vldmia     %1, {q0,q1,q2,q3}                   \n\t"        // load matrix 1 into q0, q1, q2, q3
        "vldmia     %2, {q4,q5,q6,q7}                   \n\t"        // load matrix 2 into q4, q5, q6, q7        
                  
        // multiply row 0
        "vmul.F32   q8, q4, d0[0]                       \n\t"
        "vmla.F32   q8, q5, d0[1]                       \n\t"
        "vmla.F32   q8, q6, d1[0]                       \n\t"
        "vmla.F32   q8, q7, d1[1]                       \n\t"
        
        // multiply row 1
        "vmul.F32   q9, q4, d2[0]                       \n\t"
        "vmla.F32   q9, q5, d2[1]                       \n\t"
        "vmla.F32   q9, q6, d3[0]                       \n\t"
        "vmla.F32   q9, q7, d3[1]                       \n\t"
                  
        // multiply row 2
        "vmul.F32   q10, q4, d4[0]                       \n\t"
        "vmla.F32   q10, q5, d4[1]                       \n\t"
        "vmla.F32   q10, q6, d5[0]                       \n\t"
        "vmla.F32   q10, q7, d5[1]                       \n\t"
      
        // multiply row 3
        "vmul.F32   q11, q4, d6[0]                       \n\t"
        "vmla.F32   q11, q5, d6[1]                       \n\t"
        "vmla.F32   q11, q6, d7[0]                       \n\t"
        "vmla.F32   q11, q7, d7[1]                       \n\t"
        
        "vstmia     %0, {q8,q9,q10,q11}                   \n\t"        // store into result
                  
        :
        : "r" (pResult->afEntries), "r" (pMat1->afEntries), "r" (pMat2->afEntries)
        : "q0","q1","q2","q3","q4","q5","q6","q7","q8","q9","q10","q11","q12","memory"
    );

}

/*
**
*/
void Matrix44TransposeNEON( tMatrix44* pResult, tMatrix44 const* pMat )
{
    asm volatile (
      "vldmia     %1, {q0,q1,q2,q3}                     \n\t"        // load matrix 1 into q0, q1, q2, q3
      
      "vzip.32    q0, q2                                \n\t"
      "vzip.32    q1, q3                                \n\t"
      "vzip.32    q0, q1                                \n\t"
      "vzip.32    q2, q3                                \n\t"
                  
      "vstmia     %0, {q0,q1,q2,q3}                   \n\t"        // store into result
      
      :
      : "r" (pResult->afEntries), "r" (pMat->afEntries)
      : "q0","q1","q2","q3","q4","q5","q6","q7","q8","q9","q10","q11","q12","memory"
    );
}

/*
**
*/
void Matrix44TransformNEON( tVector4* pResultV, tVector4 const* pV, tMatrix44 const* pMat )
{
    asm volatile (
      "vldmia     %1, {q0,q1,q2,q3}                     \n\t"        // load matrix 1 into q0, q1, q2, q3
      "vldmia     %2, {q4}                              \n\t"        // load vector into q4
          
      // transpose matrix
      "vzip.32    q0, q2                                \n\t"
      "vzip.32    q1, q3                                \n\t"
      "vzip.32    q0, q1                                \n\t"
      "vzip.32    q2, q3                                \n\t"
                  
      "vmul.F32   q5, q0, d8[0]                            \n\t"
      "vmla.F32   q5, q1, d8[1]                            \n\t"
      "vmla.F32   q5, q2, d9[0]                            \n\t"
      "vmla.F32   q5, q3, d9[1]                            \n\t"
    
      "vstmia     %0, {q5}                              \n\t"        // store into result
      
      :
      : "r" (pResultV), "r" (pMat->afEntries), "r" (pV)
      : "q0","q1","q2","q3","q4","q5","memory"
    );
}

#endif // __ARM_NEON__