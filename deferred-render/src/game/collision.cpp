#include "vector.h"
#include "modelinstance.h"

/*
**
*/
static bool isPointInTriangle( tVector4 const* pPt, 
							   tVector4 const* aTriPos )
{
	float fU = 0.0f;
	float fV = 0.0f;
	calcBarycentric3( pPt, &aTriPos[0], &aTriPos[1], &aTriPos[2], &fU, &fV ); 

	return ( fU >= 0.0f ) && ( fV >= 0.0f ) && ( fU + fV <= 1.0f );
}

/*
**
*/
bool getLowestRoot( float* pfRoot, 
				    float fA,
					float fB,
					float fC,
					float fMaxR )
{	
	// Quadratic equation: b +/- sqrt( b^2 - 4ac ) / 2a
	
	bool bRet = false;

	float fDeterminant = fB * fB - 4.0f * fA * fC;
	if( fDeterminant >= 0.0 )
	{
		float fSqrtD = sqrtf( fDeterminant );
		float fRoot0 = ( -fB + fSqrtD ) / ( 2.0f * fA );
		float fRoot1 = ( -fB - fSqrtD ) / ( 2.0f * fA );

		// sort
		if( fRoot0 > fRoot1 )
		{
			float fTemp = fRoot0;
			fRoot0 = fRoot1;
			fRoot1 = fTemp;
		}

		if( fRoot0 > 0.0f && fRoot0 < fMaxR )
		{
			*pfRoot = fRoot0;
			bRet = true;
		}

		if( !bRet )
		{
			if( fRoot1 > 0.0f && fRoot1 < fMaxR )
			{
				*pfRoot = fRoot1;
				bRet = true;
			}
		}
	}

	return bRet;
}

/*
**
*/
void computePlane( tVector4* pPlane, tVector4 const* aPos )
{
	tVector4 v0, v1;
	Vector4Subtract( &v0, &aPos[1], &aPos[0] );
	Vector4Subtract( &v1, &aPos[2], &aPos[0] );
	Vector4Cross( pPlane, &v1, &v0 );
	Vector4Normalize( pPlane, pPlane );

	pPlane->fW = -Vector4Dot( pPlane, &aPos[0] );
}

/*
**
*/
tVector4 getSlidingPosition( tVector4 const* pPos, 
						     tVector4 const* pPlane, 
							 tVector4 const* pVelocity,
							 tVector4 const* pVertexPos,
							 float fSpeed )
{
	tVector4 velocityWithSpeed = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4MultScalar( &velocityWithSpeed, pVelocity, fSpeed );

	// position into the plane
	tVector4 posIntoPlane = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4Add( &posIntoPlane, pPos, &velocityWithSpeed );

	// shoot ray toward the plane along plane normal
	tVector4 planeSpeedVelocity = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4MultScalar( &planeSpeedVelocity, pPlane, fSpeed );

	tVector4 posOutOfPlane = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4MultScalar( &posOutOfPlane, pPlane, 10.0f );
	Vector4Add( &posOutOfPlane, &posOutOfPlane, &posIntoPlane );

	//tVector4 negPlane = { 0.0f, 0.0f, 0.0f, -pPlane->fW };
	//Vector4MultScalar( &negPlane, pPlane, -1.0f );

	// collision point to the plane normal
	// plane: p . N + D = 0
	// ray: p = p0 + t * ( p1 - p0 )
	// ( p0 + t * ( p1 - p0 ) ) . N + D = 0
	// t * ( p1 - p0 ) . N = -( p0 . N + D )
	// t = -( p0 . N + D ) / ( p1 - p0 ) . N
	//float fNumer = -( Vector4Dot( &posIntoPlane, &negPlane ) + negPlane.fW );
	//float fDenom = Vector4Dot( &negPlane, pPlane );
	float fNumer = -( Vector4Dot( &posIntoPlane, pPlane ) + pPlane->fW );
	tVector4 outOfPlaneV = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4Subtract( &outOfPlaneV, &posOutOfPlane, &posIntoPlane );
	float fDenom = Vector4Dot( &outOfPlaneV, pPlane );

	float fT = -1.0f;
	if( fabs( fDenom ) > 0.0f )
	{
		fT = fNumer / fDenom;
	}

	tVector4 collisionPt = { 0.0f, 0.0f, 0.0f, 1.0f };
	tVector4 scaledVelocity = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vector4MultScalar( &scaledVelocity, &outOfPlaneV, fT );
	Vector4Add( &collisionPt, &scaledVelocity, &posIntoPlane );

	return collisionPt;
}

/*
**
*/
bool checkCollision( tVector4* pCollisionPt,
					 tVector4 const* aFacePos,
				     tVector4 const* pSpherePos,
					 tVector4 const* pVelocity )
{
	// test test test
	/*tVector4 aPos[] = 
	{
		{ -10.0f, 10.0f, 0.0f, 1.0f },
		{ -10.0f, -10.0f, 0.0f, 1.0f },
		{ 10.0f, 10.0f, 0.0f, 1.0f }
	};
	
	tVector4 aPos[] = 
	{
		{ -10.0f, 10.0f, 20.0f, 1.0f },
		{ -10.0f, -10.0f, 0.0f, 1.0f },
		{ 10.0f, 10.0f, 20.0f, 1.0f }
	};*/

	// plane of the triangle
	tVector4 plane;
	computePlane( &plane, aFacePos );

	float fPlaneDotVelocity = Vector4Dot( &plane, pVelocity );
	float fSphereSignedDistToPlane = Vector4Dot( pSpherePos, &plane ) + plane.fW;

	// interval of plane intersection
	float fT0 = 2.0f, fT1 = 2.0f;
	
	if( fabs( fPlaneDotVelocity ) > 0.0f )
	{
		fT0 = ( -1.0f - fSphereSignedDistToPlane ) / fPlaneDotVelocity;
		fT1 = ( 1.0f - fSphereSignedDistToPlane ) / fPlaneDotVelocity;
	}

	// sort
	if( fT0 > fT1 )
	{
		float fTemp = fT1;
		fT0 = fTemp;
		fT1 = fT0;
	}
	
	// check if there is collision with the shortest point of sphere from plane
	bool bCollided = false;
	if( ( fT0 >= 0.0f && fT0 <= 1.0f ) || ( fT1 >= 0.0f && fT1 <= 1.0f ) )
	{
		// shortest point
		tVector4 shortestPoint = { 0.0f, 0.0f, 0.0f, 1.0f };
		Vector4Subtract( &shortestPoint, pSpherePos, &plane );
		
		// see if the point is in the triangle
		if( isPointInTriangle( &shortestPoint, aFacePos ) )
		{
			bCollided = true;
			memcpy( pCollisionPt, &shortestPoint, sizeof( tVector4 ) );
		}
		
		// didn't collide with the triangle, check vertices and edges
		if( !bCollided )
		{
			// shortest multiplier 
			float fT = 2.0f;

			float fVelocityLength = Vector4Magnitude( pVelocity );
			float fVelocitySquareLength = fVelocityLength * fVelocityLength;

			// check vertices on the triangle
			tVector4 ptToSphere = { 0.0f, 0.0f, 0.0f, 1.0f };
			tVector4 sphereToPt = { 0.0f, 0.0f, 0.0f, 1.0f };
			for( int i = 0; i < 3; i++ )
			{
				float fA = fVelocitySquareLength;
				
				Vector4Subtract( &ptToSphere, &aFacePos[i], pSpherePos );
				float fB = Vector4Dot( pVelocity, &ptToSphere ) * 2.0f;
				
				Vector4MultScalar( &sphereToPt, &ptToSphere, -1.0f );
				float fPtToSphereLength = Vector4Magnitude( &sphereToPt );
				float fC = fPtToSphereLength * fPtToSphereLength - 1.0f;
				
				// solve root for the intersection quadractic
				float fNewT = -1.0f;
				if( getLowestRoot( &fNewT, fA, fB, fC, fT ) )
				{
					// collided with this vertex
					fT = fNewT;
					bCollided = true;
					memcpy( pCollisionPt, &aFacePos[i], sizeof( tVector4 ) );
				}

			} // for i = 0 to 3	

			// check edges on the triangle
			tVector4 spherePtToVertex = { 0.0f, 0.0f, 0.0f, 1.0f };
			for( int i = 0; i < 3; i++ )
			{
				tVector4 const* pVertPos = &aFacePos[i];

				// vertex indices for edge
				int iIndex0 = i;
				int iIndex1 = ( i + 1 ) % 3;
				
				// edge
				tVector4 edge = { 0.0f, 0.0f, 0.0f, 1.0f };
				Vector4Subtract( &edge, &aFacePos[iIndex1], &aFacePos[iIndex0] );
				float fEdgeLength = Vector4Magnitude( &edge );
				float fSquareEdgeLength = fEdgeLength * fEdgeLength;
				
				// center to vertex and length
				Vector4Subtract( &spherePtToVertex, pVertPos, pSpherePos ); 
				float fSpherePtToVertexLength = Vector4Magnitude( &spherePtToVertex );
				float fSquareSpherePtToVertexLength = fSpherePtToVertexLength * fSpherePtToVertexLength;
				
				// edge . velocity, edge . center to vertex
				float fEdgeDotVelocity = Vector4Dot( &edge, pVelocity );
				float fEdgeDotSpherePtToVertex = Vector4Dot( &edge, &spherePtToVertex );
			
				// entries for the equation
				float fA = fSquareEdgeLength * -fVelocitySquareLength +
						   fEdgeDotVelocity * fEdgeDotVelocity;
				
				float fB = fSquareEdgeLength * ( 2.0f * Vector4Dot( pVelocity, &spherePtToVertex ) ) - 
						   2.0f * fEdgeDotVelocity * fEdgeDotSpherePtToVertex;

				float fC = fSquareEdgeLength * ( 1.0f - fSquareSpherePtToVertexLength ) + 
						   fEdgeDotSpherePtToVertex * fEdgeDotSpherePtToVertex;
				
				// solve for root
				float fNewT = -1.0f;
				if( getLowestRoot( &fNewT, fA, fB, fC, fT ) )
				{
					float f = ( fEdgeDotVelocity * fNewT - fEdgeDotSpherePtToVertex ) / fSquareEdgeLength;
					if( f >= 0.0f && f <= 1.0f )
					{
						// collided with this edge
						fT = fNewT;
						bCollided = true;

						pCollisionPt->fX = pVertPos->fX + f * edge.fX;
						pCollisionPt->fY = pVertPos->fY + f * edge.fY;
						pCollisionPt->fZ = pVertPos->fZ + f * edge.fZ;
					}
				}
							
			}	// for i = 0 to 3
		}	// if not inside triangle
	}	// if collision parameter is in [0, 1]
	
	if( bCollided )
	{
		// get the slide velocity
		tVector4 slidePos = getSlidingPosition( pCollisionPt, &plane, pVelocity, &aFacePos[0], 1.0f );
		tVector4 slideVelocity = { 0.0f, 0.0f, 0.0f, 1.0f };
		Vector4Subtract( &slideVelocity, &slidePos, pCollisionPt );
		Vector4Normalize( &slideVelocity, &slideVelocity );

		tVector4 newPos = { 0.0f, 0.0f, 0.0f, 1.0f };
		tVector4 slideVelocitySpeed = { 0.0f, 0.0f, 0.0f, 1.0f };
		Vector4MultScalar( &slideVelocitySpeed, &slideVelocity, 1.0f );
		Vector4Add( &newPos, pSpherePos, &slideVelocitySpeed );
	}

	return bCollided;
}	

/*
**
*/
bool spherePlaneCollision( tVector4 const* pPlane,
						   tVector4 const* pSpherePos,
						   float fRadius,
						   float* pfDistance )
{
	float fDistance = Vector4Dot( pPlane, pSpherePos ) + pPlane->fW;
	if( pfDistance )
	{
		*pfDistance = fabs( fDistance );
	}
	return( fabs( fDistance ) < fRadius );
}

/*
**
*/
bool sphereTriangleCollision( tVector4 const* aPos,
							  tVector4 const* pSpherePos,
							  float fRadius,
							  tVector4* pContactPt )
{
	float fDistance = 2.0f;
	tVector4 plane = { 0.0f, 0.0f, 0.0f, 1.0f };
	computePlane( &plane, aPos );
	
	bool bSpherePlaneCollision = spherePlaneCollision( &plane,
													   pSpherePos,
													   fRadius,
													   &fDistance );
	
	bool bCollided = false;
	if( bSpherePlaneCollision )
	{
		float fDiff = 1.0f - fDistance;
		tVector4 contactPt = 
		{
			pSpherePos->fX + plane.fX * fDiff,
			pSpherePos->fY + plane.fY * fDiff,
			pSpherePos->fZ + plane.fZ * fDiff,
			1.0f
		};

		float fU = 2.0f, fV = 2.0f;
		calcBarycentric3( &contactPt, &aPos[0], &aPos[1], &aPos[2], &fU, &fV );
		if( fU >= 0.0f && fV >= 0.0f && fU + fV <= 1.0f )
		{
			memcpy( pContactPt, &contactPt, sizeof( tVector4 ) );
			bCollided = true;
		}
	}
	
	return bCollided;
}