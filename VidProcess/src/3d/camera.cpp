#include "camera.h"
#include "render.h"

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
	
	Vector4Subtract( &mLookDir, &mLookAt, &mPosition );
	Vector4Normalize( &mLookDir, &mLookDir );
	mLookDir.fW = 1.0f;

	// update view matrix
	Matrix44LookAt( &mViewMatrix,
                   &mPosition,
                   &mLookAt,
                   &mUp,
                   &mInvRotMatrix,
                   false );
	 
	Matrix44Perspective( &mProjectionMatrix,
                        mfFOV,
                        iScreenWidth,
                        iScreenHeight,
                        mfFar,
                        mfNear );
	
	tMatrix44 transpose;
	Matrix44Transpose( &transpose, &mViewMatrix );
	Matrix44Multiply( &mViewProjMatrix, &transpose, &mProjectionMatrix );
    
	float fHalfScreenWidth = (float)iScreenWidth * 0.5f;
	float fHalfScreenHeight = (float)iScreenHeight * 0.5f;

	Matrix44Orthographic( &mOrthographicMatrix,
						  -fHalfScreenWidth,
						  fHalfScreenWidth,
						  -fHalfScreenHeight,
						  fHalfScreenHeight,
						  mfNear,
						  mfFar );

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
void CCamera::setUp( tVector4 const* pUp )
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
	Vector4Normalize( &right, &right );

	Vector4Cross( &up, &lookDir, &right );
	up.fW = 1.0f;
	Vector4Normalize( &up, &up );
	
    // far frustum corners
	farCenter.fX = mPosition.fX + lookDir.fX * mfFar;
	farCenter.fY = mPosition.fY + lookDir.fY * mfFar;
	farCenter.fZ = mPosition.fZ + lookDir.fZ * mfFar;
	farCenter.fW = 1.0f;
	
	farTopLeft.fX = farCenter.fX + ( up.fX * fFarHeight * 0.5f ) - ( right.fX * fFarWidth * 0.5f );
	farTopLeft.fY = farCenter.fY + ( up.fY * fFarHeight * 0.5f ) - ( right.fY * fFarWidth * 0.5f );
	farTopLeft.fZ = farCenter.fZ + ( up.fZ * fFarHeight * 0.5f ) - ( right.fZ * fFarWidth * 0.5f);
	farTopLeft.fW = 1.0f;
	
	farTopRight.fX = farCenter.fX + ( up.fX * fFarHeight * 0.5f ) + ( right.fX * fFarWidth * 0.5f );
	farTopRight.fY = farCenter.fY + ( up.fY * fFarHeight * 0.5f ) + ( right.fY * fFarWidth * 0.5f );
	farTopRight.fZ = farCenter.fZ + ( up.fZ * fFarHeight * 0.5f ) + ( right.fZ * fFarWidth * 0.5f );
	farTopRight.fW = 1.0f;
	
	farBottomLeft.fX = farCenter.fX - ( up.fX * fFarHeight * 0.5f ) - ( right.fX * fFarWidth * 0.5f );
	farBottomLeft.fY = farCenter.fY - ( up.fY * fFarHeight * 0.5f ) - ( right.fY * fFarWidth * 0.5f );
	farBottomLeft.fZ = farCenter.fZ - ( up.fZ * fFarHeight * 0.5f ) - ( right.fZ * fFarWidth * 0.5f );
	farBottomLeft.fW = 1.0f;
	
	farBottomRight.fX = farCenter.fX - ( up.fX * fFarHeight * 0.5f ) + ( right.fX * fFarWidth * 0.5f );
	farBottomRight.fY = farCenter.fY - ( up.fY * fFarHeight * 0.5f ) + ( right.fY * fFarWidth * 0.5f );
	farBottomRight.fZ = farCenter.fZ - ( up.fZ * fFarHeight * 0.5f ) + ( right.fZ * fFarWidth * 0.5f );
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
    
	Vector4Cross( &xDir, &zDir, &mUp );
	Vector4Normalize( &xDir, &xDir );
    Vector4Cross( &yDir, &xDir, &zDir );
	Vector4Normalize( &yDir, &yDir );
    
	tVector4 projected = { 0.0f, 0.0f, 0.0f, 1.0f };
	projected.fZ = Vector4Dot( &cameraToPt, &zDir );
	if( projected.fZ > mfFar + fRadius || projected.fZ < mfNear - fRadius )
	{
		bRet = false;
	}
    
	if( bRet )
	{
		projected.fX = Vector4Dot( &cameraToPt, &xDir );
		projected.fY = Vector4Dot( &cameraToPt, &yDir );

		float fRatio = (float)renderGetScreenHeight() / (float)renderGetScreenWidth();

		float fWidth = projected.fZ * 2.0f * tanf( mfFOV * 0.5f );
		float fHeight = fWidth * fRatio;

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
    //tVector4 pt;
    int iCount = 0;
    
	tVector4 const* pPlane = mFrustum.maPlanes;
	float fW = pPlane->fW;
	float fHalfSize = fSize * 0.5f;
	
	float fLeft = pPos->fX - fHalfSize;
	float fRight = pPos->fX + fHalfSize;
	float fTop = pPos->fY + fHalfSize;
	float fBottom = pPos->fY - fHalfSize;
	float fFront = pPos->fZ - fHalfSize;
	float fBack = pPos->fZ + fHalfSize;

    for( int i = 0; i < NUM_FRUSTUM_PLANES; i++ )
    {
		pPlane = &mFrustum.maPlanes[i];
		fW = pPlane->fW;

        iCount = 0;
        
        // bottom-left-front
        //pt.fX = fLeft;
        //pt.fY = fBottom;
        //pt.fZ = fFront;
        //float fDP = Vector4Dot( pPlane, &pt ) + fW;
        float fDP = fLeft * pPlane->fX + fBottom * pPlane->fY + fFront * pPlane->fZ + fW;
		if( fDP > 0 )
        {
            ++iCount;
			continue;
        }
        
        
        // bottom-right-front
        //pt.fX = fRight;
        //pt.fY = fBottom;
        //pt.fZ = fFront;
        //fDP = Vector4Dot( pPlane, &pt ) + fW;
		fDP = fRight * pPlane->fX + fBottom * pPlane->fY + fFront * pPlane->fZ + fW;
        if( fDP > 0 )
        {
            ++iCount;
			continue;
        }
        
        // top-left-front
        //pt.fX = fLeft;
        //pt.fY = fTop;
        //pt.fZ = fFront;
        //fDP = Vector4Dot( pPlane, &pt ) + fW;
        fDP = fLeft * pPlane->fX + fTop * pPlane->fY + fFront * pPlane->fZ + fW;
		if( fDP > 0 )
        {
            ++iCount;
			continue;
        }
        
        // top-right-front
        //pt.fX = fRight;
        //pt.fY = fTop;
        //pt.fZ = fFront;
        //fDP = Vector4Dot( pPlane, &pt ) + fW;
        fDP = fRight * pPlane->fX + fTop * pPlane->fY + fFront * pPlane->fZ + fW;
		if( fDP > 0 )
        {
            ++iCount;
			continue;
        }
        
        // bottom-left-back
        //pt.fX = fLeft;
        //pt.fY = fBottom;
        //pt.fZ = fBack;
        //fDP = Vector4Dot( pPlane, &pt ) + fW;
        fDP = fLeft * pPlane->fX + fBottom * pPlane->fY + fBack * pPlane->fZ + fW;
		if( fDP > 0 )
        {
            ++iCount;
			continue;
        }
        
        // bottom-right-back
        //pt.fX = fRight;
        //pt.fY = fBottom;
        //pt.fZ = fBack;
        //fDP = Vector4Dot( pPlane, &pt ) + fW;
        fDP = fRight * pPlane->fX + fBottom * pPlane->fY + fBack * pPlane->fZ + fW;
		if( fDP > 0 )
        {
            ++iCount;
			continue;
        }
        
        // top-left-back
        //pt.fX = fLeft;
        //pt.fY = fTop;
        //pt.fZ = fBack;
        //fDP = Vector4Dot( pPlane, &pt ) + fW;
        fDP = fLeft * pPlane->fX + fTop * pPlane->fY + fBack * pPlane->fZ + fW;
		if( fDP > 0 )
        {
            ++iCount;
			continue;
        }
        
        // top-right-back
        //pt.fX = fRight;
        //pt.fY = fTop;
        //pt.fZ = fBack;
        //fDP = Vector4Dot( pPlane, &pt ) + fW;
        fDP = fRight * pPlane->fX + fTop * pPlane->fY + fBack * pPlane->fZ + fW;
		if( fDP > 0 )
        {
            ++iCount;
			continue;
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
	endPt.fX = origin.fX + dir.fX * 500.0f;
	endPt.fY = origin.fY + dir.fY * 500.0f;
	endPt.fZ = origin.fZ + dir.fZ * 500.0f;
    
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

/*
**
*/
void CCamera::makeOrthoFrustum( tVector4* pTopPlane,
							    tVector4* pBottomPlane,
							    tVector4* pLeftPlane,
							    tVector4* pRightPlane,
								tVector4* pNearPlane,
							    tVector4* pFarPlane,
							    float fWidth,
							    float fHeight )
{
	float fHalfWidth = fWidth * 0.5f;
	float fHalfHeight = fHeight * 0.5f;

	tVector4 lookDir = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4Subtract( &lookDir, &mLookAt, &mPosition );
	Vector4Normalize( &lookDir, &lookDir );
	
	if( fabs( lookDir.fY ) > fabs( lookDir.fX ) &&
		fabs( lookDir.fY ) > fabs( lookDir.fZ ) )
	{
		mUp.fX = 0.0f;
		mUp.fY = 0.0f;
		mUp.fZ = 1.0f;
	}

	tVector4 side = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4Cross( &side, &mUp, &lookDir );
	Vector4Normalize( &side, &side );

	tVector4 vertical = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4Cross( &vertical, &lookDir, &side );
	Vector4Normalize( &vertical, &vertical );

	// side-width
	tVector4 sideWidth = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4MultScalar( &sideWidth, &side, fHalfWidth );

	// vertical-height
	tVector4 verticalHeight = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4MultScalar( &verticalHeight, &vertical, fHalfHeight );

	// depth-far
	tVector4 depthFar = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4MultScalar( &depthFar, &lookDir, mfFar );

	// depth-near
	tVector4 depthNear = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4MultScalar( &depthNear, &lookDir, mfNear );

	// front
	tVector4 topLeftFront = 
	{ 
		mPosition.fX - sideWidth.fX + verticalHeight.fX + depthNear.fX,
		mPosition.fY - sideWidth.fY + verticalHeight.fY + depthNear.fY,
		mPosition.fZ - sideWidth.fZ + verticalHeight.fZ + depthNear.fZ,
		1.0f
	};
	

	tVector4 topRightFront = 
	{
		mPosition.fX + sideWidth.fX + verticalHeight.fX + depthNear.fX,
		mPosition.fY + sideWidth.fY + verticalHeight.fY + depthNear.fY,
		mPosition.fZ + sideWidth.fZ + verticalHeight.fZ + depthNear.fZ,
		1.0f
	};

	tVector4 bottomLeftFront = 
	{
		mPosition.fX - sideWidth.fX - verticalHeight.fX + depthNear.fX,
		mPosition.fY - sideWidth.fY - verticalHeight.fY + depthNear.fY,
		mPosition.fZ - sideWidth.fZ - verticalHeight.fZ + depthNear.fZ,
		1.0f
	};

	tVector4 bottomRightFront = 
	{
		mPosition.fX + sideWidth.fX - verticalHeight.fX + depthNear.fX,
		mPosition.fY + sideWidth.fY - verticalHeight.fY + depthNear.fY,
		mPosition.fZ + sideWidth.fZ - verticalHeight.fZ + depthNear.fZ,
		1.0f
	};

	// back
	tVector4 topLeftBack = 
	{
		mPosition.fX - sideWidth.fX + verticalHeight.fX + depthFar.fX,
		mPosition.fY - sideWidth.fY + verticalHeight.fY + depthFar.fY,
		mPosition.fZ - sideWidth.fZ + verticalHeight.fZ + depthFar.fZ,
		1.0f
	};

	tVector4 topRightBack = 
	{
		mPosition.fX + sideWidth.fX + verticalHeight.fX + depthFar.fX,
		mPosition.fY + sideWidth.fY + verticalHeight.fY + depthFar.fY,
		mPosition.fZ + sideWidth.fZ + verticalHeight.fZ + depthFar.fZ,
		1.0f
	};

	tVector4 bottomLeftBack = 
	{
		mPosition.fX - sideWidth.fX - verticalHeight.fX + depthFar.fX,
		mPosition.fY - sideWidth.fY - verticalHeight.fY + depthFar.fY,
		mPosition.fZ - sideWidth.fZ - verticalHeight.fZ + depthFar.fZ,
		1.0f
	};

	tVector4 bottomRightBack = 
	{
		mPosition.fX + sideWidth.fX - verticalHeight.fX + depthFar.fX,
		mPosition.fY + sideWidth.fY - verticalHeight.fY + depthFar.fY,
		mPosition.fZ + sideWidth.fZ - verticalHeight.fZ + depthFar.fZ,
		1.0f
	};

	tVector4 diff0 = { 0.0f, 0.0f, 0.0f, 1.0f };
	tVector4 diff1 = { 0.0f, 0.0f, 0.0f, 1.0f };

	// near
	Vector4Subtract( &diff0, &topRightFront, &topLeftFront );
	Vector4Subtract( &diff1, &bottomLeftFront, &topLeftFront );
	Vector4Normalize( &diff0, &diff0 );
	Vector4Normalize( &diff1, &diff1 );
	Vector4Cross( pNearPlane, &diff1, &diff0 );
	pNearPlane->fW = -Vector4Dot( pNearPlane, &topLeftFront );

	// far
	Vector4Subtract( &diff0, &topLeftBack, &topRightBack );
	Vector4Subtract( &diff1, &bottomRightBack, &topRightBack );
	Vector4Normalize( &diff0, &diff0 );
	Vector4Normalize( &diff1, &diff1 );
	Vector4Cross( pFarPlane, &diff1, &diff0 );
	pFarPlane->fW = -Vector4Dot( pFarPlane, &topLeftBack );

	// left
	Vector4Subtract( &diff0, &topLeftFront, &topLeftBack );
	Vector4Subtract( &diff1, &bottomLeftBack, &topLeftBack );
	Vector4Normalize( &diff0, &diff0 );
	Vector4Normalize( &diff1, &diff1 );
	Vector4Cross( pLeftPlane, &diff1, &diff0 );
	pLeftPlane->fW = -Vector4Dot( pLeftPlane, &topLeftBack );

	// right
	Vector4Subtract( &diff0, &topRightFront, &topRightBack );
	Vector4Subtract( &diff1, &bottomRightBack, &topRightBack );
	Vector4Normalize( &diff0, &diff0 );
	Vector4Normalize( &diff1, &diff1 );
	Vector4Cross( pRightPlane, &diff0, &diff1 );
	pRightPlane->fW = -Vector4Dot( pRightPlane, &topRightBack );

	// top
	Vector4Subtract( &diff0, &topLeftBack, &topLeftFront );
	Vector4Subtract( &diff1, &topLeftFront, &topRightFront );
	Vector4Normalize( &diff0, &diff0 );
	Vector4Normalize( &diff1, &diff1 );
	Vector4Cross( pTopPlane, &diff0, &diff1 );
	pTopPlane->fW = -Vector4Dot( pTopPlane, &topLeftFront );

	// bottom
	Vector4Subtract( &diff0, &bottomLeftBack, &bottomLeftFront );
	Vector4Subtract( &diff1, &bottomLeftFront, &bottomRightFront );
	Vector4Normalize( &diff0, &diff0 );
	Vector4Normalize( &diff1, &diff1 );
	Vector4Cross( pBottomPlane, &diff1, &diff0 );
	pBottomPlane->fW = -Vector4Dot( pBottomPlane, &bottomLeftFront );
}

/*
**
*/
void CCamera::setFrustum( tVector4 const* pTopPlane,
						  tVector4 const* pBottomPlane,
						  tVector4 const* pLeftPlane,
						  tVector4 const* pRightPlane,
						  tVector4 const* pNearPlane,
						  tVector4 const* pFarPlane )
{
	memcpy( &mFrustum.maPlanes[FRUSTUM_PLANE_TOP], pTopPlane, sizeof( tVector4 ) );
	memcpy( &mFrustum.maPlanes[FRUSTUM_PLANE_BOTTOM], pBottomPlane, sizeof( tVector4 ) );
	memcpy( &mFrustum.maPlanes[FRUSTUM_PLANE_LEFT], pLeftPlane, sizeof( tVector4 ) );
	memcpy( &mFrustum.maPlanes[FRUSTUM_PLANE_RIGHT], pRightPlane, sizeof( tVector4 ) );
	memcpy( &mFrustum.maPlanes[FRUSTUM_PLANE_NEAR], pNearPlane, sizeof( tVector4 ) );
	memcpy( &mFrustum.maPlanes[FRUSTUM_PLANE_FAR], pFarPlane, sizeof( tVector4 ) );
}