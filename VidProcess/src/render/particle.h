//
//  particle.h
//  CityBenchmark
//
//  Created by Tony Peng on 8/11/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//
#ifndef __PARTICLE_H__
#define __PARTICLE_H__

#include "vector.h"
#include "ShaderManager.h"

struct Particle
{
    tVector4    mPosition;
    tVector4    mDirection;
    tVector4    mStartPosition;
    tVector4    mColor;
    tVector4    mDeltaColor;
    
    float       mfRadialAcceleration;
    float       mfTangentialAcceleration;
    
    float       mfRadius;
    float       mfRadiusDelta;
    
    float       mfAngle;
    float       mfDegreePerSecond;
    float       mfSpriteAngle;
    float       mfSpriteAngleDelta;
    
    float       mfParticleSize;
    float       mfParticleSizeDelta;

    double      mfTimeToLive;
    
    tVector4    mOrigPosition;
};

typedef struct Particle tParticle;

struct ParticleVertex
{
    tVector4    mPosition;
    tVector2    mTexCoord;
    tVector4    mColor;
};

typedef struct ParticleVertex tParticleVertex;

class CEmitter
{
public:
    enum
    {
        PARTICLETYPE_GRAVITY = 0,
        PARTICLETYPE_RADIAL,
        
        NUM_PARTICLE_TYPES,
    };
    
public:
    CEmitter();
    ~CEmitter();
    
    inline void setSourcePosition( const tVector4* pPosition ) { mSourcePosition.fX = pPosition->fX; mSourcePosition.fY = pPosition->fY; mSourcePosition.fZ = pPosition->fZ; }
    inline void setParticleCount( int iCount ) { miParticleCount = iCount; }
    inline void setActive( bool bActive ) { mbActive = bActive; }
    inline void setDuration( double fDuration ) { mfDuration = fDuration; }
    inline void setTextureID( int iTextureID ) { miTextureID = iTextureID; }
    inline void setEmissionRate( float fRate ) { mfEmissionRate = fRate; }
    inline void setFullScreen( bool bFullScreen ) { mbFullScreen = bFullScreen; }
    
    inline int getParticleCount( void ) { return miParticleCount; }
    
    
    void loadFile( const char* szFileName );
    void render( void );
    void update( double fDelta );
    
    void start( void ); 
    void stop( void ); 
    
    inline float getDuration( void ) { return mfDuration; }
    
    void setShader( int iShader );
    bool isDone();
    bool isActive( void );
    
    inline const char* getTextureName( void ) { return mszTextureName; }
    
    void copy( CEmitter* pCopy );
    
    inline int getNumParticles( void ) { return miParticleCount; }
    inline tParticle* getParticle( int iIndex ) { return &maParticles[iIndex]; }
    
    inline void setBillBoard( bool bBillBoard ) { mbBillBoard = bBillBoard; }
    
protected:
    void addParticle();
    void initParticle( tParticle* pParticle );
    
protected:
    tVector4        mSourcePosition;
    tVector4        mSourcePositionVariance;
    int             miType;
    
    float           mfAngle;
    float           mfAngleVariance;
    
    float           mfSpeed;
    float           mfSpeedVariance;
    
    float           mfRadialAcceleration;
    float           mfTangentialAcceleration;
    
    float           mfRadialVariance;
    float           mfTangentialVariance;
    
    tVector4        mGravity;
    
    float           mfParticleLifespan;
    float           mfParticleLifespanVariance;
    
    tVector4        mStartColor;
    tVector4        mStartColorVariance;
    
    tVector4        mFinishColor;
    tVector4        mFinishColorVariance;
    
    float           mfStartParticleSize;
    float           mfStartParticleSizeVariance;
    
    float           mfFinishParticleSize;
    float           mfFinishParticleSizeVariance;
    
    int             miMaxParticles;
    int             miParticleCount;
    int             miNumParticlesAlloc;
    
    float           mfEmissionRate;
    float           mfEmitCounter;
    
    double          mfElapsedTime;
    double          mfDuration;
    
    int             miBlendFuncSource;
    int             miBlendFuncDestination;
    
    float           mfMaxRadius;
    float           mfMaxRadiusVariance;
    
    float           mfRadiusSpeed;
    float           mfMinRadius;
    
    float           mfRotatePerSecond;
    float           mfRotatePerSecondVariance;
    
    float           mfRotationStart;
    float           mfRotationStartVariance;
    
    float           mfRotationEnd;
    float           mfRotationEndVariance;
    
    tParticle*      maParticles;
        
    bool            mbActive;
    bool            mbUseTexture;
    int             miParticleIndex;
        
    int             miTextureID;
    
    char            mszTextureName[128];
    
    bool            mbFullScreen;
    
    bool            mbBillBoard;
    
    void (*mpfnCallBack)();
    
protected:
    tParticleVertex*        maParticleVerts;
    int                     miNumParticleVertsAlloc;
    
    int                     miShader;
    int                     miPosition;
    int                     miTexCoord;
    int                     miColor;
};

// Adding a separate class that can have start/end colors accessible
class CEmitterOpen : public CEmitter
{
public:
    
    inline void setStartColor( const tVector4* pColor) { mStartColor.fX = pColor->fX; mStartColor.fY = pColor->fY; mStartColor.fZ = pColor->fZ; mStartColor.fW = pColor->fW; };
    inline void setFinishColor( const tVector4* pColor) { mFinishColor.fX = pColor->fX; mFinishColor.fY = pColor->fY; mFinishColor.fZ = pColor->fZ; mFinishColor.fW = pColor->fW; };
    
public:
    
    void downSize(const float* pScaleFactor, const float* pSpeedFactor);
    
};


#endif // __PARTICLE_H__