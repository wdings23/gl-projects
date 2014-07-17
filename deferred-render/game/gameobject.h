#ifndef __GAMEOBJECT_H__
#define __GAMEOBJECT_H__

struct GameObject
{
	tVector4			mPosition;
	tVector4			mDimension;
	tVector4			mVelocity;

	float				mfScale;
	float				mfAngle;

	int					miState;
	int					miPrevState;

	int					miType;

	double				mfStateStartTime;
};

typedef struct GameObject	tGameObject;

#endif // __GAMEOBJECT_H__