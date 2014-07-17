#ifndef __COLLISION_H__
#define __COLLISION_H__

tVector4 getSlidingPosition( tVector4 const* pPos, 
						     tVector4 const* pPlane, 
							 tVector4 const* pVelocity,
							 tVector4 const* pVertexPos,
							 float fSpeed );

bool checkCollision( tVector4* pCollisionPt,
					 tVector4 const* aFacePos,
				     tVector4 const* pSpherePos,
					 tVector4 const* pVelocity );

bool spherePlaneCollision( tVector4 const* pPlane,
						   tVector4 const* pSpherePos,
						   float fRadius,
						   float* pfDistance );

bool sphereTriangleCollision( tVector4 const* aPos,
							  tVector4 const* pSpherePos,
							  float fRadius,
							  tVector4* pContactPt );

void computePlane( tVector4* pPlane, tVector4 const* aPos );

#endif // __COLLISION_H__