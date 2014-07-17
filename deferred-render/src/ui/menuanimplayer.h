#ifndef __MENUANIMPLAYER_H__
#define __MENUANIMPLAYER_H__

#include "animplayerDB.h"
#include "Particle.h"

typedef void (*finishFunc)( void* pData );

class CMenuAnimPlayer
{
public:
	enum
	{
		STATE_STOP = 0,
		STATE_START,
		STATE_PLAYING,

		NUM_STATES,
	};

	enum
	{
		TYPE_ENTER = 0,
		TYPE_EXIT,
		TYPE_FOCUS,
		TYPE_NORMAL,
        
        TYPE_USER_0,
        TYPE_USER_1,
        TYPE_USER_2,
        TYPE_USER_3,
        TYPE_USER_4,

		NUM_TYPES,
	};
public:
	CMenuAnimPlayer( void );
	~CMenuAnimPlayer( void );
	
	void update( float fDT );

	void getRotation( float* pfAngle );
	void getScaling( tVector4* pScaling );
	void getTranslation( tVector4* pTranslation );
	void getAnchorPoint( tVector4* pAnchorPoint );
	void getColor( tVector4* pColor );
	void getActive( bool* bActive );

	void addAnimDB( CAnimPlayerDB* pDB, int iType );
	inline int getState( void ) { return miState; }
	inline void setState( int iState ) { miPrevState = miState; miState = iState; }

	inline int getType( void ) { return miType; }
	inline void setType( int iType ) { miPrevType = miType; miType = iType; mfTime = 0.0f; }

	inline float getTime( void ) { return mfTime; }
	inline float getLastTime( int iType ) { return mafLastTime[iType]; }
	
	inline void setTime( float fTime ) { mfTime = fTime; }

	inline void setFinishFunc( finishFunc pFinishFunc, void* pData ) { mpFinishFunc = pFinishFunc; mpFuncData = pData; }

	float getStageWidth( void );
	float getStageHeight( void );
    
    bool animTypeHasData( void );
    bool animTypeHasData( int iType );
    
    void setLoopType( bool bLoop, int iType );
    bool getLoop( int iType ) { return mabLoop[iType]; }
    
    CAnimPlayerDB const* getAnimDB( int iType );
    
    void copy( CMenuAnimPlayer* pDestination );
    
    inline CEmitter* getEmitter( void ) { return mpEmitter; }
    
    inline tVector2* getAnchorOffset( void ) { return &mAnchorOffset; }
    inline void setAnchorOffset( tVector2* pOffset ) { mAnchorOffset.fX = pOffset->fX; mAnchorOffset.fY = pOffset->fY; }
    
protected:
	CAnimPlayerDB*		mapAnimDB[NUM_TYPES];
	float				mfStartTime;
	float				mfTime;
	
	int					miPrevState;
	int					miState;

	int					miPrevType;
	int			   		miType;

	float				mafLastTime[NUM_TYPES];
    bool                mabLoop[NUM_TYPES];
    
	void*				mpFuncData;
	void (*mpFinishFunc)( void* pData );
    
    CEmitter*           mpEmitter;
    
    tVector2            mAnchorOffset;
};

#endif // __MENUANIMPLAYER_H__