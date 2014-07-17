#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "vector.h"
#include "matrix.h"

enum
{
	FRUSTUM_PLANE_LEFT = 0,
	FRUSTUM_PLANE_RIGHT,
	FRUSTUM_PLANE_TOP,
	FRUSTUM_PLANE_BOTTOM,
	FRUSTUM_PLANE_NEAR,
	FRUSTUM_PLANE_FAR,
	
	NUM_FRUSTUM_PLANES,
};

struct Frustum
{
	tVector4	maPlanes[NUM_FRUSTUM_PLANES];
};

typedef struct Frustum tFrustum;

class CCamera
{
public:
	CCamera( void );
	~CCamera( void );
    
	void update( int iScreenWidth, int iScreenHeight );
	void setPosition( tVector4 const* pPosition );
	void setLookAt( tVector4 const* pLookAt );
	void setUp( tVector4 const* pUp );
	inline tVector4 const* getUp( void ) const { return &mUp; }

    inline void setFar( float fFar ) { mfFar = fFar; }
    inline void setNear( float fNear ) { mfNear = fNear; }
    
	inline float getFar( void ) const { return mfFar; }
	inline float getNear( void ) const { return mfNear; }

    inline void setFOV( float fFOV ) { mfFOV = fFOV; }
    inline float getFOV( void ) const { return mfFOV; }
    
	inline const tVector4* getPosition( void ) const { return &mPosition; }
	inline const tVector4* getLookAt( void ) const { return &mLookAt; }
    
    inline const tMatrix44* getViewMatrix( void ) const { return &mViewMatrix; }
	inline const tMatrix44* getProjectionMatrix( void ) const { return &mProjectionMatrix; }
    inline const tMatrix44* getViewProjMatrix( void ) const { return &mViewProjMatrix; }
    
    inline const tMatrix44* getInvRotMatrix( void ) const { return &mInvRotMatrix; }
    
	bool inFrustum( tVector4* pPos, float fRadius );
	bool sphereInFrustum( tVector4* pPos, float fRadius ) const;
    bool cubeInFrustum( tVector4 const* pPos, float fSize ) const;
    
	void calcOrthographicProjection( int iScreenWidth, int iScreenHeight );

    bool touchToWorldCoord( const tVector4* pNormal,
                            float fPlaneD,
                            float fX, 
                            float fY,
                            int iScreenWidth,
                            int iScreenHeight,
                            tVector4* pResult );
    
	void makeOrthoFrustum( tVector4* pTopPlane,
						   tVector4* pBottomPlane,
						   tVector4* pLeftPlane,
						   tVector4* pRightPlane,
						   tVector4* pNearPlane,
						   tVector4* pFarPlane,
						   float fWidth,
						   float fHeight );

	void setFrustum( tVector4 const* pTopPlane,
				     tVector4 const* pBottomPlane,
				     tVector4 const* pLeftPlane,
				     tVector4 const* pRightPlane,
				     tVector4 const* pNearPlane,
				     tVector4 const* pFarPlane );

	inline const tMatrix44* getOrthographicMatrix( void ) const { return &mOrthographicMatrix; }
	
	inline const tVector4* getLookDir( void ) const { return &mLookDir; }

	tVector4	mFarCenter;
	tVector4	mNearCenter;
    
	tVector4	mFarTopLeft;
	tVector4	mFarTopRight;
	tVector4	mFarBottomLeft;
	tVector4	mFarBottomRight;
    
	tVector4	mNearTopLeft;
	tVector4	mNearTopRight;
	tVector4	mNearBottomLeft;
	tVector4	mNearBottomRight;
    
public:
	static CCamera* instance( void );
    
protected:
	static CCamera* mpInstance;
    
protected:
	tMatrix44	mViewMatrix;
	tMatrix44	mInvRotMatrix;
	
	tMatrix44	mProjectionMatrix;
	tMatrix44	mViewProjMatrix;
    
	tMatrix44	mOrthographicMatrix;

	tVector4	mLookAt;
	tVector4	mPosition;
	tVector4	mUp;
	tVector4	mLookDir;
	

	tFrustum	mFrustum;
	float		mfFOV;
	float		mfFar;
	float		mfNear;
    
	float		mfRatio;
    
protected:
	void calcFrustum( int iScreenWidth, int iScreenHeight );
    
};

#endif // __CAMERA_H__