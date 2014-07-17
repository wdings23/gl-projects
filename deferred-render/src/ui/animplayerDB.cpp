#include "animplayerDB.h"

#define NUM_FRAMES_ALLOC 10

/*
**
*/
CAnimPlayerDB::CAnimPlayerDB( void )
{
	miNumScaleFrameAlloc = NUM_FRAMES_ALLOC;
	miNumScaleFrames = 0;
	maScaleFrames = (tVectorAnimFrame *)MALLOC( sizeof( tVectorAnimFrame ) * miNumScaleFrameAlloc );

	miNumRotationFrameAlloc = NUM_FRAMES_ALLOC;
	miNumRotationFrames = 0;
	maRotationFrames = (tVectorAnimFrame *)MALLOC( sizeof( tVectorAnimFrame ) * miNumRotationFrameAlloc );

	miNumPositionFrameAlloc = NUM_FRAMES_ALLOC;
	miNumPositionFrames = 0;
	maPositionFrames = (tVectorAnimFrame *)MALLOC( sizeof( tVectorAnimFrame ) * miNumPositionFrameAlloc );

	miNumAnchorPtFrameAlloc = NUM_FRAMES_ALLOC;
	miNumAnchorPtFrames = 0;
	maAnchorPtFrames = (tVectorAnimFrame *)MALLOC( sizeof( tVectorAnimFrame ) * miNumAnchorPtFrameAlloc );
	
	miNumColorFrameAlloc = NUM_FRAMES_ALLOC;
	miNumColorFrames = 0;
	maColorFrames = (tVectorAnimFrame *)MALLOC( sizeof( tVectorAnimFrame ) * miNumColorFrameAlloc );

	miNumActiveFrameAlloc = NUM_FRAMES_ALLOC;
	miNumActiveFrames = 0;
	maActiveFrames = (tVectorAnimFrame *)MALLOC( sizeof( tVectorAnimFrame ) * miNumActiveFrameAlloc );


	mfLastTime = 0.0f;

	mfStageWidth = 0.0f;
	mfStageHeight = 0.0f;
    
    memset( mszParticle, 0, sizeof( mszParticle ) );
    
    memset( mszMusic, 0, sizeof( mszMusic ) );
    memset( mszSound, 0, sizeof( mszSound ) );
    
    mfStartParticleTime = 0.0f;
}

/*
**
*/
CAnimPlayerDB::~CAnimPlayerDB( void )
{
	FREE( maScaleFrames );
	FREE( maRotationFrames );
	FREE( maPositionFrames );
	FREE( maAnchorPtFrames );
	FREE( maColorFrames );
}

/*
**
*/
void CAnimPlayerDB::getFrameAtTime( tVectorAnimFrame* pResult, int iType, float fTime )
{
	int iNumFrames = 0;
	tVectorAnimFrame* paVectorAnimFrames = NULL;
	
	switch( iType )
	{
		case FRAME_SCALE:
			paVectorAnimFrames = maScaleFrames;
			iNumFrames = miNumScaleFrames;
			break;
		case FRAME_ROTATION:
			paVectorAnimFrames = maRotationFrames;
			iNumFrames = miNumRotationFrames;
			break;
		case FRAME_POSITION:
			paVectorAnimFrames = maPositionFrames;
			iNumFrames = miNumPositionFrames;
			break;
		case FRAME_ANCHOR_PT:
			paVectorAnimFrames = maAnchorPtFrames;
			iNumFrames = miNumAnchorPtFrames;
			break;
		case FRAME_COLOR:
			paVectorAnimFrames = maColorFrames;
			iNumFrames = miNumColorFrames;
			break;
		case FRAME_ACTIVE:
			paVectorAnimFrames = maActiveFrames;
			iNumFrames = miNumActiveFrames;
			break;

	};

	// get the next keyframe from time
	int iNextFrame = 0;
	for( iNextFrame = 0; iNextFrame < iNumFrames; iNextFrame++ )
	{
		if( paVectorAnimFrames[iNextFrame].mfTime > fTime )
		{
			break;
		}
	}
	
	// previous and next frames
	tVectorAnimFrame* pPrevFrame = NULL;
	tVectorAnimFrame* pNextFrame = NULL;
	if( iNextFrame < iNumFrames )
	{
		pNextFrame = &paVectorAnimFrames[iNextFrame];
	}
	else
	{
		pNextFrame = &paVectorAnimFrames[iNextFrame-1];
	}

	if( iNextFrame > 0 )
	{
		pPrevFrame = &paVectorAnimFrames[iNextFrame-1];
	}
	else
	{
		pPrevFrame = pNextFrame;
	}

	// percentage in the interpolation
	float fPct = 0.0f;
	if( pNextFrame != pPrevFrame )
	{
		fPct = ( fTime - pPrevFrame->mfTime ) / ( pNextFrame->mfTime - pPrevFrame->mfTime );
	}
	
	// just need to if the frame is active by seeing if the previous frame is active
	if( iType == FRAME_ACTIVE )
	{
		pResult->mfTime = fTime;
		memcpy( &pResult->mValue, &pPrevFrame->mValue, sizeof( tVector4 ) );
	}
	else
	{
		pResult->mfTime = fTime;
		pResult->mValue.fX = pPrevFrame->mValue.fX + ( pNextFrame->mValue.fX - pPrevFrame->mValue.fX ) * fPct;
		pResult->mValue.fY = pPrevFrame->mValue.fY + ( pNextFrame->mValue.fY - pPrevFrame->mValue.fY ) * fPct;
		pResult->mValue.fZ = pPrevFrame->mValue.fZ + ( pNextFrame->mValue.fZ - pPrevFrame->mValue.fZ ) * fPct;
		pResult->mValue.fW = 1.0f;
	}
}

/*
**
*/
void CAnimPlayerDB::addFrame( tVectorAnimFrame* pFrame, int iType )
{
	switch( iType )
	{
		case FRAME_SCALE:
		{
			if( miNumScaleFrames + 1 >= miNumScaleFrameAlloc )
			{
				miNumScaleFrameAlloc += NUM_FRAMES_ALLOC;
				maScaleFrames = (tVectorAnimFrame *)REALLOC( maScaleFrames, sizeof( tVectorAnimFrame ) * miNumScaleFrameAlloc );
			}

			memcpy( &maScaleFrames[miNumScaleFrames++], pFrame, sizeof( tVectorAnimFrame ) );
		}
		break;

		case FRAME_ROTATION:
		{
			if( miNumRotationFrames + 1 >= miNumRotationFrameAlloc )
			{
				miNumRotationFrameAlloc += NUM_FRAMES_ALLOC;
				maRotationFrames = (tVectorAnimFrame *)REALLOC( maRotationFrames, sizeof( tVectorAnimFrame ) * miNumRotationFrameAlloc );
			}

			memcpy( &maRotationFrames[miNumRotationFrames++], pFrame, sizeof( tVectorAnimFrame ) );
		}
		break;

		case FRAME_POSITION:
		{
			if( miNumPositionFrames + 1 >= miNumPositionFrameAlloc )
			{
				miNumPositionFrameAlloc += NUM_FRAMES_ALLOC;
				maPositionFrames = (tVectorAnimFrame *)REALLOC( maPositionFrames, sizeof( tVectorAnimFrame ) * miNumPositionFrameAlloc );
			}

			memcpy( &maPositionFrames[miNumPositionFrames++], pFrame, sizeof( tVectorAnimFrame ) );
		}
		break;

		case FRAME_ANCHOR_PT:
		{
			if( miNumAnchorPtFrames + 1 >= miNumAnchorPtFrameAlloc )
			{
				miNumAnchorPtFrameAlloc += NUM_FRAMES_ALLOC;
				maAnchorPtFrames = (tVectorAnimFrame *)REALLOC( maAnchorPtFrames, sizeof( tVectorAnimFrame ) * miNumAnchorPtFrameAlloc );
			}

			memcpy( &maAnchorPtFrames[miNumAnchorPtFrames++], pFrame, sizeof( tVectorAnimFrame ) );
		}
		break;

		case FRAME_COLOR:
		{
			if( miNumColorFrames + 1 >= miNumColorFrameAlloc )
			{
				miNumColorFrameAlloc += NUM_FRAMES_ALLOC;
				maColorFrames = (tVectorAnimFrame *)REALLOC( maColorFrames, sizeof( tVectorAnimFrame ) * miNumColorFrameAlloc );
			}

			memcpy( &maColorFrames[miNumColorFrames++], pFrame, sizeof( tVectorAnimFrame ) );
		}
		break;

		case FRAME_ACTIVE:
		{
			if( miNumActiveFrames + 1 >= miNumActiveFrameAlloc )
			{
				miNumActiveFrameAlloc += NUM_FRAMES_ALLOC;
				maActiveFrames = (tVectorAnimFrame *)REALLOC( maActiveFrames, sizeof( tVectorAnimFrame ) * miNumActiveFrameAlloc );
			}

			memcpy( &maActiveFrames[miNumActiveFrames++], pFrame, sizeof( tVectorAnimFrame ) );
		}
		break;
	}

	if( iType != FRAME_ACTIVE && mfLastTime < pFrame->mfTime )
	{
		mfLastTime = pFrame->mfTime;
	}
}

/*
**
*/
void CAnimPlayerDB::setName( const char* szName )
{
	strncpy( mszName, szName, sizeof( mszName ) );
}

/*
**
*/
void CAnimPlayerDB::setParticle( const char* szName )
{
    strncpy( mszParticle, szName, sizeof( mszParticle ) );
}

/*
**
*/
void CAnimPlayerDB::load( TiXmlNode* pNode, const char* szFileName )
{
	while( pNode )
	{
		// skip comments
		while( pNode && pNode->Type() == TiXmlNode::TINYXML_COMMENT )
		{
			pNode = pNode->NextSibling();
		}
		
		if( pNode == NULL )
		{
			break;
		}
		
		const char* szValue = pNode->Value();
		if( !strcmp( szValue, "name" ) )
		{
			// filename to concat to the actual name of the animation for ID
			char szNoExtension[64];
			const char* szEnd = strstr( szFileName, "." );
			if( szEnd )
			{
				memset( szNoExtension, 0, sizeof( szNoExtension ) );
				memcpy( szNoExtension, szFileName, (size_t)szEnd - (size_t)szFileName );
			}
			else
			{
				strcpy( szNoExtension, szFileName );
			}

			const char* szName = pNode->FirstChild()->Value();
			sprintf( mszName, "%s/%s", szNoExtension, szName );
            
			OUTPUT( "%s : %d LOAD FROM %s mszName = %s\n",
					 __FUNCTION__,
					 __LINE__,
                     szFileName,
					 mszName );
		}
		else if( !strcmp( szValue, "positions" ) )
		{
			readKeyFrames( pNode, FRAME_POSITION );
		}
		else if( !strcmp( szValue, "rotations" ) )
		{
			readKeyFrames( pNode, FRAME_ROTATION );
		}
		else if( !strcmp( szValue, "scalings" ) )
		{
			readKeyFrames( pNode, FRAME_SCALE );
		}
		else if( !strcmp( szValue, "anchorPoints" ) )
		{
			readKeyFrames( pNode, FRAME_ANCHOR_PT );
		}
		else if( !strcmp( szValue, "opacities" ) )
		{
			readKeyFrames( pNode, FRAME_COLOR );
		}
		else if( !strcmp( szValue, "activities" ) )
		{
			readKeyFrames( pNode, FRAME_ACTIVE );
		}
        else if( !strcmp( szValue, "sound" ) )
        {
            setSound( pNode->FirstChild()->Value() );
        }
        else if( !strcmp( szValue, "music" ) )
        {
            setMusic( pNode->FirstChild()->Value() );
        }
        else if( !strcmp( szValue, "particle" ) )
        {
            setParticle( pNode->FirstChild()->Value() );
        }
        else if( !strcmp( szValue, "particle_start" ) )
        {
            float fTime = atof( pNode->FirstChild()->Value() );
            setParticleStartTime( fTime );
        }

		pNode = pNode->NextSibling();
	}
    
    // get the last active time for particle
    if( strlen( mszParticle ) )
    {
        if( mfLastTime < maActiveFrames[miNumActiveFrames-1].mfTime )
        {
            mfLastTime = maActiveFrames[miNumActiveFrames-1].mfTime;
        }
    }
}

/*
**
*/
void CAnimPlayerDB::parseValue( tVector4* pResult, const char* szValue )
{
	char szOutput[256];
	strncpy( szOutput, szValue, sizeof( szOutput ) );

	memset( pResult, 0, sizeof( tVector4 ) );

	const char* szToken = strtok( szOutput, "," );
	assert( szToken );
	if( !strcmp( szToken, "true" ) )
	{
		pResult->fX = 1.0f;
	}
	else if( !strcmp( szToken, "false" ) )
	{
		pResult->fX = 0.0f;
	}
	else
	{
		pResult->fX = atof( szToken );
	}

	szToken = strtok( NULL, "," );
	if( szToken )
	{
		if( !strcmp( szToken, "true" ) )
		{
			pResult->fY = 1.0f;
		}
		else if( !strcmp( szToken, "false" ) )
		{
			pResult->fY = 0.0f;
		}
		else
		{
			pResult->fY = atof( szToken );
		}
	}
}

/*
**
*/
void CAnimPlayerDB::readKeyFrames( TiXmlNode* pChild, int iType )
{
	TiXmlNode* pKeyFrame = pChild->FirstChild();
	while( pKeyFrame )
	{
		TiXmlNode* pTime = pKeyFrame->FirstChild();
		TiXmlNode* pValue = pTime->NextSibling();

		tVectorAnimFrame animFrame;

		animFrame.mfTime = atof( pTime->FirstChild()->Value() );
		const char* szValue = pValue->FirstChild()->Value();
		
		parseValue( &animFrame.mValue, szValue );
		addFrame( &animFrame, iType );

		pKeyFrame = pKeyFrame->NextSibling();
	}
}

/*
**
*/
void CAnimPlayerDB::setSound( const char* szSound )
{
    strncpy( mszSound, szSound, sizeof( mszSound ) );
}

/*
**
*/
void CAnimPlayerDB::setMusic( const char* szMusic )
{
    strncpy( mszMusic, szMusic, sizeof( mszMusic ) );
}
