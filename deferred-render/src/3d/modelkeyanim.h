#ifndef __MODELKEYANIM_H__
#define __MODELKEYANIM_H__

#include "vector.h"

struct ModelKeyFrame
{
	float				mfTime;
	float				mfVisibility;
	tVector4			mTranslation;
	tVector4			mRotation;
	tVector4			mScaling;
};

typedef struct ModelKeyFrame tModelKeyFrame;

struct ModelKeyAnim
{
	tModelKeyFrame*			maKeyFrames;
	int						miNumKeyFrames;
	int						miNumKeyFrameAlloc;

};

typedef struct ModelKeyAnim tModelKeyAnim;

void modelKeyAnimInit( tModelKeyAnim* pAnim );
void modelKeyAnimRelease( tModelKeyAnim* pAnim );
void modelKeyAnimLoad( tModelKeyAnim* pAnim, const char* szFileName );
void modelKeyAnimGetKeyAtTime( tModelKeyAnim const* pAnim, float fTime, tModelKeyFrame* pResult );

#endif // __MODELKEYANIM_H__