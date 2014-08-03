#include "game.h"
#include "render.h"
#include "jobmanager.h"

/*
**
*/
static tVector4 evalSH( tVector4 const* pDirection )
{
	tVector2 SHCoeff = { 1.0f / ( 2.0f * sqrtf( PI ) ), sqrtf( 3.0f ) / ( 2.0f * sqrtf( PI ) ) };

	tVector4 ret =
	{
		SHCoeff.fX * pDirection->fW,
		-SHCoeff.fY * pDirection->fY,
		SHCoeff.fY * pDirection->fZ,
		-SHCoeff.fY * pDirection->fX
	};

	return ret;
}

/*
**
*/
static tVector4 evalCosineLobe( tVector4 const* pDirection )
{
	tVector2 SHCosLobe = { sqrtf( PI ) * 0.5f, sqrtf( PI * 0.33333f ) };

	tVector4 ret = 
	{
		SHCosLobe.fX * pDirection->fW, 
		-SHCosLobe.fY * pDirection->fY,
		SHCosLobe.fY * pDirection->fZ,
		-SHCosLobe.fY * pDirection->fX,
	};

	return ret;
}

/*
**
*/
static void testLPV( void )
{
	tVector4 aVolumeColors[] =
	{
		{ 1.0f, 0.0f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f, 1.0f },
	};

	// floor surrounded by two walls
	tVector4 aVolumeNormals[] =
	{
		{ 1.0f, -1.0f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f, 1.0f },
		{ -1.0f, -1.0f, 0.0f, 1.0f },
	};

	for( int i = 0; i < 3; i++ )
	{
		Vector4Normalize( &aVolumeNormals[i], &aVolumeNormals[i] );
	}

	// color from RSM encoded in spherical harmonics
	tVector3 aColorSH[3][3];

	int iNumVolumes = sizeof( aVolumeColors ) / sizeof( *aVolumeColors );

	// initial flux (Reflection Shadow Map injection)
	float fOneOverPI = 1.0f / PI;
	for( int i = 0; i < iNumVolumes; i++ )
	{
		tVector4 const* pVolumeColor = &aVolumeColors[i];

		tVector4 SHCoeff = evalCosineLobe( &aVolumeNormals[i] );
		SHCoeff.fX *= fOneOverPI;
		SHCoeff.fY *= fOneOverPI;
		SHCoeff.fZ *= fOneOverPI;
		SHCoeff.fW *= fOneOverPI;

		printf( "SHCoeff %d = ( %f, %f, %f, %f )\n", 
				i,
				SHCoeff.fX,
				SHCoeff.fY,
				SHCoeff.fZ,
				SHCoeff.fW );

		tVector3* pColorSH0 = &aColorSH[i][0];
		tVector3* pColorSH1 = &aColorSH[i][1];
		tVector3* pColorSH2 = &aColorSH[i][2];

		pColorSH0->fX = pVolumeColor->fX * SHCoeff.fX; 
		pColorSH0->fY = pVolumeColor->fX * SHCoeff.fY; 
		pColorSH0->fZ = pVolumeColor->fX * SHCoeff.fZ;
		
		pColorSH1->fX = pVolumeColor->fY * SHCoeff.fX; 
		pColorSH1->fY = pVolumeColor->fY * SHCoeff.fY; 
		pColorSH1->fZ = pVolumeColor->fY * SHCoeff.fZ;
		
		pColorSH2->fX = pVolumeColor->fZ * SHCoeff.fX; 
		pColorSH2->fY = pVolumeColor->fZ * SHCoeff.fY; 
		pColorSH2->fZ = pVolumeColor->fZ * SHCoeff.fZ;

	}	// for i = 0 to num volumes

	// contributes in spherical harmonics
	tVector4 aSHContributions[3][3];
	memset( aSHContributions, 0, sizeof( aSHContributions ) );
	
	// angle from center of current volume to the neighbor's volume
	const float fDirectFaceSubtendedSolidAngle = 0.4006696846f / PI;
	const float fSideFaceSubtendedSolidAngle = 0.4234413544f / PI;

	// propagation
	for( int i = 0; i < iNumVolumes; i++ )
	{
		for( int iNeighbor = i - 1; iNeighbor <= i + 1; iNeighbor++ )
		{
			if( iNeighbor < 0 || iNeighbor == i || iNeighbor >= iNumVolumes )
			{
				continue;
			}

			// direction
			float fDirX = (float)( iNeighbor - i );
			tVector4 directionToNeighbor = { fDirX, 0.0f, 0.0f, 1.0f };

			// neighbor's color coefficient
			tVector3 const* aNeighborColorSH = aColorSH[iNeighbor];

			// neighbor direction in SH
			tVector4 directionToNeighborSH = evalSH( &directionToNeighbor );
			tVector4 directionToNeighborCosLobe = evalCosineLobe( &directionToNeighbor );

			// color sh coefficient dot with direction 
			float fDPRed = aNeighborColorSH[0].fX * directionToNeighborSH.fX + 
						   aNeighborColorSH[0].fY * directionToNeighborSH.fY + 
						   aNeighborColorSH[0].fZ * directionToNeighborSH.fZ;

			float fDPGreen = aNeighborColorSH[1].fX * directionToNeighborSH.fX + 
							 aNeighborColorSH[1].fY * directionToNeighborSH.fY + 
							 aNeighborColorSH[1].fZ * directionToNeighborSH.fZ;

			float fDPBlue = aNeighborColorSH[2].fX * directionToNeighborSH.fX + 
						    aNeighborColorSH[2].fY * directionToNeighborSH.fY + 
						    aNeighborColorSH[2].fZ * directionToNeighborSH.fZ;
			
			float fRedContribMult = fDirectFaceSubtendedSolidAngle * fDPRed;  
			float fGreenContribMult = fDirectFaceSubtendedSolidAngle * fDPGreen;  
			float fBlueContribMult = fDirectFaceSubtendedSolidAngle * fDPBlue;

			// add up contributions
			tVector4* pContrib = &aSHContributions[i][0];
			pContrib->fX += directionToNeighborCosLobe.fX * fRedContribMult;
			pContrib->fY += directionToNeighborCosLobe.fY * fRedContribMult;
			pContrib->fZ += directionToNeighborCosLobe.fZ * fRedContribMult;
			pContrib->fW += directionToNeighborCosLobe.fW * fRedContribMult;

			pContrib = &aSHContributions[i][1];
			pContrib->fX += directionToNeighborCosLobe.fX * fGreenContribMult;
			pContrib->fY += directionToNeighborCosLobe.fY * fGreenContribMult;
			pContrib->fZ += directionToNeighborCosLobe.fZ * fGreenContribMult;
			pContrib->fW += directionToNeighborCosLobe.fW * fGreenContribMult;

			pContrib = &aSHContributions[i][2];
			pContrib->fX += directionToNeighborCosLobe.fX * fBlueContribMult;
			pContrib->fY += directionToNeighborCosLobe.fY * fBlueContribMult;
			pContrib->fZ += directionToNeighborCosLobe.fZ * fBlueContribMult;
			pContrib->fW += directionToNeighborCosLobe.fW * fBlueContribMult;

			tVector3 center = { 0.0f, 0.0f, 0.0f };
			tVector3 aSides[] = 
			{
				{ -0.5f + fDirX, 0.0f, 0.0f },		// left
				{ 0.5 + fDirX, 0.0f, 0.0f },		// right
				{ fDirX, 0.0f, -0.5f },		// front
				{ fDirX, 0.0f, 0.5f },		// back
			};

			tMatrix44 rotYNeg, rotYPlus;
			Matrix44RotateY( &rotYNeg, -PI * 0.25f );
			Matrix44RotateY( &rotYPlus, PI * 0.25f );

			// sides of the neighbor's volume
			for( int iSide = 0; iSide < 4; iSide++ )
			{
				tVector3 const* pSide = &aSides[iSide];
				tVector4 dir = 
				{
					pSide->fX,
					pSide->fY,
					pSide->fZ,
					1.0f
				};

				Vector4Normalize( &dir, &dir );

				tVector4 side0 = { 0.0f, 0.0f, 0.0f, 1.0f }; 
				tVector4 side1 = { 0.0f, 0.0f, 0.0f, 1.0f };

				Matrix44Transform( &side0, &dir, &rotYNeg );
				Matrix44Transform( &side1, &dir, &rotYPlus );

				Vector4Normalize( &side0, &side0 );
				Vector4Normalize( &side1, &side1 );

				tVector4 directionToSideSH = evalSH( &dir );
				tVector4 directionToSideCosLobe = evalCosineLobe( &dir );

				// color sh coefficient dot with direction 
				float fDPRed = aNeighborColorSH[0].fX * directionToSideSH.fX + 
							   aNeighborColorSH[0].fY * directionToSideSH.fY + 
							   aNeighborColorSH[0].fZ * directionToSideSH.fZ;

				float fDPGreen = aNeighborColorSH[1].fX * directionToSideSH.fX + 
								 aNeighborColorSH[1].fY * directionToSideSH.fY + 
								 aNeighborColorSH[1].fZ * directionToSideSH.fZ;

				float fDPBlue = aNeighborColorSH[2].fX * directionToSideSH.fX + 
								aNeighborColorSH[2].fY * directionToSideSH.fY + 
								aNeighborColorSH[2].fZ * directionToSideSH.fZ;
			
				float fRedContribMult = fDirectFaceSubtendedSolidAngle * fDPRed;  
				float fGreenContribMult = fDirectFaceSubtendedSolidAngle * fDPGreen;  
				float fBlueContribMult = fDirectFaceSubtendedSolidAngle * fDPBlue;

				// add up contribution
				// red
				tVector4* pContrib = &aSHContributions[i][0];
				pContrib->fX += directionToSideCosLobe.fX * fRedContribMult;
				pContrib->fY += directionToSideCosLobe.fY * fRedContribMult;
				pContrib->fZ += directionToSideCosLobe.fZ * fRedContribMult;
				pContrib->fW += directionToSideCosLobe.fW * fRedContribMult;

				// green
				pContrib = &aSHContributions[i][1];
				pContrib->fX += directionToSideCosLobe.fX * fGreenContribMult;
				pContrib->fY += directionToSideCosLobe.fY * fGreenContribMult;
				pContrib->fZ += directionToSideCosLobe.fZ * fGreenContribMult;
				pContrib->fW += directionToSideCosLobe.fW * fGreenContribMult;

				// blue
				pContrib = &aSHContributions[i][2];
				pContrib->fX += directionToSideCosLobe.fX * fBlueContribMult;
				pContrib->fY += directionToSideCosLobe.fY * fBlueContribMult;
				pContrib->fZ += directionToSideCosLobe.fZ * fBlueContribMult;
				pContrib->fW += directionToSideCosLobe.fW * fBlueContribMult;
				
			}	// for side = 0 to 4

		}	// for neighbor = 0 to num volumes

	}	// for i = 0 to num volumes

	tVector3 aColors[] = 
	{
		{ 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f },
	};
	
	// color = ( SH( -normal ) . contribution )
	for( int i = 0; i < 3; i++ )
	{
		tVector4 negNormal = 
		{
			aVolumeNormals[i].fX,
			aVolumeNormals[i].fY,
			aVolumeNormals[i].fZ,
			1.0f,
		};

		tVector4 normalSH = evalSH( &negNormal );

		aColors[i].fX = aSHContributions[i][0].fX * normalSH.fX + aSHContributions[i][0].fY * normalSH.fY + aSHContributions[i][0].fZ * normalSH.fZ + aSHContributions[i][0].fW * normalSH.fW;
		aColors[i].fY = aSHContributions[i][1].fX * normalSH.fX + aSHContributions[i][1].fY * normalSH.fY + aSHContributions[i][1].fZ * normalSH.fZ + aSHContributions[i][1].fW * normalSH.fW;
		aColors[i].fZ = aSHContributions[i][2].fX * normalSH.fX + aSHContributions[i][2].fY * normalSH.fY + aSHContributions[i][2].fZ * normalSH.fZ + aSHContributions[i][2].fW * normalSH.fW;

	}

	for( int i = 0; i < 3; i++ )
	{
		for( int j = 0; j < 3; j++ )
		{
			printf( "SH %d %d ( %f, %f, %f, %f )\n",
					i,
					j,
					aSHContributions[i][j].fX,
					aSHContributions[i][j].fY,
					aSHContributions[i][j].fZ );
		}

		printf( "\n" );
	}

	for( int i = 0; i < 3; i++ )
	{
		printf( "color %d ( %f, %f, %f )\n",
				i,
				aColors[i].fX,
				aColors[i].fY,
				aColors[i].fZ );
	}
}

/*
**
*/
CGame* CGame::mpInstance = NULL;
CGame* CGame::instance( void )
{
	if( mpInstance == NULL )
	{
		mpInstance = new CGame();
	}

	return mpInstance;
}

/*
**
*/
CGame::CGame( void ) :
	mbVRView(false)
{
}

/*
**
*/
CGame::~CGame( void )
{
	
}

/*
**
*/
void CGame::init( void )
{
testLPV();

	levelInit( &mLevel );

	miScreenWidth = renderGetScreenWidth();
	miScreenHeight = renderGetScreenHeight();

	tVector4 pos = { 0.0f, 0.0f, -12.0f, 1.0f };
	tVector4 lookAt = { 0.0f, 0.0f, 100.0f, 1.0f };
	tVector4 up = { 0.0f, 1.0f, 0.0f, 1.0f };

	mCamera.setPosition( &pos );
	mCamera.setLookAt( &lookAt );
	mCamera.setFar( 100.0f );
	mCamera.setUp( &up );

	mGameRender.setCamera( &mCamera );
	mGameRender.setLevel( &mLevel );
	mGameRender.init();

	jobManagerInit( gpJobManager );

	mLastTouch.fX = mLastTouch.fY = -1.0f;
}

/*
**
*/
void CGame::update( float fDT )
{
    jobManagerUpdate( gpJobManager );
    
	mCamera.update( miScreenWidth, miScreenHeight );
	mGameRender.updateLevel();

	mfDT = fDT;
}

/*
**
*/
void CGame::draw( void )
{
	mGameRender.draw( mfDT );
}

/*
**
*/
void CGame::inputUpdate( float fX, float fY, int iType )
{
	if( iType == TOUCHTYPE_BEGAN || iType == TOUCHTYPE_ENDED )
	{
		mLastTouch.fX = fX;
		mLastTouch.fY = fY;
	}

    if( mLastTouch.fX < 0.0f || mLastTouch.fY < 0.0f )
    {
        mLastTouch.fX = fX;
        mLastTouch.fY = fY;
    }
    
	tVector4 diff = 
	{
		fX - mLastTouch.fX,
		fY - mLastTouch.fY
	};

	tVector4 const* pCamPos = mCamera.getPosition();
	tVector4 const* pCamLookAt = mCamera.getLookAt();

	tVector4 newCamPos = 
	{
		pCamPos->fX + diff.fX * 0.01f,
		pCamPos->fY + diff.fY * 0.01f,
		pCamPos->fZ + diff.fZ * 0.01f,
		1.0f
	};

	tVector4 newCamLookAt = 
	{
		pCamLookAt->fX + diff.fX * 0.01f,
		pCamLookAt->fY + diff.fY * 0.01f,
		pCamLookAt->fZ + diff.fZ * 0.01f,
		1.0f
	};
	
	mCamera.setPosition( &newCamPos );
	mCamera.setLookAt( &newCamLookAt );

	mLastTouch.fX = fX;
	mLastTouch.fY = fY;
}

/*
**
*/
void CGame::setSceneFileName( const char* szSceneFileName )
{
	strncpy( mszSceneFileName, szSceneFileName, sizeof( mszSceneFileName ) );
	mGameRender.setSceneFileName( mszSceneFileName );
}

/*
**
*/
void CGame::toggleShader( void )
{
	mGameRender.toggleShader();
}

/*
**
*/
void CGame::zoomCamera( float fDPos )
{
	tVector4 const* pPos = mCamera.getPosition();
	tVector4 const* pLookAt = mCamera.getLookAt();

	tVector4 newPos, newLookAt;
	memcpy( &newPos, pPos, sizeof( tVector4 ) );
	memcpy( &newLookAt, pLookAt, sizeof( tVector4 ) );

	newPos.fZ += fDPos;
	newLookAt.fZ += fDPos;

	mCamera.setPosition( &newPos );
	mCamera.setLookAt( &newLookAt );
}

/*
**
*/
void CGame::tiltCamera( float fDPos )
{
	tVector4 const* pPos = mCamera.getPosition();
	tVector4 const* pLookAt = mCamera.getLookAt();

	tVector4 newPos, newLookAt;
	memcpy( &newPos, pPos, sizeof( tVector4 ) );
	memcpy( &newLookAt, pLookAt, sizeof( tVector4 ) );

	newLookAt.fY += fDPos;

	mCamera.setPosition( &newPos );
	mCamera.setLookAt( &newLookAt );
}