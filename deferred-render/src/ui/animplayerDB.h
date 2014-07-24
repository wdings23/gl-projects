#ifndef __ANIMPLAYER_H__
#define __ANIMPLAYER_H__

#include "vector.h"
#include "tinyxml.h"

struct VectorAnimFrame
{
	float			mfTime;
	tVector4		mValue;
};

typedef struct VectorAnimFrame tVectorAnimFrame;

class CAnimPlayerDB
{
public:
	enum
	{
		FRAME_SCALE = 0,
		FRAME_ROTATION,
		FRAME_POSITION,
		FRAME_ANCHOR_PT,
		FRAME_COLOR,
		FRAME_ACTIVE,

		NUM_FRAM_TYPES,
	};

public:
	CAnimPlayerDB( void );
	virtual ~CAnimPlayerDB( void );
	
	void getFrameAtTime( tVectorAnimFrame* pResult, int iType, float fTime );
	void addFrame( tVectorAnimFrame* pFrame, int iType );
	
	void setName( const char* szName );
	void setParticle( const char* szName );
    void load( TiXmlNode* pNode, const char* szFileName );

	void readKeyFrames( TiXmlNode* pChild, int iType );

	inline const char* getName( void ) const { return mszName; }
    inline const char* getParticle( void ) { return mszParticle; }
    
	inline float getLastTime( void ) { return mfLastTime; }
	
	inline void setStageWidth( float fWidth ) { mfStageWidth = fWidth; }
	inline void setStageHeight( float fHeight ) { mfStageHeight = fHeight; }
	
	inline float getStageWidth( void ) { return mfStageWidth; }
	inline float getStageHeight( void ) { return mfStageHeight; }

    inline const char* getSound( void ) { return mszSound; }
    inline const char* getMusic( void ) { return mszMusic; }
    
    inline void setParticleStartTime( float fTime ) { mfStartParticleTime = fTime; }
    inline float getParticleStartTime( void ) { return mfStartParticleTime; }
    
    void setSound( const char* szSound );
    void setMusic( const char* szMusic );
    
protected:
	void parseValue( tVector4* pResult, const char* szValue );

protected:
	tVectorAnimFrame*		maScaleFrames;
	int						miNumScaleFrames;
	int						miNumScaleFrameAlloc;

	tVectorAnimFrame*		maRotationFrames;
	int						miNumRotationFrames;
	int						miNumRotationFrameAlloc;

	tVectorAnimFrame*		maPositionFrames;
	int						miNumPositionFrames;
	int						miNumPositionFrameAlloc;

	tVectorAnimFrame*		maAnchorPtFrames;
	int						miNumAnchorPtFrames;
	int						miNumAnchorPtFrameAlloc;

	tVectorAnimFrame*		maColorFrames;
	int						miNumColorFrames;
	int						miNumColorFrameAlloc;

	tVectorAnimFrame*		maActiveFrames;
	int						miNumActiveFrames;
	int						miNumActiveFrameAlloc;

	char					mszName[256];
	float					mfLastTime;

    char                    mszParticle[256];
    float                   mfStartParticleTime;
    
    char                    mszSound[256];
    char                    mszMusic[256];
    
	float					mfStageWidth;
	float					mfStageHeight;
};

#endif // __ANIMPLAYER_H__