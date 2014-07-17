#include "modelkeyanim.h"
#include "tinyxml.h"
#include "filepathutil.h"

#define NUM_KEYS_PER_ALLOC	100
#define FRAMES_PER_SECOND 24.0

enum 
{
	FRAMETYPE_VISIBILITY = 0,
	
	FRAMETYPE_TRANSLATION_X,
	FRAMETYPE_TRANSLATION_Y,
	FRAMETYPE_TRANSLATION_Z,
	
	FRAMETYPE_ROTATION_X,
	FRAMETYPE_ROTATION_Y,
	FRAMETYPE_ROTATION_Z,
	
	FRAMETYPE_SCALING_X,
	FRAMETYPE_SCALING_Y,
	FRAMETYPE_SCALING_Z,

	NUM_FRAMETYPES,
};

static void loadInFrames( tModelKeyAnim* pAnim, TiXmlNode* pNode, int iType );

/*
**
*/
void modelKeyAnimInit( tModelKeyAnim* pAnim )
{
	pAnim->miNumKeyFrames = 0;
	pAnim->miNumKeyFrameAlloc = NUM_KEYS_PER_ALLOC;
	pAnim->maKeyFrames = (tModelKeyFrame *)malloc( sizeof( tModelKeyAnim ) * pAnim->miNumKeyFrameAlloc );
	memset( pAnim->maKeyFrames, 0, sizeof( tModelKeyAnim ) * pAnim->miNumKeyFrameAlloc );
}

/*
**
*/
void modelKeyAnimRelease( tModelKeyAnim* pAnim )
{
	pAnim->miNumKeyFrames = 0;
	pAnim->miNumKeyFrameAlloc = 0;
	free( pAnim->maKeyFrames );
}

/*
**
*/
void modelKeyAnimLoad( tModelKeyAnim* pAnim, const char* szFileName )
{
	char szFullPath[256];
	getFullPath( szFullPath, szFileName );

	TiXmlDocument doc( szFullPath );
	doc.LoadFile();
	TiXmlNode* pNode = doc.FirstChild()->FirstChild();

	while( pNode )
	{
		const char* szVal = pNode->Value();
		if( !strcmp( szVal, "visibility" ) )
		{
			loadInFrames( pAnim, pNode, FRAMETYPE_VISIBILITY );
		}
		else if( !strcmp( szVal, "translateX" ) )
		{
			loadInFrames( pAnim, pNode, FRAMETYPE_TRANSLATION_X );
		}
		else if( !strcmp( szVal, "translateY" ) )
		{
			loadInFrames( pAnim, pNode, FRAMETYPE_TRANSLATION_Y );
		}
		else if( !strcmp( szVal, "translateZ" ) )
		{
			loadInFrames( pAnim, pNode, FRAMETYPE_TRANSLATION_Z );
		}
		else if( !strcmp( szVal, "rotateX" ) )
		{
			loadInFrames( pAnim, pNode, FRAMETYPE_ROTATION_X );
		}
		else if( !strcmp( szVal, "rotateY" ) )
		{
			loadInFrames( pAnim, pNode, FRAMETYPE_ROTATION_Y );
		}
		else if( !strcmp( szVal, "rotateZ" ) )
		{
			loadInFrames( pAnim, pNode, FRAMETYPE_ROTATION_Z );
		}
		else if( !strcmp( szVal, "scaleX" ) )
		{
			loadInFrames( pAnim, pNode, FRAMETYPE_SCALING_X );
		}
		else if( !strcmp( szVal, "scaleY" ) )
		{
			loadInFrames( pAnim, pNode, FRAMETYPE_SCALING_Y );
		}
		else if( !strcmp( szVal, "scaleZ" ) )
		{
			loadInFrames( pAnim, pNode, FRAMETYPE_SCALING_Z );
		}

		pNode = pNode->NextSibling();
	}
}

/*
**
*/
void modelKeyAnimGetKeyAtTime( tModelKeyAnim const* pAnim, float fTime, tModelKeyFrame* pResult )
{
	tModelKeyFrame* pNextKeyFrame = NULL;
	tModelKeyFrame* pPrevKeyFrame = NULL;

	if( fTime >= pAnim->maKeyFrames[pAnim->miNumKeyFrames-1].mfTime )
	{
		memcpy( pResult, &pAnim->maKeyFrames[pAnim->miNumKeyFrames-1], sizeof( tModelKeyFrame ) );
		return;
	}

	if( fTime <= pAnim->maKeyFrames[0].mfTime )
	{
		memcpy( pResult, &pAnim->maKeyFrames[0], sizeof( tModelKeyFrame ) );
		return;
	}
	
	// find prev and next frame
	for( int i = 0; i < pAnim->miNumKeyFrames; i++ )
	{
		if( pAnim->maKeyFrames[i].mfTime > fTime )
		{
			pNextKeyFrame = &pAnim->maKeyFrames[i];

			if( i - 1 >= 0 )
			{
				pPrevKeyFrame = &pAnim->maKeyFrames[i-1];
			}
			else
			{
				pPrevKeyFrame = &pAnim->maKeyFrames[i];
			}

			break;
		}

	}	// for i = 0 to num keyframes

	float fFrameTimeDiff = pNextKeyFrame->mfTime - pPrevKeyFrame->mfTime;
	float fCurrTimeDiff = pNextKeyFrame->mfTime - fTime;

	float fPct = 0.0f;
	if( fFrameTimeDiff > 0.0f )
	{
		fPct = 1.0f - fCurrTimeDiff / fFrameTimeDiff;
	}
	
	pResult->mfTime = fTime;
	pResult->mfVisibility = pPrevKeyFrame->mfVisibility + ( pNextKeyFrame->mfVisibility - pPrevKeyFrame->mfVisibility ) * fPct;
	
	pResult->mTranslation.fX = pPrevKeyFrame->mTranslation.fX + ( pNextKeyFrame->mTranslation.fX - pPrevKeyFrame->mTranslation.fX ) * fPct;
	pResult->mTranslation.fY = pPrevKeyFrame->mTranslation.fY + ( pNextKeyFrame->mTranslation.fY - pPrevKeyFrame->mTranslation.fY ) * fPct;
	pResult->mTranslation.fZ = pPrevKeyFrame->mTranslation.fZ + ( pNextKeyFrame->mTranslation.fZ - pPrevKeyFrame->mTranslation.fZ ) * fPct;
	pResult->mTranslation.fW = 1.0f;

	pResult->mRotation.fX = pPrevKeyFrame->mRotation.fX + ( pNextKeyFrame->mRotation.fX - pPrevKeyFrame->mRotation.fX ) * fPct;
	pResult->mRotation.fY = pPrevKeyFrame->mRotation.fY + ( pNextKeyFrame->mRotation.fY - pPrevKeyFrame->mRotation.fY ) * fPct;
	pResult->mRotation.fZ = pPrevKeyFrame->mRotation.fZ + ( pNextKeyFrame->mRotation.fZ - pPrevKeyFrame->mRotation.fZ ) * fPct;
	pResult->mRotation.fW = 1.0f;

	pResult->mScaling.fX = pPrevKeyFrame->mScaling.fX + ( pNextKeyFrame->mScaling.fX - pPrevKeyFrame->mScaling.fX ) * fPct;
	pResult->mScaling.fY = pPrevKeyFrame->mScaling.fY + ( pNextKeyFrame->mScaling.fY - pPrevKeyFrame->mScaling.fY ) * fPct;
	pResult->mScaling.fZ = pPrevKeyFrame->mScaling.fZ + ( pNextKeyFrame->mScaling.fZ - pPrevKeyFrame->mScaling.fZ ) * fPct;
	pResult->mScaling.fW = 1.0f;

}

/*
**
*/
/*
**
*/
static void loadInFrames( tModelKeyAnim* pAnim, TiXmlNode* pNode, int iType )
{
	tModelKeyFrame* pFrame = &pAnim->maKeyFrames[0];
	
	int iFrames = 0;
	TiXmlNode* pChild = pNode->FirstChild();
	while( pChild )
	{
		// time
		float fTime = (float)atof( pChild->FirstChild()->Value() );
		pChild = pChild->NextSibling();
		
		float fValue = (float)atof( pChild->FirstChild()->Value() );
		
		// type
		if( iType == FRAMETYPE_VISIBILITY )
		{
			pFrame->mfVisibility = fValue;
		}
		else if( iType == FRAMETYPE_TRANSLATION_X )
		{
			pFrame->mTranslation.fX = fValue;
			pFrame->mTranslation.fW = 1.0f;
		}
		else if( iType == FRAMETYPE_TRANSLATION_Y )
		{
			pFrame->mTranslation.fY = fValue;
			pFrame->mTranslation.fW = 1.0f;
		}
		else if( iType == FRAMETYPE_TRANSLATION_Z )
		{
			pFrame->mTranslation.fZ = fValue;
			pFrame->mTranslation.fW = 1.0f;
		}
		else if( iType == FRAMETYPE_ROTATION_X )
		{
			pFrame->mRotation.fX = fValue * ( 3.14159f / 180.0f );
			pFrame->mRotation.fW = 1.0f;
		}
		else if( iType == FRAMETYPE_ROTATION_Y )
		{
			pFrame->mRotation.fY = fValue * ( 3.14159f / 180.0f );
			pFrame->mRotation.fW = 1.0f;
		}
		else if( iType == FRAMETYPE_ROTATION_Z )
		{
			pFrame->mRotation.fZ = fValue * ( 3.14159f / 180.0f );
			pFrame->mRotation.fW = 1.0f;
		}
		else if( iType == FRAMETYPE_SCALING_X )
		{
			pFrame->mScaling.fX = fValue;
			pFrame->mScaling.fW = 1.0f;
		}
		else if( iType == FRAMETYPE_SCALING_Y )
		{
			pFrame->mScaling.fY = fValue;
			pFrame->mScaling.fW = 1.0f;
		}
		else if( iType == FRAMETYPE_SCALING_Z )
		{
			pFrame->mScaling.fZ = fValue;
			pFrame->mScaling.fW = 1.0f;
		}
		
		pFrame->mfTime = ( fTime - 1.0f ) / FRAMES_PER_SECOND;
		
		// more space needed
		if( iFrames + 1 >= pAnim->miNumKeyFrameAlloc )
		{
			pAnim->miNumKeyFrameAlloc += NUM_KEYS_PER_ALLOC;
			pAnim->maKeyFrames = (tModelKeyFrame *)realloc( pAnim->maKeyFrames, sizeof( tModelKeyFrame ) * pAnim->miNumKeyFrameAlloc );
		}

		if( iFrames + 1 > pAnim->miNumKeyFrames )
		{
			pAnim->miNumKeyFrames = iFrames + 1;
		}

		++pFrame;
		++iFrames;

		pChild = pChild->NextSibling();
	}
}