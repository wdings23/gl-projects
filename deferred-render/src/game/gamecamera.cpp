#include "gamecamera.h"
#include "camera.h"
#include "quaternion.h"
#include "render.h"

struct CameraInfo 
{
	tVector4	mLookAngle;
	tVector4	mPos;
};

typedef struct CameraInfo tCameraInfo;

tCameraInfo sCameraInfo = 
{
	{ 0.0f, 0.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f },
};


/*
**
*/
void gameCameraInit( tVector4 const* pPos, tVector4 const* pLookAt )
{
	memcpy( &sCameraInfo.mPos, pPos, sizeof( tVector4 ) );
}

/*
**
*/
void gameCameraUpdate( float fDLookX, float fDLookY, float fDPosX, float fDPosY, float fDPosZ )
{
	tQuaternion qLookX = { 0.0f, 0.0f, 0.0f, 1.0f };
	tQuaternion qLookY = { 0.0f, 0.0f, 0.0f, 1.0f };
	
	tVector4 xAxis = { 1.0f, 0.0f, 0.0f, 1.0f };
	tVector4 yAxis = { 0.0f, 1.0f, 0.0f, 1.0f };
	
	// change of look
	sCameraInfo.mLookAngle.fY += fDLookX * 0.01f;
	sCameraInfo.mLookAngle.fX += fDLookY * 0.01f;
	
	// quaternion in x and y axis
	quaternionFromAxisAngle( &qLookX, &xAxis, sCameraInfo.mLookAngle.fY );
	quaternionFromAxisAngle( &qLookY, &yAxis, -sCameraInfo.mLookAngle.fX );
	
	// matrix from quaternion
	tMatrix44 rotMatrix;
	tQuaternion qTotal;
	Matrix44Identity( &rotMatrix );
	quaternionMultiply( &qTotal, &qLookX, &qLookY );
	quaternionToMatrix( &rotMatrix, &qTotal );

	// apply to camera lookat
	tVector4 initialLookAt = { 1.0f, 0.0f, 0.0f, 1.0f };
	tVector4 result = { 0.0f, 0.0f, 0.0f, 1.0f };
	Matrix44Transform( &result, &initialLookAt, &rotMatrix );
	
	tVector4 lookAt = { 0.0f, 0.0f, 0.0f, 1.0f };
	tVector4 newLookAt = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4MultScalar( &lookAt, &result, 100.0f );
	Vector4Add( &newLookAt, &lookAt, &sCameraInfo.mPos );

	sCameraInfo.mPos.fX += fDPosX;
	sCameraInfo.mPos.fY += fDPosY;

	CCamera* pCamera = CCamera::instance();
	pCamera->setPosition( &sCameraInfo.mPos );
	pCamera->setLookAt( &newLookAt );
	pCamera->update( renderGetScreenWidth(), renderGetScreenHeight() );

	OUTPUT( "fDX = %.2f fDY = %.2f\nnewLookAt = ( %.2f, %.2f, %.2f )\n", 
			fDLookX, fDLookY,
			newLookAt.fX, newLookAt.fY, newLookAt.fZ );
}