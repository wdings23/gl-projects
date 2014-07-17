#include "menuanimplayer.h"
#include "Particle.h"

/*
**
*/
CMenuAnimPlayer::CMenuAnimPlayer( void )
{
	mfTime = 0.0f;
	mfStartTime = 0.0f;
	miState = STATE_STOP;
	miType = TYPE_ENTER;

	memset( mapAnimDB, 0, sizeof( mapAnimDB ) );
	memset( mafLastTime, 0, sizeof( mafLastTime ) );

	mpFuncData = NULL;
	mpFinishFunc = NULL;
    
    memset( mabLoop, 0, sizeof( mabLoop ) );
    mpEmitter = NULL;
    
    memset( &mAnchorOffset, 0, sizeof( tVector2 ) );
}
	
/*
**
*/
CMenuAnimPlayer::~CMenuAnimPlayer( void )
{
    if( mpEmitter )
    {
        delete mpEmitter;
    }
}

/*
**
*/
void CMenuAnimPlayer::update( float fDT )
{
    if( miState == STATE_START || miState == STATE_PLAYING )
    {
        mfTime += fDT;
	}
    
	if( miState == STATE_START )
	{
		miState = STATE_PLAYING;
        
        // start sound
        if( mapAnimDB[miType] )
        {
            const char* szSound = mapAnimDB[miType]->getSound();
            if( strlen( szSound ) )
            {
            }
        }
	}
	
    // start particle?
    if( miState == STATE_PLAYING )
    {
        if( mapAnimDB[miType] )
        {            
            const char* szParticle = mapAnimDB[miType]->getParticle();
            if( strlen( szParticle ) )
            {
                float fStartTime = mapAnimDB[miType]->getParticleStartTime();
                if( mpEmitter == NULL )
                {
                    if( fStartTime <= mfTime )
                    {
                        mpEmitter = new CEmitter();
                        mpEmitter->loadFile( szParticle );
                        mpEmitter->start();
                    }
                }
            }
        }
    }
    
	if( mfTime >= mafLastTime[miType] )
	{
        if( mabLoop[miType] )
        {
            mfTime = 0.0f;
            miState = STATE_START;
        }
        else
        {
            miState = STATE_STOP;
            
            if( mpEmitter )
            {
                delete mpEmitter;
                mpEmitter = NULL;
            }
        }
        
        if( mpFinishFunc )
        {
            mpFinishFunc( mpFuncData );
            mpFinishFunc = NULL;
            mpFuncData = NULL;
        }
	}
}

/*
**
*/
void CMenuAnimPlayer::getRotation( float* pfAngle )
{
	if( mapAnimDB[miType] )
	{
		tVectorAnimFrame rotFrame;
		mapAnimDB[miType]->getFrameAtTime( &rotFrame, CAnimPlayerDB::FRAME_ROTATION, mfTime );
		*pfAngle = rotFrame.mValue.fX * ( 3.14159f / 180.0f );
	}
	else
	{
		*pfAngle = 0.0f;
	}
}

/*
**
*/
void CMenuAnimPlayer::getScaling( tVector4* pScaling )
{
	if( mapAnimDB[miType] )
	{
		tVectorAnimFrame scaleFrame;
		mapAnimDB[miType]->getFrameAtTime( &scaleFrame, CAnimPlayerDB::FRAME_SCALE, mfTime );
		memcpy( pScaling, &scaleFrame.mValue, sizeof( tVector4 ) );
		pScaling->fX *= 0.01f;
		pScaling->fY *= 0.01f;
	}
	else
	{
		pScaling->fX = 1.0f;
		pScaling->fY = 1.0f;
	}
}

/*
**
*/
void CMenuAnimPlayer::getTranslation( tVector4* pTranslation )
{
	if( mapAnimDB[miType] )
	{
		tVectorAnimFrame transFrame;
		mapAnimDB[miType]->getFrameAtTime( &transFrame, CAnimPlayerDB::FRAME_POSITION, mfTime );
		memcpy( pTranslation, &transFrame.mValue, sizeof( tVector4 ) );
	}
	else
	{
		memset( pTranslation, 0, sizeof( tVector4 ) );
	}
}

/*
**
*/
void CMenuAnimPlayer::getAnchorPoint( tVector4* pAnchorPoint )
{
	if( mapAnimDB[miType] )
	{
		tVectorAnimFrame anchorFrame;
		mapAnimDB[miType]->getFrameAtTime( &anchorFrame, CAnimPlayerDB::FRAME_ANCHOR_PT, mfTime );
		memcpy( pAnchorPoint, &anchorFrame.mValue, sizeof( tVector4 ) );
	}
	else
	{
		memset( pAnchorPoint, 0, sizeof( tVector4 ) );
	}
}

/*
**
*/
void CMenuAnimPlayer::getColor( tVector4* pColor )
{
	if( mapAnimDB[miType] )
	{
		tVectorAnimFrame colorFrame;
		mapAnimDB[miType]->getFrameAtTime( &colorFrame, CAnimPlayerDB::FRAME_COLOR, mfTime );
		memcpy( pColor, &colorFrame.mValue, sizeof( tVector4 ) );
	}
	else
	{
		pColor->fX = pColor->fY = pColor->fZ = pColor->fW = 1.0f;
	}
}

/*
**
*/
void CMenuAnimPlayer::getActive( bool* bActive )
{
	if( mapAnimDB[miType] )
	{
		tVectorAnimFrame activeFrame;
		mapAnimDB[miType]->getFrameAtTime( &activeFrame, CAnimPlayerDB::FRAME_ACTIVE, mfTime );
		if( activeFrame.mValue.fX > 0.0f )
		{
			*bActive = true;
		}
		else
		{
			*bActive = false;
		}
	}
	else
	{
		*bActive = true;
	}
}

/*
**
*/
void CMenuAnimPlayer::addAnimDB( CAnimPlayerDB* pDB, int iType ) 
{ 
	mapAnimDB[iType] = pDB; 
	mafLastTime[iType] = mapAnimDB[iType]->getLastTime();
}

/*
**
*/
float CMenuAnimPlayer::getStageWidth( void )
{
	return mapAnimDB[miType]->getStageWidth();
}
	
/*
**
*/
float CMenuAnimPlayer::getStageHeight( void )
{
	return mapAnimDB[miType]->getStageHeight();
}

/*
**
*/
bool CMenuAnimPlayer::animTypeHasData( void )
{
    return ( mapAnimDB[miType] != NULL );
}

/*
**
*/
bool CMenuAnimPlayer::animTypeHasData( int iType )
{
    return ( mapAnimDB[iType] != NULL );
}

/*
**
*/
void CMenuAnimPlayer::setLoopType( bool bLoop, int iType )
{
    mabLoop[iType] = bLoop;
}

/*
**
*/
CAnimPlayerDB const* CMenuAnimPlayer::getAnimDB( int iType )
{
    return mapAnimDB[iType];
}

/*
**
*/
void CMenuAnimPlayer::copy( CMenuAnimPlayer* pDestination )
{
    for( int i = 0; i < NUM_TYPES; i++ )
    {
        pDestination->mapAnimDB[i] = mapAnimDB[i];
    }
    
    pDestination->mfStartTime = mfStartTime;
    pDestination->mfTime = mfTime;
    
    pDestination->miPrevState = miPrevState;
    pDestination->miState = miState;
    
    pDestination->miPrevType = miPrevType;
    pDestination->miType = miType;
    
    memcpy( pDestination->mafLastTime, mafLastTime, sizeof( mafLastTime ) );
    memcpy( pDestination->mabLoop, mabLoop, sizeof( mabLoop ) );
    
    pDestination->mpFuncData = mpFuncData;
    pDestination->mpFinishFunc = mpFinishFunc;
}
