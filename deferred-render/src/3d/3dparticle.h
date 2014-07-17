#ifndef __3DPARTICLE_H__
#define __3DPARTICLE_H__

#include "vector.h"
#include "camera.h"
#include "shadermanager.h"

enum
{
	PARTICLE_DEAD = 0,
	PARTICLE_ALIVE,

	NUM_PARTICLE_STATES,
};

enum
{
	EMITTER_STOPPED = 0,
	EMITTER_ACTIVE,

	NUM_EMITTER_STATES,
};

enum
{
    EMITTER_TYPE_REGULAR = 0,
    EMITTER_TYPE_RADIAL,
    
    NUM_EMITTER_TYPES,
};

struct Particle3D
{
	tVector4				mPosition;
	tVector4				mAngle;
	tVector4				mDirection;
	tVector4				mColor;
    tVector4                mStartAngle;
    
	tVector4				mSize;
	tVector4				mSizeInc;

	float					mfSpeed;

	float					mfStartTime;
	tVector4				mCurrDirection;
	int						miState;

	tVector4				mXFormPos;
    
    int                     miIndex;

};

typedef struct Particle3D tParticle3D;

struct Emitter3D
{
    int                     miType;
	int						miState;

	tVector4				mSourcePos;
	tVector4				mDirection;
	tVector4				mParticleSize;
	
	float					mfDuration;

	float					mfStartTime;
	float					mfParticleSpeed;
	float					mfParticleRotationSpeed;

	tParticle3D*			maParticles;
	float					mfTimeBetweenParticles;
	
	tVector4				mParticlePositionRanges;
	tVector4				mParticleAngleRanges;
	tVector4				mParticleSizeRanges;

    tVector4                mEmitterAngleRanges;
    
    tVector4                mParticleAngleStart;
    tVector4                mParticleAngleEnd;
    
	tVector4				mGravity;
	
	float					mfLastSpawnedParticleTime;
	float					mfParticleDuration;

	float					mfAngleVarianceY;

	int						miNumParticles;
	int						miMaxParticles;
    
	tVector4				mColorStart;
	tVector4				mColorEnd;

	tVector4				mParticleSizeStart;
	tVector4				mParticleSizeEnd;

    bool                    mbCollision;
    
	float					mfCurrTime;
	char					mszTextureName[256];

	GLint					miSrcBlend;
	GLint					miDestBlend;
    
	float					mfParticleSizeScale;
	float					mfParticleSpeedScale;

    // for radial
    float                   mfAnglePerParticle;
    int                     miCurrParticle;
    tVector4                mStartRadialRotation;
    float                   mfStartRadius;
    float                   mfEndRadius;
    tVector4                mAnimRadialRotation;
    tVector4                mCurrRadialAnimRotation;
    int						miNumParticlesPerEmit;
	float					mfTimeBetweenEmit;
    
};

typedef struct Emitter3D tEmitter3D;

void emitter3DInit( tEmitter3D* pEmitter );
void emitter3DStart( tEmitter3D* pEmitter );
void emitter3DStop( tEmitter3D* pEmitter );
void emitter3DLoad( tEmitter3D* pEmitter, const char* szFileName );
void emitter3DUpdate( tEmitter3D* pEmitter, float fDT );
void emitter3DDraw( tEmitter3D* pEmitter, 
				    CCamera const* pCamera, 
					unsigned int iShader, 
					GLuint iSrcBlend, 
					GLuint iDestBlend );

#endif // __3DPARTICLE_H__