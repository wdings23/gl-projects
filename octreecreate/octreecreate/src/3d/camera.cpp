#include "camera.h"

#include <stdio.h>
#include <string.h>

/*
 **
 */
CCamera* CCamera::mpInstance = NULL;
CCamera* CCamera::instance( void )
{
	if( mpInstance == NULL )
	{
		mpInstance = new CCamera();
	}
    
	return mpInstance;
}

/*
 **
 */
CCamera::CCamera( void )
{
	Matrix44Identity( &mViewMatrix );
	Matrix44Identity( &mInvRotMatrix );
    
	mPosition.fX = 0.0f; mPosition.fY = 0.0f; mPosition.fZ = 0.0f; mPosition.fW = 1.0f;
	mLookAt.fX = 0.0f; mLookAt.fY = 0.0f; mLookAt.fZ = 0.0f; mLookAt.fW = 1.0f;
	mUp.fX = 0.0f; mUp.fY = 1.0f; mUp.fZ = 0.0f; mUp.fW = 1.0f;
    
	mfFOV = 45.0f * ( 3.14159f / 180.0f );
	mfNear = 1.0f;
	mfFar = 1000.0f;
	mfRatio = 1.0f;
}

/*
 **
 */
CCamera::~CCamera( void )
{
}

/*
 **
 */
void CCamera::update( int iScreenWidth, int iScreenHeight )
{
	//mfFOV = 2.0f * atanf( ( (float)iScreenHeight * 0.5f ) / ( mfFar - mfNear ) );
	//float fRatio = (float)iScreenWidth / (float)iScreenHeight;
	//mfFOV *= fRatio;
	
	// update view matrix
	Matrix44LookAt( &mViewMatrix,
                   &mPosition,
                   &mLookAt,
                   &mUp,
                   &mInvRotMatrix,
                   false );
	
	Vector4Subtract( &mLookAtNormalized, &mPosition, &mLookAt );
	Vector4Normalize( &mLookAtNormalized, &mLookAtNormalized );
    
	Matrix44Perspective( &mProjectionMatrix,
                        mfFOV,
                        iScreenWidth,
                        iScreenHeight,
                        mfFar,
                        mfNear );
	
	tMatrix44 transpose;
	Matrix44Transpose( &transpose, &mViewMatrix );
	Matrix44Multiply( &mViewProjMatrix, &transpose, &mProjectionMatrix );
    
	calcFrustum( iScreenWidth, iScreenHeight );
}

/*
 **
 */
void CCamera::setPosition( tVector4 const* pPosition )
{
	memcpy( &mPosition, pPosition, sizeof( mPosition ) );
}

/*
**
*/
void CCamera::setLookAt( tVector4 const* pLookAt )
{
	memcpy( &mLookAt, pLookAt, sizeof( mLookAt ) );
}

/*
 **
 */
void CCamera::setUp( tVector4* pUp )
{
	memcpy( &mUp, pUp, sizeof( mUp ) );
}

/*
 **
 */
void CCamera::calcFrustum( int iScreenWidth, int iScreenHeight )
{
	// screen width to height ratio
    float fRatio = (float)iScreenWidth / (float)iScreenHeight;
    
    // get near frustum dimension
	float fNearHeight = 2.0f * tan( mfFOV * 0.5f ) * mfNear;
	float fNearWidth = fNearHeight * fRatio;
	
    // get far frustum dimension
	float fFarHeight = 2.0f * tan( mfFOV * 0.5f ) * mfFar;
	float fFarWidth = fFarHeight * fRatio;
	
	tVector4 farCenter, farTopLeft, farTopRight, farBottomLeft, farBottomRight;
	tVector4 lookDir, right, up;
	
    // look direction
    Vector4Subtract( &lookDir, &mLookAt, &mPosition );
	Vector4Normalize( &lookDir, &lookDir );
	
    // get the right vector for the corners
    Vector4Cross( &right, &mUp, &lookDir );
	right.fW = 1.0f;
	Vector4Cross( &up, &lookDir, &right );
	up.fW = 1.0f;
	
    // far frustum corners
	farCenter.fX = mPosition.fX + lookDir.fX * mfFar;
	farCenter.fY = mPosition.fY + lookDir.fY * mfFar;
	farCenter.fZ = mPosition.fZ + lookDir.fZ * mfFar;
	farCenter.fW = 1.0f;
	
	farTopLeft.fX = farCenter.fX + ( mUp.fX * fFarHeight * 0.5f ) - ( right.fX * fFarWidth * 0.5f );
	farTopLeft.fY = farCenter.fY + ( mUp.fY * fFarHeight * 0.5f ) - ( right.fY * fFarWidth * 0.5f );
	farTopLeft.fZ = farCenter.fZ + ( mUp.fZ * fFarHeight * 0.5f ) - ( right.fZ * fFarWidth * 0.5f);
	farTopLeft.fW = 1.0f;
	
	farTopRight.fX = farCenter.fX + ( mUp.fX * fFarHeight * 0.5f ) + ( right.fX * fFarWidth * 0.5f );
	farTopRight.fY = farCenter.fY + ( mUp.fY * fFarHeight * 0.5f ) + ( right.fY * fFarWidth * 0.5f );
	farTopRight.fZ = farCenter.fZ + ( mUp.fZ * fFarHeight * 0.5f ) + ( right.fZ * fFarWidth * 0.5f );
	farTopRight.fW = 1.0f;
	
	farBottomLeft.fX = farCenter.fX - ( mUp.fX * fFarHeight * 0.5f ) - ( right.fX * fFarWidth * 0.5f );
	farBottomLeft.fY = farCenter.fY - ( mUp.fY * fFarHeight * 0.5f ) - ( right.fY * fFarWidth * 0.5f );
	farBottomLeft.fZ = farCenter.fZ - ( mUp.fZ * fFarHeight * 0.5f ) - ( right.fZ * fFarWidth * 0.5f );
	farBottomLeft.fW = 1.0f;
	
	farBottomRight.fX = farCenter.fX - ( mUp.fX * fFarHeight * 0.5f ) + ( right.fX * fFarWidth * 0.5f );
	farBottomRight.fY = farCenter.fY - ( mUp.fY * fFarHeight * 0.5f ) + ( right.fY * fFarWidth * 0.5f );
	farBottomRight.fZ = farCenter.fZ - ( mUp.fZ * fFarHeight * 0.5f ) + ( right.fZ * fFarWidth * 0.5f );
	farBottomRight.fW = 1.0f;
	
    // near frustum corners
	tVector4 nearCenter, nearTopLeft, nearTopRight, nearBottomLeft, nearBottomRight;
	
	nearCenter.fX = mPosition.fX + mfNear * lookDir.fX;
	nearCenter.fY = mPosition.fY + mfNear * lookDir.fY;
	nearCenter.fZ = mPosition.fZ + mfNear * lookDir.fZ;
	nearCenter.fW = 1.0f;
	
	nearTopLeft.fX = nearCenter.fX + ( up.fX * fNearHeight * 0.5f ) - ( right.fX * fNearWidth * 0.5f );
	nearTopLeft.fY = nearCenter.fY + ( up.fY * fNearHeight * 0.5f ) - ( right.fY * fNearWidth * 0.5f );
	nearTopLeft.fZ = nearCenter.fZ + ( up.fZ * fNearHeight * 0.5f ) - ( right.fZ * fNearWidth * 0.5f );
	nearTopLeft.fW = 1.0f;
	
	nearTopRight.fX = nearCenter.fX + ( up.fX * fNearHeight * 0.5f ) + ( right.fX * fNearWidth * 0.5f );
	nearTopRight.fY = nearCenter.fY + ( up.fY * fNearHeight * 0.5f ) + ( right.fY * fNearWidth * 0.5f );
	nearTopRight.fZ = nearCenter.fZ + ( up.fZ * fNearHeight * 0.5f ) + ( right.fZ * fNearWidth * 0.5f );
	nearTopRight.fW = 1.0f;
	
	nearBottomLeft.fX = nearCenter.fX - ( up.fX * fNearHeight * 0.5f ) - ( right.fX * fNearWidth * 0.5f );
	nearBottomLeft.fY = nearCenter.fY - ( up.fY * fNearHeight * 0.5f ) - ( right.fY * fNearWidth * 0.5f );
	nearBottomLeft.fZ = nearCenter.fZ - ( up.fZ * fNearHeight * 0.5f ) - ( right.fZ * fNearWidth * 0.5f );
	nearBottomLeft.fW = 1.0f;
	
	nearBottomRight.fX = nearCenter.fX - ( up.fX * fNearHeight * 0.5f ) + ( right.fX * fNearWidth * 0.5f );
	nearBottomRight.fY = nearCenter.fY - ( up.fY * fNearHeight * 0.5f ) + ( right.fY * fNearWidth * 0.5f );
	nearBottomRight.fZ = nearCenter.fZ - ( up.fZ * fNearHeight * 0.5f ) + ( right.fZ * fNearWidth * 0.5f );
	nearBottomRight.fW = 1.0f;
	
	memcpy( &mFarTopLeft, &farTopLeft, sizeof( tVector4 ) );
	memcpy( &mFarTopRight, &farTopRight, sizeof( tVector4 ) );
	memcpy( &mFarBottomLeft, &farBottomLeft, sizeof( tVector4 ) );
	memcpy( &mFarBottomRight, &farBottomRight, sizeof( tVector4 ) );
    
	memcpy( &mNearTopLeft, &nearTopLeft, sizeof( tVector4 ) );
	memcpy( &mNearTopRight, &nearTopRight, sizeof( tVector4 ) );
	memcpy( &mNearBottomLeft, &nearBottomLeft, sizeof( tVector4 ) );
	memcpy( &mNearBottomRight, &nearBottomRight, sizeof( tVector4 ) );
	
	memcpy( &mNearCenter, &nearCenter, sizeof( tVector4 ) );
	memcpy( &mFarCenter, &farCenter, sizeof( tVector4 ) );
    
    // near plane
	tVector4 v0, v1;
	Vector4Subtract( &v0, &nearTopRight, &nearTopLeft );
	Vector4Subtract( &v1, &nearBottomLeft, &nearTopLeft );
	Vector4Cross( &mFrustum.maPlanes[FRUSTUM_PLANE_NEAR], &v1, &v0 );
	Vector4Normalize( &mFrustum.maPlanes[FRUSTUM_PLANE_NEAR], &mFrustum.maPlanes[FRUSTUM_PLANE_NEAR] );
	mFrustum.maPlanes[FRUSTUM_PLANE_NEAR].fW = -Vector4Dot( &mFrustum.maPlanes[FRUSTUM_PLANE_NEAR], &nearTopLeft );
	
    // far plane
	Vector4Subtract( &v0, &farTopRight, &farTopLeft );
	Vector4Subtract( &v1, &farBottomLeft, &farTopLeft );
	Vector4Cross( &mFrustum.maPlanes[FRUSTUM_PLANE_FAR], &v0, &v1 );
	Vector4Normalize( &mFrustum.maPlanes[FRUSTUM_PLANE_FAR], &mFrustum.maPlanes[FRUSTUM_PLANE_FAR] );
	mFrustum.maPlanes[FRUSTUM_PLANE_FAR].fW = -Vector4Dot( &mFrustum.maPlanes[FRUSTUM_PLANE_FAR], &farTopLeft );
	
    // left plane
	Vector4Subtract( &v0, &farTopLeft, &nearTopLeft );
	Vector4Subtract( &v1, &farBottomLeft, &farTopLeft );
	Vector4Cross( &mFrustum.maPlanes[FRUSTUM_PLANE_LEFT], &v0, &v1 );
	Vector4Normalize( &mFrustum.maPlanes[FRUSTUM_PLANE_LEFT], &mFrustum.maPlanes[FRUSTUM_PLANE_LEFT] );
	mFrustum.maPlanes[FRUSTUM_PLANE_LEFT].fW = -Vector4Dot( &mFrustum.maPlanes[FRUSTUM_PLANE_LEFT], &farTopLeft );
	
    // right plane
	Vector4Subtract( &v0, &farTopRight, &nearTopRight );
	Vector4Subtract( &v1, &farBottomRight, &farTopRight );
	Vector4Cross( &mFrustum.maPlanes[FRUSTUM_PLANE_RIGHT], &v1, &v0 );
	Vector4Normalize( &mFrustum.maPlanes[FRUSTUM_PLANE_RIGHT], &mFrustum.maPlanes[FRUSTUM_PLANE_RIGHT] );
	mFrustum.maPlanes[FRUSTUM_PLANE_RIGHT].fW = -Vector4Dot( &mFrustum.maPlanes[FRUSTUM_PLANE_RIGHT], &farTopRight );
	
    // top plane
	Vector4Subtract( &v0, &farTopRight, &nearTopRight );
	Vector4Subtract( &v1, &farTopLeft, &farTopRight );
	Vector4Cross( &mFrustum.maPlanes[FRUSTUM_PLANE_TOP], &v0, &v1 );
	Vector4Normalize( &mFrustum.maPlanes[FRUSTUM_PLANE_TOP], &mFrustum.maPlanes[FRUSTUM_PLANE_TOP] );
	mFrustum.maPlanes[FRUSTUM_PLANE_TOP].fW = -Vector4Dot( &mFrustum.maPlanes[FRUSTUM_PLANE_TOP], &farTopRight );
	
    // bottom plane
	Vector4Subtract( &v0, &farBottomRight, &nearBottomLeft );
	Vector4Subtract( &v1, &farBottomLeft, &nearBottomLeft );
	Vector4Cross( &mFrustum.maPlanes[FRUSTUM_PLANE_BOTTOM], &v1, &v0 );
	Vector4Normalize( &mFrustum.maPlanes[FRUSTUM_PLANE_BOTTOM], &mFrustum.maPlanes[FRUSTUM_PLANE_BOTTOM] );
	mFrustum.maPlanes[FRUSTUM_PLANE_BOTTOM].fW = -Vector4Dot( &mFrustum.maPlanes[FRUSTUM_PLANE_BOTTOM], &farBottomRight );
}

/*
 **
 */
bool CCamera::inFrustum( tVector4* pPos, float fRadius )
{
	bool bInFrustum = true;
    
	// check against all the frustum plane
	for( int i = 0; i < NUM_FRUSTUM_PLANES; i++ )
	{
		tVector4* pPlane = &mFrustum.maPlanes[i];
		
		float fDist = Vector4Dot( pPlane, pPos ) + pPlane->fW;
		if( fDist < -fRadius )
		{
			bInFrustum = false;
			break;
		}
	}	// for i = 0 to num frustum planes
    
	return bInFrustum;
}

/*
 **
 */
bool CCamera::sphereInFrustum( tVector4* pPos, float fRadius ) const
{
	bool bRet = true;
	tVector4 cameraToPt;
	Vector4Subtract( &cameraToPt, pPos, &mPosition );
    
	tVector4 xDir, yDir, zDir;
	Vector4Subtract( &zDir, &mLookAt, &mPosition );
	Vector4Normalize( &zDir, &zDir );
    
	Vector4Cross( &xDir, &mUp, &zDir );
	Vector4Cross( &yDir, &xDir, &zDir );
	
	tVector4 projected;
	projected.fX = Vector4Dot( &cameraToPt, &xDir );
	projected.fY = Vector4Dot( &cameraToPt, &yDir );
	projected.fZ = Vector4Dot( &cameraToPt, &zDir );
	if( projected.fZ > mfFar + fRadius || projected.fZ < mfNear - fRadius )
	{
		bRet = false;
	}
    
	if( bRet )
	{
		float fHeight = projected.fZ * 2.0f * tan( mfFOV * 0.5f );
		float fWidth = fHeight * mfRatio;
		if( projected.fY < -fHeight * 0.5f - fRadius || projected.fY > fHeight * 0.5f + fRadius )
		{
			bRet = false;
		}
		
		if( projected.fX < -fWidth * 0.5f - fRadius || projected.fX > fWidth * 0.5f + fRadius )
		{
			bRet = false;
		}
	}
    
	return bRet;
}

/*
**
*/
bool CCamera::cubeInFrustum( tVector4 const* pPos, float fSize ) const
{
    tVector4 pt;
    int iCount = 0;
    
    for( int i = 0; i < NUM_FRUSTUM_PLANES; i++ )
    {
        iCount = 0;
        
        // bottom-left-front
        pt.fX = pPos->fX - fSize * 0.5f;
        pt.fY = pPos->fY - fSize * 0.5f;
        pt.fZ = pPos->fZ - fSize * 0.5f;
        float fDP = Vector4Dot( &mFrustum.maPlanes[i], &pt ) + mFrustum.maPlanes[i].fW;
        if( fDP > 0 )
        {
            ++iCount;
        }
        
        
        // bottom-right-front
        pt.fX = pPos->fX + fSize * 0.5f;
        pt.fY = pPos->fY - fSize * 0.5f;
        pt.fZ = pPos->fZ - fSize * 0.5f;
        fDP = Vector4Dot( &mFrustum.maPlanes[i], &pt ) + mFrustum.maPlanes[i].fW;
        if( fDP > 0 )
        {
            ++iCount;
        }
        
        // top-left-front
        pt.fX = pPos->fX - fSize * 0.5f;
        pt.fY = pPos->fY + fSize * 0.5f;
        pt.fZ = pPos->fZ - fSize * 0.5f;
        fDP = Vector4Dot( &mFrustum.maPlanes[i], &pt ) + mFrustum.maPlanes[i].fW;
        if( fDP > 0 )
        {
            ++iCount;
        }
        
        // top-right-front
        pt.fX = pPos->fX + fSize * 0.5f;
        pt.fY = pPos->fY + fSize * 0.5f;
        pt.fZ = pPos->fZ - fSize * 0.5f;
        fDP = Vector4Dot( &mFrustum.maPlanes[i], &pt ) + mFrustum.maPlanes[i].fW;
        if( fDP > 0 )
        {
            ++iCount;
        }
        
        // bottom-left-back
        pt.fX = pPos->fX + fSize * 0.5f;
        pt.fY = pPos->fY - fSize * 0.5f;
        pt.fZ = pPos->fZ + fSize * 0.5f;
        fDP = Vector4Dot( &mFrustum.maPlanes[i], &pt ) + mFrustum.maPlanes[i].fW;
        if( fDP > 0 )
        {
            ++iCount;
        }
        
        // bottom-right-back
        pt.fX = pPos->fX + fSize * 0.5f;
        pt.fY = pPos->fY - fSize * 0.5f;
        pt.fZ = pPos->fZ + fSize * 0.5f;
        fDP = Vector4Dot( &mFrustum.maPlanes[i], &pt ) + mFrustum.maPlanes[i].fW;
        if( fDP > 0 )
        {
            ++iCount;
        }
        
        // top-left-back
        pt.fX = pPos->fX - fSize * 0.5f;
        pt.fY = pPos->fY + fSize * 0.5f;
        pt.fZ = pPos->fZ + fSize * 0.5f;
        fDP = Vector4Dot( &mFrustum.maPlanes[i], &pt ) + mFrustum.maPlanes[i].fW;
        if( fDP > 0 )
        {
            ++iCount;
        }
        
        // top-right-back
        pt.fX = pPos->fX + fSize * 0.5f;
        pt.fY = pPos->fY + fSize * 0.5f;
        pt.fZ = pPos->fZ + fSize * 0.5f;
        fDP = Vector4Dot( &mFrustum.maPlanes[i], &pt ) + mFrustum.maPlanes[i].fW;
        if( fDP > 0 )
        {
            ++iCount;
        }
        
        if( iCount == 0 )
        {
            break;
        }
        
    }   // for i = 0 to num frustum planes
    
    return ( iCount > 0 );
}

/*
**
*/
bool CCamera::touchToWorldCoord( const tVector4* pNormal,
                                 float fPlaneD,
                                 float fX, 
                                 float fY,
                                 int iScreenWidth,
                                 int iScreenHeight,
                                 tVector4* pResult )
{
    float matProj[16];
    memcpy( matProj, mProjectionMatrix.afEntries, sizeof( matProj ) );
    
    float fScreenY = ( fY - (float)iScreenHeight ) / (float)iScreenHeight;
    fScreenY = -fScreenY * 2.0f - 1.0f;
    
    // convert to [-1, 1]
	tVector4 mousePt;
	mousePt.fX = ( ( ( 2.0f * fX ) / (float)iScreenWidth ) - 1.0f ) / matProj[0];
	//mousePt.fY = ( 1.0f - ( ( 2.0f * ( (float)iScreenHeight - fY ) ) / (float)iScreenHeight ) ) / matProj[5];
    //mousePt.fY = ( ( 2.0f * ( (float)iScreenHeight - fY ) ) / (float)iScreenHeight ) / matProj[5];
	mousePt.fY = fScreenY / matProj[5];
    mousePt.fZ = 1.0f; mousePt.fW = 1.0f;
    
    // transform to world by applying the inverse of the view matrix
	tMatrix44 inverse;
	Matrix44Inverse( &inverse, &mViewMatrix );
    tVector4 dir = { 0.0f, 0.0f, 0.0f, 1.0f };
	dir.fX = inverse.M( 0, 0 ) * mousePt.fX + inverse.M( 0, 1 ) * mousePt.fY + inverse.M( 0, 2 ) * -mousePt.fZ;
	dir.fY = inverse.M( 1, 0 ) * mousePt.fX + inverse.M( 1, 1 ) * mousePt.fY + inverse.M( 1, 2 ) * -mousePt.fZ;
	dir.fZ = inverse.M( 2, 0 ) * mousePt.fX + inverse.M( 2, 1 ) * mousePt.fY + inverse.M( 2, 2 ) * -mousePt.fZ;
	dir.fW = 1.0f;
	
	Vector4Normalize( &dir, &dir );
    
/*OUTPUT( "%s dir = ( %f, %f, %f )\n",
        __FUNCTION__,
        dir.fX,
        dir.fY,
        dir.fZ );
*/
    // origin at the center
    tVector4 origin = { 0.0f, 0.0f, 0.0f, 1.0f };
	origin.fX = inverse.M( 0, 3 );
	origin.fY = inverse.M( 1, 3 );
	origin.fZ = inverse.M( 2, 3 );
	origin.fW = 1.0f;
    
/*OUTPUT( "%s origin = ( %f, %f, %f )\n",
        __FUNCTION__,
        origin.fX,
        origin.fY,
        origin.fZ );
*/
	// end point of the ray, * 1000
	tVector4 endPt = { 0.0f, 0.0f, 0.0f, 1.0f };
	endPt.fX = origin.fX + dir.fX * 50.0f;
	endPt.fY = origin.fY + dir.fY * 50.0f;
	endPt.fZ = origin.fZ + dir.fZ * 50.0f;
    
    // plane: Ax + By + Cz + D = 0
    // N . p + D = 0 => D = -( N . p )
    // ray: p = p0 + t * ( p1 - p0 )
    // intersection: -N . p = -( N . ( p0 + t * ( p1 - p0 ) ) ) = D
    // t = D - ( N . p0 ) / ( N . p1 - N . p0 )
    // t = ( D - ( N . p0 ) ) / N . ( p1 - p0 )
    float fDP0 = Vector4Dot( pNormal, &origin );
    tVector4 diff = { 0.0f, 0.0f, 0.0f, 1.0f };
    Vector4Subtract( &diff, &endPt, &origin );
    float fNumerator = fPlaneD - fDP0;
    float fDenominator = Vector4Dot( pNormal, &diff );
    if( fDenominator == 0.0f )
    {
        return false;
    }
    
    float fT = fNumerator / fDenominator;
    
    tVector4 worldCoord = { 0.0f, 0.0f, 0.0f, 1.0f };
    worldCoord.fX = origin.fX + fT * diff.fX;
    worldCoord.fY = origin.fY + fT * diff.fY;
    worldCoord.fZ = origin.fZ + fT * diff.fZ;
    worldCoord.fW = 1.0f;
    
    memcpy( pResult, &worldCoord, sizeof( tVector4 ) );
    
    return true;
}