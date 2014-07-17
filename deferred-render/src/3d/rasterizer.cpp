#include "camera.h"
#include "matrix.h"
#include "model.h"
#include "rasterizer.h"
#include "lodepng.h"
#include "timeutil.h"

#define Z_CLAMP 0.1f
#define DEFAULT_ONE_OVER_Z 1.0f / 0.1f

#if defined( PROFILE )
// profile
enum
{
    PROFILE_RASTER_CLIP = 0,
    PROFILE_RASTER_CHECK_Z,
    PROFILE_RASTER_TRI,
    
    NUM_PROFILE_TIMES,
};

static double safProfileTimes[NUM_PROFILE_TIMES];

void clearProfileTimes( void )
{
    memset( safProfileTimes, 0, sizeof( safProfileTimes ) );
}

#endif // PROFILE

static void rasterizeQuad( float* afDepthBuffer,
                           unsigned char* acBuffer, 
						   int iBufferWidth, 
						   int iBufferHeight,
						   tVector2i* aScreenCoord,
                           float* afZCoord,
                           tVector2* apUV = NULL,
                           unsigned char const* acTexture = NULL,
                           int iTextureWidth = 0,
                           int iTextureHeight = 0 );

static void rasterizeFlatBottomTri( float* afDepthBuffer,
                                    unsigned char* acBuffer, 
                                    int iBufferWidth, 
                                    int iBufferHeight,
                                    tVector2i* pScreenCoord0,
                                    tVector2i* pScreenCoord1,
                                    tVector2i* pScreenCoord2,
                                    float fZCoord0,
                                    float fZCoord1,
                                    float fZCoord2,
                                    tVector2 const* aUV = NULL,
                                    unsigned char const* acTexture = NULL,
                                    int iTextureWidth = 0,
                                    int iTextureHeight = 0 );

static void rasterizeFlatTopTri( float* afDepthBuffer,
                                 unsigned char* acBuffer, 
                                 int iBufferWidth, 
                                 int iBufferHeight,
                                 tVector2i* pScreenCoord0,
                                 tVector2i* pScreenCoord1,
                                 tVector2i* pScreenCoord2,
                                 float fZCoord0,
                                 float fZCoord1,
                                 float fZCoord2,
                                 tVector2 const* aUV = NULL,
                                 unsigned char const* acTexture = NULL,
                                 int iTextureWidth = 0,
                                 int iTextureHeight = 0 );

static void clipTriangle( tVector4 const* aTri,
                          tVector4* aClipV,
                          float fNearZ,
                          int* piNumClipV,
                          int* aiEdgeIndex,
                          tVector2* aUV = NULL,
                          tVector2* aClipUV = NULL );

//static bool clipEdge( tVector4 const* pV0, 
//                      tVector4 const* pV1, 
//                      tVector4* pClipV,
//                      float fD,
//                      tVector2 const* pUV0 = NULL,
//                      tVector2 const* pUV1 = NULL,
//                      tVector2* pClipUV = NULL );

/*
**
*/
//bool clipEdge( tVector4 const* pV0, 
//               tVector4 const* pV1, 
//               tVector4* pClipV,
//               float fD, 
//               tVector2 const* pUV0,
//               tVector2 const* pUV1,
//               tVector2* pClipUV )
//{
//    bool bRet = false;
//    
//    float fDenom = pV1->fW - pV0->fW;
//    if( fDenom != 0.0f )
//    {
//        float fT = ( fD - pV0->fW ) / fDenom;
//        if( fT >= 0.0f && fT <= 1.0f )
//        {
//            pClipV->fX = pV0->fX + fT * ( pV1->fX - pV0->fX );
//            pClipV->fY = pV0->fY + fT * ( pV1->fY - pV0->fY );
//            pClipV->fZ = pV0->fZ + fT * ( pV1->fZ - pV0->fZ );
//            pClipV->fW = pV0->fW + fT * ( pV1->fW - pV0->fW );
//            
//            if( pUV0 && pUV1 )
//            {
//                pClipUV->fX = pUV0->fX + fT * ( pUV1->fX - pUV0->fX );
//                pClipUV->fY = pUV0->fY + fT * ( pUV1->fY - pUV0->fY );
//            }
//            
//            bRet = true;
//        }
//    }
//    
//    return bRet;
//}

// TODO: CLIP UV COORDINATES
/*
**
*/
static void clipTriangle( tVector4 const* aTri,
                          tVector4* aClipV,
                          float fNearZ,
                          int* piNumClipV,
                          int* aiEdgeIndex,
                          tVector2* aUV,
                          tVector2* aClipUV )
{
	int iNumClippedV = 0;
	for( int i = 0; i < 3; i++ )
	{
		if( aTri[i].fW < 1.0f )
		{
			// edge one
			int iNext = ( i + 1 ) % 3;
			if( aTri[iNext].fW >= 1.0f && aTri[iNext].fZ >= fNearZ )
			{
				float fT = ( fNearZ - aTri[i].fZ ) / ( aTri[iNext].fZ - aTri[i].fZ );
				aClipV[iNumClippedV].fX = aTri[i].fX + ( aTri[iNext].fX - aTri[i].fX ) * fT;
				aClipV[iNumClippedV].fY = aTri[i].fY + ( aTri[iNext].fY - aTri[i].fY ) * fT;
				aClipV[iNumClippedV].fZ = aTri[i].fZ + ( aTri[iNext].fZ - aTri[i].fZ ) * fT;
				aClipV[iNumClippedV].fW = aTri[i].fW + ( aTri[iNext].fW - aTri[i].fW ) * fT;

				++iNumClippedV;
			}

			// edge two
			int iPrev = ( ( 3 + i ) - 1 ) % 3;
			if( aTri[iPrev].fW >= 1.0f && aTri[iPrev].fZ >= fNearZ )
			{
				float fT = ( fNearZ - aTri[i].fZ ) / ( aTri[iPrev].fZ - aTri[i].fZ );
				aClipV[iNumClippedV].fX = aTri[i].fX + ( aTri[iPrev].fX - aTri[i].fX ) * fT;
				aClipV[iNumClippedV].fY = aTri[i].fY + ( aTri[iPrev].fY - aTri[i].fY ) * fT;
				aClipV[iNumClippedV].fZ = aTri[i].fZ + ( aTri[iPrev].fZ - aTri[i].fZ ) * fT;
				aClipV[iNumClippedV].fW = aTri[i].fW + ( aTri[iPrev].fW - aTri[i].fW ) * fT;

				++iNumClippedV;
			}
			
			// swap to arrange in counter clockwise direction
			if( iNumClippedV - i > 1 ) 
			{
				if( aClipV[iNumClippedV-1].fX < aClipV[iNumClippedV-2].fX )
				{
					tVector4 temp;
					memcpy( &temp, &aClipV[iNumClippedV-1], sizeof( tVector4 ) );
					memcpy( &aClipV[iNumClippedV-1], &aClipV[iNumClippedV-2], sizeof( tVector4 ) );
					memcpy( &aClipV[iNumClippedV-2], &temp, sizeof( tVector4 ) );
				}
			}

			
		}
		else
		{
			memcpy( &aClipV[iNumClippedV], &aTri[i], sizeof( tVector4 ) );
			++iNumClippedV;
		}

		*piNumClipV = iNumClippedV;
	}

#if 0
    // check edges for clipping
    
    // plane: Ax + By + Cz + D = 0
    // N . p + D = 0 => D = -( N . p )
    // ray: p = p0 + t * ( p1 - p0 )
    // intersection: -N . p = -( N . ( p0 + t * ( p1 - p0 ) ) ) = D
    // t = D - ( N . p0 ) / ( N . p1 - N . p0 )
    // t = ( D - ( N . p0 ) ) / N . ( p1 - p0 )
    // N = ( 0, 0, 1 ) D = -1
    // t = ( -1 - z0 ) / ( z1 - z0 )
    
    aiEdgeIndex[0] = aiEdgeIndex[1] = aiEdgeIndex[2] = -1;
    
    float fD = fNearZ;
    
    *piNumClipV = 0;
    
    // edge 0: v0 to v1
    tVector2 const* pUV0 = NULL;
    tVector2 const* pUV1 = NULL;
    tVector2* pClipUV = NULL;
    if( aUV )
    {
        pUV0 = &aUV[0];
        pUV1 = &aUV[1];
        pClipUV = &aClipUV[0];
    }
    
    bool bClipped = clipEdge( &aTri[0], &aTri[1], &aClipV[0], fD, pUV0, pUV1, pClipUV );
    if( bClipped )
    {
        ++( *piNumClipV );
        aiEdgeIndex[0] = 0;
    }
    
    // edge 1: v0 to v2
    pUV0 = NULL;
    pUV1 = NULL;
    pClipUV = NULL;
    if( aUV )
    {
        pUV0 = &aUV[0];
        pUV1 = &aUV[2];
        pClipUV = &aClipUV[1];
    }
    
    bClipped = clipEdge( &aTri[0], &aTri[2], &aClipV[1], fD, pUV0, pUV1, pClipUV );
    if( bClipped )
    {
        ++( *piNumClipV );
        aiEdgeIndex[1] = 1;
    }
    
    // edge 2: v1 to v2
    pUV0 = NULL;
    pUV1 = NULL;
    pClipUV = NULL;
    if( aUV )
    {
        pUV0 = &aUV[1];
        pUV1 = &aUV[2];
        pClipUV = &aClipUV[2];
    }
    
    bClipped = clipEdge( &aTri[1], &aTri[2], &aClipV[2], fD, pUV0, pUV1, pClipUV );
    if( bClipped )
    {
        ++( *piNumClipV );
        aiEdgeIndex[2] = 2;
    }
#endif // #if 0
}

/*
**
*/
void rasterizeClipTri( float* afDepthBuffer,
                       unsigned char* acBuffer, 
                       int iBufferWidth, 
                       int iBufferHeight,
                       tVector4* aVerts,
                       float fNearZ,
                       tVector2* aUV,
                       unsigned char const* acTexture,
                       int iTextureWidth,
                       int iTextureHeight )
{
#if defined( PROFILE )
    double fStartTime = getCurrTime(); 
#endif // PROFILE
    
    // clip triangle 0
    tVector4 aClipV[4];
    memset( aClipV, 0, sizeof( aClipV ) );
    int aiEdgeIndex[3] = { -1, -1, -1 };
    
    tVector2 aClipUV[4];
    memset( aClipUV, 0, sizeof( aClipUV ) );
    
    int iNumClipV = 0;
    clipTriangle( aVerts, 
                  aClipV, 
                  fNearZ, 
                  &iNumClipV, 
                  aiEdgeIndex, 
                  aUV, 
                  aClipUV );
    
    // rasterize triangle or quad
    tVector2i aScreenCoord[4];
    float afZCoord[4];
    tVector2 aNewUV[4];

    if( iNumClipV == 3 )
    {
		bool abOnScreen[3] = { true, true, true };
		
        // convert to screen coordinates
        for( int j = 0; j < 3; j++ )
        {
            float fOneOverW = 1.0f / aClipV[j].fW;
            aClipV[j].fX *= fOneOverW;
            aClipV[j].fY *= fOneOverW;
            aClipV[j].fZ *= fOneOverW;
            
            aScreenCoord[j].iX = (short)( ( aClipV[j].fX * (float)iBufferWidth + (float)iBufferWidth ) * 0.5f );
            aScreenCoord[j].iY = iBufferHeight - (short)( ( aClipV[j].fY * (float)iBufferHeight + (float)iBufferHeight ) * 0.5f );
            afZCoord[j] = aClipV[j].fZ;

			if( aScreenCoord[j].iX < 0 || aScreenCoord[j].iX > iBufferWidth ||
				aScreenCoord[j].iY < 0 || aScreenCoord[j].iY > iBufferHeight )
			{
				abOnScreen[j] = false;
			}

		}	// for j = 0 to 3
        
		// degenerate triangle
		if( ( aScreenCoord[0].iX == aScreenCoord[1].iX && aScreenCoord[0].iY == aScreenCoord[1].iY ) ||
			( aScreenCoord[0].iX == aScreenCoord[2].iX && aScreenCoord[0].iY == aScreenCoord[2].iY ) ||
			( aScreenCoord[1].iX == aScreenCoord[2].iX && aScreenCoord[1].iY == aScreenCoord[2].iY ) )
		{
			return;
		}

		if( ( aScreenCoord[0].iX <= 0 && aScreenCoord[1].iX <= 0 && aScreenCoord[2].iX <= 0 ) ||
			( aScreenCoord[0].iX >= iBufferWidth && aScreenCoord[1].iX >= iBufferWidth && aScreenCoord[2].iX >= iBufferWidth ) )
		{
			return;
		}

		if( ( aScreenCoord[0].iY <= 0 && aScreenCoord[1].iY <= 0 && aScreenCoord[2].iY <= 0 ) ||
			( aScreenCoord[0].iY >= iBufferHeight && aScreenCoord[1].iY >= iBufferHeight && aScreenCoord[2].iY >= iBufferHeight ) )
		{
			return;
		}


        // flat top, check which is on left or right
        if( aScreenCoord[0].iY == aScreenCoord[1].iY )
        {
            if( aScreenCoord[0].iX > aScreenCoord[1].iX )
            {
                tVector2i temp;
                memcpy( &temp, &aScreenCoord[0], sizeof( tVector2i ) );
                memcpy( &aScreenCoord[0], &aScreenCoord[1], sizeof( tVector2i ) );
                memcpy( &aScreenCoord[1], &temp, sizeof( tVector2i ) );
                
                float fTemp = afZCoord[0];
                afZCoord[0] = afZCoord[1];
                afZCoord[1] = fTemp;
                
                if( aUV )
                {
                    tVector2 tempV2;
                    memcpy( &tempV2, &aUV[0], sizeof( tVector2 ) );
                    memcpy( &aUV[0], &aUV[1], sizeof( tVector2 ) );
                    memcpy( &aUV[1], &tempV2, sizeof( tVector2 ) );
                }
            }
        }
        
        // sort by height, smallest to largest
        for( int i = 0; i < 3; i++ )
        {
            for( int j = i; j < 3; j++ )
            {
                // swap
                if( aScreenCoord[j].iY < aScreenCoord[i].iY )
                {
                    tVector2i temp;
                    memcpy( &temp, &aScreenCoord[i], sizeof( tVector2i ) );
                    memcpy( &aScreenCoord[i], &aScreenCoord[j], sizeof( tVector2i ) );
                    memcpy( &aScreenCoord[j], &temp, sizeof( tVector2i ) );
                    
                    float fTemp = afZCoord[i];
                    afZCoord[i] = afZCoord[j];
                    afZCoord[j] = fTemp;
                    
                    if( aUV )
                    {
                        tVector2 tempV2;
                        memcpy( &tempV2, &aUV[i], sizeof( tVector2 ) );
                        memcpy( &aUV[i], &aUV[j], sizeof( tVector2 ) );
                        memcpy( &aUV[j], &tempV2, sizeof( tVector2 ) );
                    }

                }
            }	// for j = 0 to 4
        }	// for i = 0 to 4
        
        rasterizeTri( afDepthBuffer, 
                      acBuffer, 
                      iBufferWidth, 
                      iBufferHeight, 
                      &aScreenCoord[0], 
                      &aScreenCoord[1], 
                      &aScreenCoord[2], 
                      afZCoord[0], 
                      afZCoord[1], 
                      afZCoord[2],
                      aNewUV,
                      acTexture,
                      iTextureWidth,
                      iTextureHeight );
    }
    else if( iNumClipV == 4 )
    {
        // convert to screen coordinates
        for( int j = 0; j < 4; j++ )
        {
            float fOneOverW = 1.0f / aClipV[j].fW;
            aClipV[j].fX *= fOneOverW;
            aClipV[j].fY *= fOneOverW;
            aClipV[j].fZ *= fOneOverW;
            
            aScreenCoord[j].iX = (short)( ( aClipV[j].fX * (float)iBufferWidth + (float)iBufferWidth ) * 0.5f );
            aScreenCoord[j].iY = iBufferHeight - (short)( ( aClipV[j].fY * (float)iBufferHeight + (float)iBufferHeight ) * 0.5f );
            afZCoord[j] = aClipV[j].fZ;
        }
        
		if( ( aScreenCoord[0].iX <= 0 && aScreenCoord[1].iX <= 0 && aScreenCoord[2].iX <= 0 && aScreenCoord[3].iX <= 0 ) ||
			( aScreenCoord[0].iX >= iBufferWidth && aScreenCoord[1].iX >= iBufferWidth && aScreenCoord[2].iX >= iBufferWidth && aScreenCoord[3].iX >= iBufferWidth ) )
		{
			return;
		}

		if( ( aScreenCoord[0].iY <= 0 && aScreenCoord[1].iY <= 0 && aScreenCoord[2].iY <= 0 && aScreenCoord[3].iY <= 0 ) ||
			( aScreenCoord[0].iY >= iBufferHeight && aScreenCoord[1].iY >= iBufferHeight && aScreenCoord[2].iY >= iBufferHeight && aScreenCoord[3].iY >= iBufferHeight ) )
		{
			return;
		}

        // TODO: check if all verts are out of screen, then don't bother rasterize it

        rasterizeQuad( afDepthBuffer, 
                       acBuffer, 
                       iBufferWidth, 
                       iBufferHeight, 
                       aScreenCoord, 
                       afZCoord,
                       aNewUV,
                       acTexture,
                       iTextureWidth,
                       iTextureHeight );
    }
    
#if defined( PROFILE )
    double fElapsed = getCurrTime() - fStartTime; 
    safProfileTimes[PROFILE_RASTER_CLIP] += fElapsed;
#endif // PROFILE

}

#if 0
/*
**
*/
void rasterizeModel( CModel* pModel,
                     tVector4 const* pModelPos,
                     float* afDepthBuffer,
					 unsigned char* acBuffer, 
					 int iBufferWidth, 
					 int iBufferHeight,
					 tMatrix44 const* pViewProjMatrix,
                     tVector4 const* pCamPos )
{    
    float fNearZ = 1.0f;
    
    memset( aProjPos, 0, sizeof( aProjPos ) );
    
	// transform all vertices
    int iNumPos = pModel->getNumPos();
	for( int i = 0; i < iNumPos; i++ )
	{
		tVector4 xform;
        tVector4 const* pV = pModel->getVertexPos( i );
        
        // add in the position
        tVector4 vertPos = 
        { 
            pV->fX + pModelPos->fX, 
            pV->fY + pModelPos->fY, 
            pV->fZ + pModelPos->fZ, 
            1.0f 
        };
        
        Matrix44Transform( &xform, &vertPos, pViewProjMatrix );
		memcpy( &aClipSpacePos[i], &xform, sizeof( tVector4 ) );
        
        float fOneOverW = 1.0f / xform.fW;
        aProjPos[i].fX = xform.fX * fOneOverW;
        aProjPos[i].fY = xform.fY * fOneOverW;
        afZ[i] = xform.fZ * fOneOverW;
    }
	
    tVector4 aTri0[3];
    tVector4 aTri1[3];
    
    int iNumFaces = pModel->getNumFaces();
    for( int i = 0; i < iNumFaces; i++ )
    {
        tFace const* pFace = pModel->getFace( i );
        
        // compute normal in camera space, camera's view direction is always ( 0, 0, 1 ) in camera space, so just check for z
        tVector4 edge0, edge1, normal;
        Vector4Subtract( &edge0, &aClipSpacePos[pFace->maiV[1]], &aClipSpacePos[pFace->maiV[0]] );
        Vector4Subtract( &edge1, &aClipSpacePos[pFace->maiV[2]], &aClipSpacePos[pFace->maiV[0]] );
        Vector4Cross( &normal, &edge0, &edge1 );
        
        // front face
        float fDP = normal.fZ;
        if( fDP <= 0.0f )
        {
            // check if any of the vertices are back of the near plane
            if( aClipSpacePos[pFace->maiV[0]].fW < fNearZ ||
                aClipSpacePos[pFace->maiV[1]].fW < fNearZ ||
                aClipSpacePos[pFace->maiV[2]].fW < fNearZ )
            {
                // back of camera
                // triangle 0
                memcpy( &aTri0[0], &aClipSpacePos[pFace->maiV[0]], sizeof( tVector4 ) );
                memcpy( &aTri0[1], &aClipSpacePos[pFace->maiV[2]], sizeof( tVector4 ) );
                memcpy( &aTri0[2], &aClipSpacePos[pFace->maiV[1]], sizeof( tVector4 ) );
                
                rasterizeClipTri( afDepthBuffer, 
                                  acBuffer, 
                                  iBufferWidth, 
                                  iBufferHeight, 
                                  aTri0, 
                                  fNearZ );
                
                // triangle 1
                memcpy( &aTri1[0], &aClipSpacePos[pFace->maiV[0]], sizeof( tVector4 ) );
                memcpy( &aTri1[1], &aClipSpacePos[pFace->maiV[3]], sizeof( tVector4 ) );
                memcpy( &aTri1[2], &aClipSpacePos[pFace->maiV[2]], sizeof( tVector4 ) );
                
                rasterizeClipTri( afDepthBuffer, 
                                  acBuffer, 
                                  iBufferWidth, 
                                  iBufferHeight, 
                                  aTri1, 
                                  fNearZ );
                 
            }   // if it's a triangle
            else
            {
                // convert to screen coordinate
                tVector2i aScreenCoord[4];
                float afZCoord[4];
                for( int j = 0; j < 4; j++ )
                {
                    aScreenCoord[j].iX = (short)( ( aProjPos[pFace->maiV[j]].fX * (float)iBufferWidth + (float)iBufferWidth ) * 0.5f );
                    aScreenCoord[j].iY = iBufferHeight - (short)( ( aProjPos[pFace->maiV[j]].fY * (float)iBufferHeight + (float)iBufferHeight ) * 0.5f );
                    afZCoord[j] = afZ[pFace->maiV[j]];
                }
                
                // rasterize
                rasterizeQuad( afDepthBuffer,
                               acBuffer, 
                               iBufferWidth, 
                               iBufferHeight, 
                               aScreenCoord,
                               afZCoord );
                
            }   // else if it's a quad
        
        }   // if front face
        
    }   // for i = 0 to num faces
}
#endif // #if 0


/*
**
*/
static void rasterizeQuad( float* afDepthBuffer,
                           unsigned char* acBuffer, 
						   int iBufferWidth, 
						   int iBufferHeight,
						   tVector2i* aScreenCoord,
                           float* afZCoord,
                           tVector2* aUV,
                           unsigned char const* acTexture,
                           int iTextureWidth,
                           int iTextureHeight )
{
	tVector2i aTri0[3];
	tVector2i aTri1[3];
	
    tVector2 aUV0[3];
    tVector2 aUV1[3];
    
	float afZ0[3] = { afZCoord[0], afZCoord[2], afZCoord[1] };
	float afZ1[3] = { afZCoord[0], afZCoord[3], afZCoord[2] };

	// triangle 1
	memcpy( &aTri0[0], &aScreenCoord[0], sizeof( tVector2i ) );
	memcpy( &aTri0[1], &aScreenCoord[2], sizeof( tVector2i ) );
	memcpy( &aTri0[2], &aScreenCoord[1], sizeof( tVector2i ) );

	// triangle 2
	memcpy( &aTri1[0], &aScreenCoord[0], sizeof( tVector2i ) );
	memcpy( &aTri1[1], &aScreenCoord[3], sizeof( tVector2i ) );
	memcpy( &aTri1[2], &aScreenCoord[2], sizeof( tVector2i ) );

    // triangle 1
    if( aUV )
    {
        memcpy( &aUV0[0], &aUV[0], sizeof( tVector2 ) );
        memcpy( &aUV0[1], &aUV[2], sizeof( tVector2 ) );
        memcpy( &aUV0[2], &aUV[1], sizeof( tVector2 ) );
        
        // triangle 2
        memcpy( &aUV1[0], &aUV[0], sizeof( tVector2 ) );
        memcpy( &aUV1[1], &aUV[3], sizeof( tVector2 ) );
        memcpy( &aUV1[2], &aUV[2], sizeof( tVector2 ) );
    }
    
	// flat top, check which is on left or right
	if( aTri0[0].iY == aTri0[1].iY )
	{
		if( aTri0[0].iX > aTri0[1].iX )
		{
			tVector2i temp;
			memcpy( &temp, &aTri0[0], sizeof( tVector2i ) );
			memcpy( &aTri0[0], &aTri0[1], sizeof( tVector2i ) );
			memcpy( &aTri0[1], &temp, sizeof( tVector2i ) );

			float fTemp = afZ0[0];
			afZ0[0] = afZ0[1];
			afZ0[1] = fTemp;
            
            if( aUV )
            {
                tVector2 tempV2;
                memcpy( &tempV2, &aUV0[0], sizeof( tVector2 ) );
                memcpy( &aUV0[0], &aUV0[1], sizeof( tVector2 ) );
                memcpy( &aUV0[1], &tempV2, sizeof( tVector2 ) );
            }
        }
	}

	// flat top, check which is on left or right
	if( aTri1[0].iY == aTri1[1].iY )
	{
		if( aTri1[0].iX > aTri1[1].iX )
		{
			tVector2i temp;
			memcpy( &temp, &aTri1[0], sizeof( tVector2i ) );
			memcpy( &aTri1[0], &aTri1[1], sizeof( tVector2i ) );
			memcpy( &aTri1[1], &temp, sizeof( tVector2i ) );

			float fTemp = afZ1[0];
			afZ1[0] = afZ1[1];
			afZ1[1] = fTemp;
            
            if( aUV )
            {
                tVector2 tempV2;
                memcpy( &tempV2, &aUV1[0], sizeof( tVector2 ) );
                memcpy( &aUV1[0], &aUV1[1], sizeof( tVector2 ) );
                memcpy( &aUV1[1], &tempV2, sizeof( tVector2 ) );
            }
		}
	}

	// sort by height, smallest to largest
	for( int i = 0; i < 3; i++ )
	{
		for( int j = i; j < 3; j++ )
		{
			// swap
			if( aTri0[j].iY < aTri0[i].iY )
			{
				tVector2i temp;
				memcpy( &temp, &aTri0[i], sizeof( tVector2i ) );
				memcpy( &aTri0[i], &aTri0[j], sizeof( tVector2i ) );
				memcpy( &aTri0[j], &temp, sizeof( tVector2i ) );

				float fTemp = afZ0[i];
				afZ0[i] = afZ0[j];
				afZ0[j] = fTemp;
                
                if( aUV )
                {
                    tVector2 tempV2;
                    memcpy( &tempV2, &aUV0[i], sizeof( tVector2 ) );
                    memcpy( &aUV0[i], &aUV0[j], sizeof( tVector2 ) );
                    memcpy( &aUV0[j], &tempV2, sizeof( tVector2 ) );
                }
            }
		}	// for j = 0 to 4
	}	// for i = 0 to 4
	
	for( int i = 0; i < 3; i++ )
	{
		for( int j = i; j < 3; j++ )
		{
			// swap
			if( aTri1[j].iY < aTri1[i].iY )
			{
				tVector2i temp;
				memcpy( &temp, &aTri1[i], sizeof( tVector2i ) );
				memcpy( &aTri1[i], &aTri1[j], sizeof( tVector2i ) );
				memcpy( &aTri1[j], &temp, sizeof( tVector2i ) );

				float fTemp = afZ1[i];
				afZ1[i] = afZ1[j];
				afZ1[j] = fTemp;
                
                if( aUV )
                {
                    tVector2 tempV2;
                    memcpy( &tempV2, &aUV1[i], sizeof( tVector2 ) );
                    memcpy( &aUV1[i], &aUV1[j], sizeof( tVector2 ) );
                    memcpy( &aUV1[j], &tempV2, sizeof( tVector2 ) );
                }

			}
		}	// for j = 0 to 4
	}	// for i = 0 to 4

	// 2 triangles
    tVector2* paUV0 = NULL;
    tVector2* paUV1 = NULL;
    if( aUV )
    {
        paUV0 = aUV0;
        paUV1 = aUV1;
    }

	rasterizeTri( afDepthBuffer,
                  acBuffer, 
				  iBufferWidth,
				  iBufferHeight,
				  &aTri0[0],
				  &aTri0[1],
				  &aTri0[2],
				  afZ0[0],
				  afZ0[1],
				  afZ0[2],
                  paUV0,
                  acTexture,
                  iTextureWidth,
                  iTextureHeight );

	rasterizeTri( afDepthBuffer,
                  acBuffer, 
				  iBufferWidth,
				  iBufferHeight,
				  &aTri1[0],
				  &aTri1[1],
				  &aTri1[2],
				  afZ1[0],
				  afZ1[1],
                  afZ1[2],
                  paUV1,
                  acTexture,
                  iTextureWidth,
                  iTextureHeight );
}

/*
**
*/
void rasterizeTri( float* afDepthBuffer,
                   unsigned char* acBuffer, 
                   int iBufferWidth, 
                   int iBufferHeight,
                   tVector2i* pScreenCoord0,
                   tVector2i* pScreenCoord1,
                   tVector2i* pScreenCoord2,
                   float fZCoord0,
                   float fZCoord1,
                   float fZCoord2,
                   tVector2* aUV,
                   unsigned char const* acTexture,
                   int iTextureWidth,
                   int iTextureHeight )
{
#if defined( PROFILE )
    double fStartTime = getCurrTime(); 
#endif // PROFILE
    
	if( pScreenCoord0->iY == pScreenCoord1->iY )
	{
		// swap from left to right
		if( pScreenCoord0->iX > pScreenCoord1->iX )
		{
			tVector2i temp;
			memcpy( &temp, pScreenCoord0, sizeof( tVector2i ) );
			memcpy( pScreenCoord0, pScreenCoord1, sizeof( tVector2i ) );
			memcpy( pScreenCoord1, &temp, sizeof( tVector2i ) );
            
            float fTemp;
            fTemp = fZCoord0;
            fZCoord0 = fZCoord1;
            fZCoord1 = fTemp;
            
            if( aUV )
            {
                tVector2 temp;
                memcpy( &temp, &aUV[0], sizeof( tVector2 ) );
                memcpy( &aUV[0], &aUV[1], sizeof( tVector2 ) );
                memcpy( &aUV[1], &temp, sizeof( tVector2 ) );
            }
		}

		// flat top
		rasterizeFlatTopTri( afDepthBuffer,
                             acBuffer, 
							 iBufferWidth, 
							 iBufferHeight, 
							 pScreenCoord0, 
							 pScreenCoord1, 
							 pScreenCoord2,
							 fZCoord0,
							 fZCoord1,
							 fZCoord2,
                             (tVector2 const*)aUV,
                             acTexture,
                             iTextureWidth,
                             iTextureHeight );
	}
	else if( pScreenCoord1->iY == pScreenCoord2->iY )
	{
		// swap from left to right
		if( pScreenCoord1->iX > pScreenCoord2->iX )
		{
			tVector2i temp;
			memcpy( &temp, pScreenCoord1, sizeof( tVector2i ) );
			memcpy( pScreenCoord1, pScreenCoord2, sizeof( tVector2i ) );
			memcpy( pScreenCoord2, &temp, sizeof( tVector2i ) );
            
            float fTemp;
            fTemp = fZCoord1;
            fZCoord1 = fZCoord2;
            fZCoord2 = fTemp;
		}

		// flat bottom
		rasterizeFlatBottomTri( afDepthBuffer,
                                acBuffer, 
								iBufferWidth, 
								iBufferHeight, 
								pScreenCoord0, 
								pScreenCoord1, 
								pScreenCoord2,
								fZCoord0,
								fZCoord1,
								fZCoord2,
                                aUV,
                                acTexture,
                                iTextureWidth,
                                iTextureHeight );
	}
	else
	{
		float fDXDYLeft = 0.0f;
		float fDXDYRight = 0.0f;
		//float fDZDYLeft = 0.0f;
		//float fDZDYRight = 0.0f;
        
        float fDOneOverZDYLeft = 0.0f;
        float fDOneOverZDYRight = 0.0f;
        
		// to check which vertex is on the left or right
		float fDXDY0 = (float)( pScreenCoord1->iX - pScreenCoord0->iX ) / (float)( pScreenCoord1->iY - pScreenCoord0->iY );
		float fDXDY1 = (float)( pScreenCoord2->iX - pScreenCoord0->iX ) / (float)( pScreenCoord2->iY - pScreenCoord0->iY );

		float fHeight = (float)( pScreenCoord1->iY - pScreenCoord0->iY );
		
		float fZ0 = fZCoord0;
		float fZ1 = fZCoord1;
		float fZ2 = fZCoord2;
		float fZ3 = fZCoord2;

        // 1 / z
        float fOneOverZCoord0 = DEFAULT_ONE_OVER_Z;
        float fOneOverZCoord1 = DEFAULT_ONE_OVER_Z;
        float fOneOverZCoord2 = DEFAULT_ONE_OVER_Z;
        
        // clamp 1 / z
        if( fZCoord0 >= Z_CLAMP )
        {
            fOneOverZCoord0 = 1.0f / fZCoord0;
        }
        
        if( fZCoord1 >= Z_CLAMP )
        {
            fOneOverZCoord1 = 1.0f / fZCoord1;
        }
        
        if( fZCoord2 >= Z_CLAMP )
        {
            fOneOverZCoord2 = 1.0f / fZCoord2;
        }
        
        
		tVector2i leftV, rightV;
		if( fDXDY0 <= fDXDY1 )
		{
			// vertex 1 is on the left and vertex 2 is on the right

            float fDYRight = (float)( pScreenCoord2->iY - pScreenCoord0->iY );
            
			fDXDYRight = (float)( pScreenCoord2->iX - pScreenCoord0->iX ) / fDYRight;
            fDOneOverZDYRight = (float)( fOneOverZCoord2 - fOneOverZCoord0 ) / fDYRight;
            
            //fDZDYRight = (float)( fZCoord2 - fZCoord0 ) / (float)( pScreenCoord2->iY - pScreenCoord0->iY );
            
			// left
			memcpy( &leftV, pScreenCoord1, sizeof( tVector2i ) );
			
			// right
			rightV.iX = pScreenCoord0->iX + (short)( fDXDYRight * fHeight );
			rightV.iY = pScreenCoord1->iY;

			fZ0 = fZCoord0;
			fZ1 = fZCoord1;
			fZ2 = 1.0f / ( fOneOverZCoord0 + fDOneOverZDYRight * fHeight );     // z = 1 / ( 1 / z )
            
            //fZ2 = fZCoord0 + fDZDYRight * fHeight;

			assert( leftV.iX <= rightV.iX );
		}
		else
		{
			// vertex 2 is on the left and vertex 1 is on the right
            
            float fDYLeft = (float)( pScreenCoord2->iY - pScreenCoord0->iY );
			fDXDYLeft = (float)( pScreenCoord2->iX - pScreenCoord0->iX ) / fDYLeft;
            fDOneOverZDYLeft = ( fOneOverZCoord2 - fOneOverZCoord0 ) / fDYLeft;
            
            //fDZDYLeft = (float)( fZCoord2 - fZCoord0 ) / (float)( pScreenCoord2->iY - pScreenCoord0->iY );
            
			// left
			leftV.iX = pScreenCoord0->iX + (short)( fDXDYLeft * fHeight );
			leftV.iY = pScreenCoord1->iY;

			// right
			rightV.iX = pScreenCoord1->iX;
			rightV.iY = pScreenCoord1->iY;

			fZ0 = fZCoord0;
            fZ1 = 1.0f / ( fOneOverZCoord0 + fDOneOverZDYLeft * fHeight );      // z = 1 / ( 1 / z )
            fZ2 = fZCoord1;

            //fZ1 = fZCoord0 + fDZDYLeft * fHeight;
            
			assert( leftV.iX <= rightV.iX );
		}

		rasterizeFlatBottomTri( afDepthBuffer,
                                acBuffer, 
								iBufferWidth, 
								iBufferHeight, 
								pScreenCoord0,
								&leftV, 
								&rightV, 
								fZ0,
								fZ1,
								fZ2,
                                aUV,
                                acTexture,
                                iTextureWidth,
                                iTextureHeight );

		rasterizeFlatTopTri( afDepthBuffer,
                             acBuffer, 
							 iBufferWidth, 
							 iBufferHeight, 
							 &leftV, 
							 &rightV,
							 pScreenCoord2,
							 fZ1,
							 fZ2,
                             fZ3,
                             aUV,
                             acTexture,
                             iTextureWidth,
                             iTextureHeight );
	}
    
#if defined( PROFILE )
    double fElapsed = getCurrTime() - fStartTime;
    safProfileTimes[PROFILE_RASTER_TRI] += fElapsed;
#endif // PROFILE
}

/*
**
*/
static void rasterizeFlatBottomTri( float* afDepthBuffer,
                                    unsigned char* acBuffer, 
                                    int iBufferWidth, 
                                    int iBufferHeight,
                                    tVector2i* pScreenCoord0,
                                    tVector2i* pScreenCoord1,
                                    tVector2i* pScreenCoord2,
                                    float fZCoord0,
                                    float fZCoord1,
                                    float fZCoord2,
                                    tVector2 const* aUV,
                                    unsigned char const* acTexture,
                                    int iTextureWidth,
                                    int iTextureHeight )
{
    // 1 / z is linear
	
    float fOneOverZCoord0 = DEFAULT_ONE_OVER_Z;
    float fOneOverZCoord1 = DEFAULT_ONE_OVER_Z;
    float fOneOverZCoord2 = DEFAULT_ONE_OVER_Z;
    
    if( fZCoord0 >= Z_CLAMP )
    {
        fOneOverZCoord0 = 1.0f / fZCoord0;
    }
    
    if( fZCoord1 >= Z_CLAMP )
    {
        fOneOverZCoord1 = 1.0f / fZCoord1;
    }
    
    if( fZCoord2 >= Z_CLAMP )
    {
        fOneOverZCoord2 = 1.0f / fZCoord2;
    }
    
    assert( pScreenCoord1->iY == pScreenCoord2->iY );
    
    // gradients
    float fDYLeft = (float)( pScreenCoord1->iY - pScreenCoord0->iY );
    float fDYRight = (float)( pScreenCoord2->iY - pScreenCoord0->iY );
    
	float fDXDYLeft = (float)( pScreenCoord1->iX - pScreenCoord0->iX ) / fDYLeft;
	float fDXDYRight = (float)( pScreenCoord2->iX - pScreenCoord0->iX ) / fDYRight;

    float fDOneOverZDYLeft = ( fOneOverZCoord1 - fOneOverZCoord0 ) / fDYLeft;
    float fDOneOverZDYRight = ( fOneOverZCoord2 - fOneOverZCoord0 ) / fDYLeft;
    
    // uv: duoverzdy dvoverzdy
    tVector2 dUVOverZDYLeft = { 0.0f, 0.0f };
    tVector2 dUVOverZDYRight = { 0.0f, 0.0f };
    if( aUV )
    {
        dUVOverZDYLeft.fX = ( aUV[1].fX * fOneOverZCoord1 - aUV[0].fX * fOneOverZCoord0 ) / fDYLeft;
        dUVOverZDYLeft.fY = ( aUV[1].fY * fOneOverZCoord1 - aUV[0].fY * fOneOverZCoord0 ) / fDYLeft;
    
        dUVOverZDYRight.fX = ( aUV[2].fX * fOneOverZCoord2 - aUV[0].fX * fOneOverZCoord0 ) / fDYRight;
        dUVOverZDYRight.fY = ( aUV[2].fY * fOneOverZCoord2 - aUV[0].fY * fOneOverZCoord0 ) / fDYRight;
    }
    
    
    // screen height
	int iHeight = (int)( pScreenCoord1->iY - pScreenCoord0->iY );
	for( int i = 0; i < iHeight; i++ )
	{
		float fIter = (float)i;

		// out of screen height
		if( pScreenCoord0->iY + i < 0 || pScreenCoord0->iY + i >= iBufferHeight )
		{
			continue;
		}

		float fScreenX0 = (float)pScreenCoord0->iX;

		// start and end of scanline
		float fStartX = fScreenX0 + fDXDYLeft * fIter;
		float fEndX = fScreenX0 + fDXDYRight * fIter;

		int iStartX = (int)fStartX;
		int iEndX = (int)fEndX;
	
		// clamp due to floating point inaccuracy
		if( iEndX < iStartX )
		{
			assert( abs( iEndX - iStartX ) <= 1 );
			iEndX = iStartX;
		}

		int iScanLineWidth = iEndX - iStartX;

		// start and end of z
		float fStartOneOverZ = fOneOverZCoord0 + fDOneOverZDYLeft * fIter;
        float fEndOneOverZ = fOneOverZCoord0 + fDOneOverZDYRight * fIter;
        
		// dz/dx
		float fDOneOverZDX = 0.0f;
		if( iScanLineWidth > 0 )
		{
			fDOneOverZDX = ( fEndOneOverZ - fStartOneOverZ ) / (float)iScanLineWidth;
        }
        
        // UVOverZ at this scanline
        tVector2 startUVOverZ = { 0.0f, 0.0f }; 
        tVector2 endUVOverZ = { 0.0f, 0.0f };
        tVector2 dUVOverZDX = { 0.0f, 0.0f };
        if( aUV )
        {
            // start UVOverZ
            startUVOverZ.fX = aUV[0].fX * fOneOverZCoord0 + dUVOverZDYLeft.fX * fIter;
            startUVOverZ.fY = aUV[0].fY * fOneOverZCoord0 + dUVOverZDYLeft.fY * fIter;
            
            // end UVOverZ
            endUVOverZ.fX = aUV[0].fX * fOneOverZCoord0 + dUVOverZDYRight.fX * fIter;
            endUVOverZ.fY = aUV[0].fY * fOneOverZCoord0 + dUVOverZDYRight.fY * fIter;
        
            // dUVOverZDX
            dUVOverZDX.fX = 0.0f;
            dUVOverZDX.fY = 0.0f;
            
            if( iScanLineWidth > 0 )
            {
                dUVOverZDX.fX = ( endUVOverZ.fX - startUVOverZ.fX ) / (float)iScanLineWidth;
                dUVOverZDX.fY = ( endUVOverZ.fY - startUVOverZ.fY ) / (float)iScanLineWidth;
            }
        }
        
		// scanline 
		for( int j = 0; j <= iScanLineWidth; j++ )
		{
			if( iStartX + j >= 0 && iStartX + j < iBufferWidth )
			{
				//float fZ = fStartZ + (float)j * fDZDX;
				float fDenom = fStartOneOverZ + (float)j * fDOneOverZDX;
                
                float fZ = 0.0f; 
                if( fDenom != 0.0f )
                {
                    fZ = 1.0f / fDenom;     // 1 / ( 1 / z )
                }
                
                //fZ = ( fZ + 1.0f ) * 0.5f;					// from ( -1, 1 ) to ( 0, 1 )
                
                // indices into depth array and color buffer
				int iIndex = ( ( ( pScreenCoord0->iY + i ) * iBufferWidth ) + ( iStartX + j ) ) * 4;
				int iDepthIndex = ( ( pScreenCoord0->iY + i ) * iBufferWidth ) + ( iStartX + j );
                
                WTFASSERT2( iIndex >= 0 && iIndex < iBufferWidth * iBufferHeight * 4, "depth color buffer out of bounds" );
                WTFASSERT2( iDepthIndex >= 0 && iDepthIndex < iBufferWidth * iBufferHeight, "depth buffer out of bounds" );
                
				// check the z value in this pixel
				float fThisZ = afDepthBuffer[iDepthIndex];
				if( fThisZ == 0.0f || fZ < fThisZ )
				{
                    afDepthBuffer[iDepthIndex] = fZ;
                    
					acBuffer[iIndex] = (unsigned char)( ( 1.0f - fZ ) * 255.0f );
					acBuffer[iIndex+1] = (unsigned char)( ( 1.0f - fZ ) * 255.0f );
					acBuffer[iIndex+2] = (unsigned char)( ( 1.0f - fZ ) * 255.0f );
					acBuffer[iIndex+3] = 255;

acBuffer[iIndex] = 255;
acBuffer[iIndex+1] = 255;
acBuffer[iIndex+2] = 255;

				}
			
                // UV
                if( aUV && acTexture )
                {
                    tVector2 uv = { 0.0f, 0.0f };
                    uv.fX = startUVOverZ.fX + (float)j * dUVOverZDX.fX;
                    uv.fY = startUVOverZ.fY + (float)j * dUVOverZDX.fY;
                    
                    // perspective, uv/z / z = uv
                    uv.fX /= fDenom;
                    uv.fY /= fDenom;
                    
                    tVector2i texCoord = 
                    {
                        (short)( uv.fX * (float)iTextureWidth ),
                        (short)( uv.fY * (float)iTextureHeight )
                    };
                    
                    if( texCoord.iX >= iTextureWidth )
                    {
                        texCoord.iX = iTextureWidth - 1;
                    }
                    
                    if( texCoord.iY >= iTextureHeight )
                    {
                        texCoord.iY = iTextureHeight - 1;
                    }
                    
                    int iUVIndex = ( (int)( texCoord.iY * (float)iTextureWidth ) + texCoord.iX ) * 4;
                    acBuffer[iIndex] = acTexture[iUVIndex];
                    acBuffer[iIndex+1] = acTexture[iUVIndex+1];
                    acBuffer[iIndex+2] = acTexture[iUVIndex+2];
                    acBuffer[iIndex+3] = acTexture[iUVIndex+3];
                }
                
            }   // if within device's width 
		
        }   // for j = 0 to scanline width
        
	}   // for i = 0 to height 
}

/*
**
*/
static void rasterizeFlatTopTri( float* afDepthBuffer,
                                 unsigned char* acBuffer, 
								 int iBufferWidth, 
								 int iBufferHeight,
								 tVector2i* pScreenCoord0,
								 tVector2i* pScreenCoord1,
								 tVector2i* pScreenCoord2,
								 float fZCoord0,
								 float fZCoord1,
                                 float fZCoord2,
                                 tVector2 const* aUV,
                                 unsigned char const* acTexture,
                                 int iTextureWidth,
                                 int iTextureHeight )
{
    float fOneOverZCoord0 = DEFAULT_ONE_OVER_Z;
    float fOneOverZCoord1 = DEFAULT_ONE_OVER_Z;
    float fOneOverZCoord2 = DEFAULT_ONE_OVER_Z;
    
    if( fZCoord0 >= Z_CLAMP )
    {
        fOneOverZCoord0 = 1.0f / fZCoord0;
    }
    
    if( fZCoord1 >= Z_CLAMP )
    {
        fOneOverZCoord1 = 1.0f / fZCoord1;
    }
    
    if( fZCoord2 >= Z_CLAMP )
    {
        fOneOverZCoord2 = 1.0f / fZCoord2;
    }
    
    // gradient
    float fDYLeft = (float)( pScreenCoord2->iY - pScreenCoord0->iY );
    float fDYRight = (float)( pScreenCoord2->iY - pScreenCoord1->iY );
    
	float fDXDYLeft = (float)( pScreenCoord2->iX - pScreenCoord0->iX ) / fDYLeft;
	float fDXDYRight = (float)( pScreenCoord2->iX - pScreenCoord1->iX ) / fDYRight;

    float fDOneOverZDYLeft = ( fOneOverZCoord2 - fOneOverZCoord0 ) / fDYLeft;
    float fDOneOverZDYRight = ( fOneOverZCoord2 - fOneOverZCoord1 ) / fDYRight;
    
    // uv: duoverzdy dvoverzdy
    tVector2 dUVOverZDYLeft = { 0.0f, 0.0f };
    tVector2 dUVOverZDYRight = { 0.0f, 0.0f };
    if( aUV )
    {
        dUVOverZDYLeft.fX = ( aUV[2].fX * fOneOverZCoord2 - aUV[0].fX * fOneOverZCoord0 ) / fDYLeft;
        dUVOverZDYLeft.fY = ( aUV[2].fY * fOneOverZCoord2 - aUV[0].fY * fOneOverZCoord0 ) / fDYLeft;
        
        dUVOverZDYRight.fX = ( aUV[2].fX * fOneOverZCoord2 - aUV[1].fX * fOneOverZCoord1 ) / fDYRight;
        dUVOverZDYRight.fY = ( aUV[2].fY * fOneOverZCoord2 - aUV[1].fY * fOneOverZCoord1 ) / fDYRight;
    }
    
	int iHeight = (int)( pScreenCoord2->iY - pScreenCoord0->iY );
	for( int i = 0; i <= iHeight; i++ )
	{
		float fIter = (float)i;
		int iY = pScreenCoord0->iY + i;

		// out of screen height
		if( pScreenCoord0->iY + i < 0 || pScreenCoord0->iY + i >= iBufferHeight )
		{
			continue;
		}

		// start and end of scanline
		float fStartX = (float)pScreenCoord0->iX + fDXDYLeft * fIter;
		float fEndX = (float)pScreenCoord1->iX + fDXDYRight * fIter;

		int iStartX = (int)fStartX;
		int iEndX = (int)fEndX;

		// clamp due to floating point inaccuracy
		if( iEndX < iStartX )
		{
			assert( abs( iEndX - iStartX ) <= 1 );
			iEndX = iStartX;
		}
	
		int iScanLineWidth = iEndX - iStartX;

		// start and end of z
		float fStartOneOverZ = fOneOverZCoord0 + fDOneOverZDYLeft * fIter;
        float fEndOneOverZ = fOneOverZCoord1 + fDOneOverZDYRight * fIter;
        
		// dz/dx
		float fDOneOverZDX = 0.0f;
        if( iScanLineWidth > 0 )
		{
			fDOneOverZDX = ( fEndOneOverZ - fStartOneOverZ ) / (float)iScanLineWidth;
		}

        // UVOverZ at this scanline
        tVector2 startUVOverZ = { 0.0f, 0.0f }; 
        tVector2 endUVOverZ = { 0.0f, 0.0f };
        tVector2 dUVOverZDX = { 0.0f, 0.0f };
        if( aUV )
        {
            // start UVOverZ
            startUVOverZ.fX = aUV[0].fX * fOneOverZCoord0 + dUVOverZDYLeft.fX * fIter;
            startUVOverZ.fY = aUV[0].fY * fOneOverZCoord0 + dUVOverZDYLeft.fY * fIter;
            
            // end UVOverZ
            endUVOverZ.fX = aUV[1].fX * fOneOverZCoord1 + dUVOverZDYRight.fX * fIter;
            endUVOverZ.fY = aUV[1].fY * fOneOverZCoord1 + dUVOverZDYRight.fY * fIter;
            
            // dUVOverZDX
            dUVOverZDX.fX = ( endUVOverZ.fX - startUVOverZ.fX ) / (float)iScanLineWidth;
            dUVOverZDX.fY = ( endUVOverZ.fY - startUVOverZ.fY ) / (float)iScanLineWidth;
        }
        
		// scanline 
		for( int j = 0; j <= iScanLineWidth; j++ )
		{
            // within the screen width
			if( iStartX + j >= 0 && iStartX + j < iBufferWidth )
			{
				//float fZ = fStartZ + (float)j * fDZDX;
                float fDenom = fStartOneOverZ + (float)j * fDOneOverZDX;
                
                float fZ = 0.0f; 
                if( fDenom != 0.0f )
                {
                    fZ = 1.0f / fDenom;     // 1 / ( 1 / z )
                }
                
				//fZ = ( fZ + 1.0f ) * 0.5f;					// from ( -1, 1 ) to ( 0, 1 )

				int iIndex = ( ( iY * iBufferWidth ) + ( iStartX + j ) ) * 4;
                int iDepthIndex = ( iY * iBufferWidth ) + ( iStartX + j );
                
                WTFASSERT2( iIndex >= 0 && iIndex < iBufferWidth * iBufferHeight * 4, "depth color buffer out of bounds" );
                WTFASSERT2( iDepthIndex >= 0 && iDepthIndex < iBufferWidth * iBufferHeight, "depth buffer out of bounds" );
                
				// check the z value in this pixel
				//float fThisZ = (float)acBuffer[iIndex] / 255.0f;
				float fThisZ = afDepthBuffer[iDepthIndex];
                
				if( fThisZ == 0.0f || fZ < fThisZ )
				{
                    afDepthBuffer[iDepthIndex] = fZ;
                    
					acBuffer[iIndex] = (unsigned char)( ( 1.0f - fZ ) * 255.0f );
					acBuffer[iIndex+1] = (unsigned char)( ( 1.0f - fZ ) * 255.0f );
					acBuffer[iIndex+2] = (unsigned char)( ( 1.0f - fZ ) * 255.0f );
					acBuffer[iIndex+3] = 255;
				
acBuffer[iIndex] = 255;
acBuffer[iIndex+1] = 255;
acBuffer[iIndex+2] = 255;
				}
                
                // UV
                if( aUV && acTexture )
                {
                    tVector2 uv = { 0.0f, 0.0f };
                    uv.fX = startUVOverZ.fX + (float)j * dUVOverZDX.fX;
                    uv.fY = startUVOverZ.fY + (float)j * dUVOverZDX.fY;
                    
                    // perspective, uv/z / z = uv
                    uv.fX /= fDenom;
                    uv.fY /= fDenom;
                    
                    tVector2i texCoord = 
                    {
                        (short)( uv.fX * (float)iTextureWidth ),
                        (short)( uv.fY * (float)iTextureHeight )
                    };
                    
                    if( texCoord.iX >= iTextureWidth )
                    {
                        texCoord.iX = iTextureWidth - 1;
                    }
                    
                    if( texCoord.iY >= iTextureHeight )
                    {
                        texCoord.iY = iTextureHeight - 1;
                    }
                    
                    int iUVIndex = ( (int)( texCoord.iY * (float)iTextureWidth ) + texCoord.iX ) * 4;
                    acBuffer[iIndex] = acTexture[iUVIndex];
                    acBuffer[iIndex+1] = acTexture[iUVIndex+1];
                    acBuffer[iIndex+2] = acTexture[iUVIndex+2];
                    acBuffer[iIndex+3] = acTexture[iUVIndex+3];
                }   // if uv
			
            }   // if within device's width
		
        }   // for j = 0 to scanline width
	
    }   // for i = 0 to height
}

/*
**
*/
bool checkZBuffer( tVector4 const* pTopLeftFront, 
				   tVector4 const* pBottomRightBack,
				   tMatrix44 const* pViewProjMatrix,
				   float* afDepthBuffer,
                   unsigned char* acBuffer, 
				   int iBufferWidth, 
				   int iBufferHeight )
{   
#if defined( PROFILE )
    double fStartTime = getCurrTime();
#endif // PROFILE
    
    float fNearZ = 1.0f;
    
	// convert to view projection coordinates
	tVector4 viewProjTopLeftFront, viewProjBottomRightBack;
	Matrix44Transform( &viewProjTopLeftFront, pTopLeftFront, pViewProjMatrix );
	Matrix44Transform( &viewProjBottomRightBack, pBottomRightBack, pViewProjMatrix );
	
    if( viewProjTopLeftFront.fW <= fNearZ && viewProjBottomRightBack.fW <= fNearZ )
    {
        // back of the camera
        return true;
    }
    else if( viewProjTopLeftFront.fW <= fNearZ )
    {
        // top left front is behind camera and bottom right back is in front, just accept
        return false;
    }
    else if( viewProjBottomRightBack.fW <= fNearZ )
    {
        // bottom right back is behind camera and top left front is in front, just accept
        return false;
    }
    
	// view projection coordinate
    if( viewProjTopLeftFront.fW > 0.0f )
    {
        viewProjTopLeftFront.fX /= viewProjTopLeftFront.fW;
        viewProjTopLeftFront.fY /= viewProjTopLeftFront.fW;
        viewProjTopLeftFront.fZ /= viewProjTopLeftFront.fW;
    }
    
    if( viewProjBottomRightBack.fW > 0.0f )
    {
        viewProjBottomRightBack.fX /= viewProjBottomRightBack.fW;
        viewProjBottomRightBack.fY /= viewProjBottomRightBack.fW;
        viewProjBottomRightBack.fZ /= viewProjBottomRightBack.fW;
    }
    
	// center of the cube
	tVector4 center = 
	{
		( viewProjBottomRightBack.fX + viewProjTopLeftFront.fX ) * 0.5f,
		( viewProjBottomRightBack.fY + viewProjTopLeftFront.fY ) * 0.5f,
		( viewProjBottomRightBack.fZ + viewProjTopLeftFront.fZ ) * 0.5f,
		1.0f
	};

	// screen (logical) coordinate for center
	tVector2i centerScreen =
	{
		(short)( ( center.fX * (float)iBufferWidth + (float)iBufferWidth ) * 0.5f ),
		(short)( iBufferHeight - (int)( ( center.fY * (float)iBufferHeight + (float)iBufferHeight ) * 0.5f ) )
	};

	// get the extent 
	tVector4 diff;
	Vector4Subtract( &diff, &viewProjBottomRightBack, &viewProjTopLeftFront );

	diff.fX = fabs( diff.fX );
	diff.fY = fabs( diff.fY );
	diff.fZ = fabs( diff.fZ );

	// get the largest extent
	float fCubeSize = 0.0f;
	if( diff.fX >= diff.fY && diff.fX >= diff.fZ )
	{
		fCubeSize = diff.fX;
	}
	else if( diff.fY >= diff.fX && diff.fY >= diff.fZ )
	{
		fCubeSize = diff.fY;
	}
	else
	{
		fCubeSize = diff.fZ;
	}
	
	// smallest z to use
	float fZ = viewProjTopLeftFront.fZ;
	if( viewProjBottomRightBack.fZ < viewProjTopLeftFront.fZ )
	{
		fZ = viewProjBottomRightBack.fZ;
	}
	
	int iWidth = (int)ceilf( fCubeSize * (float)iBufferWidth );
	int iHeight = (int)ceilf( fCubeSize * (float)iBufferWidth );
	
	// check against z buffer
	bool bHidden = true;
	for( int iY = 0; iY < iHeight; iY++ )
	{
		int iCurrY = centerScreen.iY + ( iY - ( iHeight >> 1 ) );
        if( iCurrY < 0 || iCurrY >= iBufferHeight )
        {
            continue;
        }
        
		for( int iX = 0; iX < iWidth; iX++ )
		{
			int iCurrX = centerScreen.iX + ( iX - ( iWidth >> 1 ) );
            if( iCurrX < 0 || iCurrX >= iBufferWidth )
            {
                continue;
            }
            
			int iIndex = ( ( iCurrY * iBufferWidth + iCurrX ) << 2 );
            int iDepthIndex = ( iCurrY * iBufferWidth + iCurrX );
            
            WTFASSERT2( iIndex >= 0 && iIndex < iBufferWidth * iBufferHeight * 4, "depth color buffer out of bounds" );
            WTFASSERT2( iDepthIndex >= 0 && iDepthIndex < iBufferWidth * iBufferHeight, "depth buffer out of bounds" );
            
			//float fThisZ = (float)acBuffer[iIndex] / 255.0f;
			float fThisZ = afDepthBuffer[iDepthIndex];
            
            if( fThisZ == 0.0f || fZ < fThisZ )
			{
				bHidden = false;
                break;
			}
			
		}   // for x = 0 to width
        
        if( !bHidden )
        {
            break;
        }
        
	}   // for y = 0 to height

#if defined( PROFILE )
    double fElapsed = getCurrTime() - fStartTime;
    safProfileTimes[PROFILE_RASTER_CHECK_Z] += fElapsed;
#endif // PROFILE
    
    return bHidden;
}
