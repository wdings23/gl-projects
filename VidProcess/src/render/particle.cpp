#include "filepathutil.h"
#include "tinyxml.h"
#include "Particle.h"
#include "TextureManager.h"
#include "Matrix.h"
#include "ShaderManager.h"
#include "camera.h"

#if !defined( WINDOWS )
#include "zlib.h"
#endif // WINDOWS

#define NUM_PARTICLES_PER_ALLOC 1000
#define MAX_UPDATE_RATE 20.0f

#define SCREEN_TO_WORLD_SCALE 1.0f
#define SPEED_SCALE 1.0f
#define SIZE_SCALE 1.0f

int decode_base64(unsigned char *dest, const char *src);

void renderQuad( const char* szTextureName,
                tVector4 const* aXFormV,
                tVector4 const* aColors,
                tVector2 const* aUV,
                int iShaderID,
                bool bScreenAligned,
                int miBlendFuncSource,
                int miBlendFuncDestination );

CEmitter::CEmitter( void )
{
    maParticles = NULL;
    maParticleVerts = NULL;
    
    mpfnCallBack = NULL;
    
    miParticleCount = 0;
    mfEmitCounter = 0.0f;
    
    memset( &mSourcePosition, 0, sizeof( mSourcePosition ) );
    mSourcePosition.fW = 1.0f;
    miShader = 0;
    mbFullScreen = false;
    
    mbBillBoard = false;
}


CEmitter::~CEmitter()
{
#if defined( USE_RENDER_HEAP )
    renderFree( maParticles );
    renderFree( maParticleVerts );
#else
    FREE( maParticles );
    FREE( maParticleVerts );
#endif // USE_RENDER_HEAP
    
    mpfnCallBack = NULL;
}

/*
**
*/
void CEmitter::start( void ) 
{ 
    mbActive = true; mfElapsedTime = 0.0; 
    miParticleCount = 0;
}

/*
**
*/
void CEmitter::stop( void ) 
{ 
    mbActive = false; 
    //miParticleCount = 0;
}

/*
**
*/
bool CEmitter::isActive( void )
{
    bool bRet = mbActive;
    return bRet;
}

void CEmitter::loadFile( const char* szFileName )
{    
    char szFullPath[256];
    getFullPath( szFullPath, szFileName );
    
    TiXmlDocument doc( szFullPath );
    bool bLoaded = doc.LoadFile();
    if( bLoaded )
    {        
        TiXmlElement* pElement = doc.FirstChildElement();
        TiXmlNode* pNode = pElement->FirstChild();
        
        while( pNode )
        {
            if( !strcmp( pNode->Value(), "sourcePosition" ) )
            {
                const char* szX = pNode->ToElement()->Attribute( "x" );
                const char* szY = pNode->ToElement()->Attribute( "y" );
                
                mSourcePosition.fX = atof( szX );
                mSourcePosition.fY = atof( szY );
            }
            else if( !strcmp( pNode->Value(), "sourcePositionVariance" ) )
            {
                const char* szX = pNode->ToElement()->Attribute( "x" );
                const char* szY = pNode->ToElement()->Attribute( "y" );
                
                mSourcePositionVariance.fX = atof( szX );
                mSourcePositionVariance.fY = atof( szY );
            }
            else if( !strcmp( pNode->Value(), "speed" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfSpeed = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "speedVariance" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfSpeedVariance = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "particleLifeSpan" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfParticleLifespan = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "particleLifespanVariance" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfParticleLifespanVariance = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "angle" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfAngle = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "angleVariance" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfAngleVariance = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "gravity" ) )
            {
                const char* szX = pNode->ToElement()->Attribute( "x" );
                const char* szY = pNode->ToElement()->Attribute( "y" );
                
                mGravity.fX = atof( szX );
                mGravity.fY = atof( szY );
            }
            else if( !strcmp( pNode->Value(), "radialAcceleration" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfRadialAcceleration = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "tangentialAcceleration" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfTangentialAcceleration = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "tangentialAccelVariance" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfTangentialVariance = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "radialAcceleration" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfRadialAcceleration = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "radialAccelVariance" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfRadialVariance = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "startColor" ) )
            {
                const char* szRed = pNode->ToElement()->Attribute( "red" );
                const char* szGreen = pNode->ToElement()->Attribute( "green" );
                const char* szBlue = pNode->ToElement()->Attribute( "blue" );
                const char* szAlpha = pNode->ToElement()->Attribute( "alpha" );
                
                mStartColor.fX = atof( szRed );
                mStartColor.fY = atof( szGreen );
                mStartColor.fZ = atof( szBlue );
                mStartColor.fW = atof( szAlpha );
            }
            else if( !strcmp( pNode->Value(), "startColorVariance" ) )
            {
                const char* szRed = pNode->ToElement()->Attribute( "red" );
                const char* szGreen = pNode->ToElement()->Attribute( "green" );
                const char* szBlue = pNode->ToElement()->Attribute( "blue" );
                const char* szAlpha = pNode->ToElement()->Attribute( "alpha" );
                
                mStartColorVariance.fX = atof( szRed );
                mStartColorVariance.fY = atof( szGreen );
                mStartColorVariance.fZ = atof( szBlue );
                mStartColorVariance.fW = atof( szAlpha );
            }
            else if( !strcmp( pNode->Value(), "finishColor" ) )
            {
                const char* szRed = pNode->ToElement()->Attribute( "red" );
                const char* szGreen = pNode->ToElement()->Attribute( "green" );
                const char* szBlue = pNode->ToElement()->Attribute( "blue" );
                const char* szAlpha = pNode->ToElement()->Attribute( "alpha" );
                
                mFinishColor.fX = atof( szRed );
                mFinishColor.fY = atof( szGreen );
                mFinishColor.fZ = atof( szBlue );
                mFinishColor.fW = atof( szAlpha );
            }
            else if( !strcmp( pNode->Value(), "finishColorVariance" ) )
            {
                const char* szRed = pNode->ToElement()->Attribute( "red" );
                const char* szGreen = pNode->ToElement()->Attribute( "green" );
                const char* szBlue = pNode->ToElement()->Attribute( "blue" );
                const char* szAlpha = pNode->ToElement()->Attribute( "alpha" );
                
                mFinishColorVariance.fX = atof( szRed );
                mFinishColorVariance.fY = atof( szGreen );
                mFinishColorVariance.fZ = atof( szBlue );
                mFinishColorVariance.fW = atof( szAlpha );
            }
            else if( !strcmp( pNode->Value(), "maxParticles" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                miMaxParticles = atoi( szNum );
            }
            else if( !strcmp( pNode->Value(), "startParticleSize" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfStartParticleSize = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "startParticleSizeVariance" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfStartParticleSizeVariance = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "finishParticleSize" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfFinishParticleSize = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "finishParticleSizeVariance" ) ||
                     !strcmp( pNode->Value(), "FinishParticleSizeVariance" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfFinishParticleSizeVariance = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "duration" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfDuration = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "emitterType" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                miType = atoi( szNum );
            }
            else if( !strcmp( pNode->Value(), "maxRadius" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfMaxRadius = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "maxRadiusVariance" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfMaxRadiusVariance = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "minRadius" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfMinRadius = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "rotatePerSecond" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfRotatePerSecond = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "rotatePerSecondVariance" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfRotatePerSecondVariance = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "blendFuncSource" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                miBlendFuncSource = atoi( szNum );
            }
            else if( !strcmp( pNode->Value(), "blendFuncDestination" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                miBlendFuncDestination = atoi( szNum );
            }
            else if( !strcmp( pNode->Value(), "rotationStart" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfRotationStart = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "rotationStartVariance" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfRotationStartVariance = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "rotationEnd" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfRotationEnd = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "rotationEndVariance" ) )
            {
                const char* szNum = pNode->ToElement()->Attribute( "value" );
                mfRotationEndVariance = atof( szNum );
            }
            else if( !strcmp( pNode->Value(), "texture" ) )
            {
                const char* szTextureName = pNode->ToElement()->Attribute( "name" );
                strncpy( mszTextureName, szTextureName, sizeof( mszTextureName ) );
            
                const char* szStartExtension = strstr( szFileName, "." );
                
                // check for pvr texture
                char szPVRTextureName[256];
                memset( szPVRTextureName, 0, sizeof( szPVRTextureName ) );
                memcpy( szPVRTextureName, szFileName, (unsigned int)( szStartExtension - szFileName ) );
                strncat( szPVRTextureName, ".pvr", 4 );
                char szFullPath[256];
                getFullPath( szFullPath, szPVRTextureName );
                FILE* pPVRFilePtr = fopen( szFullPath, "rb" );
                if( pPVRFilePtr )
                {
                    strncpy( mszTextureName, szPVRTextureName, sizeof( mszTextureName ) );
                }
                else
                {
                    memset( mszTextureName, 0, sizeof( mszTextureName ) );
                    memcpy( mszTextureName, szFileName, (unsigned int)( szStartExtension - szFileName ) );
                    strncat( mszTextureName, ".png", sizeof( mszTextureName ) );
                }
                
                fclose( pPVRFilePtr );
                
#if 0
                const char* szData = pNode->ToElement()->Attribute( "data" );
                
                // temp image
                unsigned char* acZippedData = (unsigned char *)MALLOC( 256 * 256 * 4 );
                
                // decode to usable binary format
                int iSize = decode_base64( acZippedData, szData );
                
                // zip file name
                char szZipName[512];
                const char* szEnd = strstr( mszTextureName, "." );
                memset( szZipName, 0, sizeof( szZipName ) );
                memcpy( szZipName, mszTextureName, (unsigned int)szEnd - (unsigned int)mszTextureName );
                strncat( szZipName, ".zip", sizeof( szZipName ) );
                
                // save out zipped image
                char szFullPath[512];
                getWritePath( szFullPath, szZipName );
                FILE* fp = fopen( szFullPath, "wb" );
                assert( fp );
                fwrite( acZippedData, sizeof( char ), iSize, fp );
                fclose( fp );
                
                // re-open for decompression
                fp = fopen( szFullPath, "rb" );
                FREE( acZippedData );
                
                // texture file to save out
                char szFullOutPath[512];
                getWritePath( szFullOutPath, mszTextureName );
                FILE* pOutFile = fopen( szFullOutPath, "wb" );
                
                // decompress
                z_stream stream;
                stream.zalloc = Z_NULL;
                stream.zFREE = Z_NULL;
                stream.opaque = Z_NULL;
                stream.avail_in = 0;
                stream.next_in = Z_NULL;
                
                int iRet = inflateInit2( &stream, 16 + MAX_WBITS );
                assert( iRet == Z_OK );
                
                const int iChunk = 1024;
                
                do 
                {    
                    Bytef aIn[iChunk];
                    Bytef aOut[iChunk];
                    stream.avail_in = fread( aIn, 1, iChunk, fp );
                    if( stream.avail_in == 0 )
                    {
                        break;
                    }
                    
                    stream.next_in = aIn;
                    
                    bool bError = false;
                    do 
                    {
                        stream.avail_out = iChunk;
                        stream.next_out = aOut;
                        iRet = inflate( &stream, Z_NO_FLUSH );
                        assert( iRet != Z_STREAM_ERROR );
                        
                        switch( iRet )
                        {
                            case Z_NEED_DICT:
                                iRet = Z_DATA_ERROR;
                            case Z_DATA_ERROR:
                            case Z_MEM_ERROR:
                                inflateEnd( &stream );
                                bError = true;
                                break;
                        }
                        
                        if( bError )
                        {
                            iRet = Z_STREAM_END;
                            break;
                        }
                        
                        unsigned int iGotten = iChunk - stream.avail_out;
                        fwrite( aOut, 1, iGotten, pOutFile );
                        
                    } while( stream.avail_out == 0 );
                    
                } while( iRet != Z_STREAM_END );
                
                fclose( fp );
                fclose( pOutFile );
#endif // #if 0
                
                // delete the zip file
                //deleteFile( szZipName );
            
                CTextureManager::instance()->registerTexture( mszTextureName );
            }
                        
            pNode = pNode->NextSibling();
        }   // while valid node
        
        
    }   // if loaded
    else
    {
        WTFASSERT2( 0, "can't load %s error: %s", szFileName, doc.ErrorDesc() );
    }
    
    assert( miMaxParticles > 0 );
    miNumParticlesAlloc = miMaxParticles;
    
#if defined( USE_RENDER_HEAP )
    maParticles = (tParticle *)renderMALLOC( sizeof( tParticle ) * miNumParticlesAlloc );
    maParticleVerts = (tParticleVertex *)renderMALLOC( sizeof( tParticleVertex ) * miNumParticlesAlloc * 6 );
#else
    maParticles = (tParticle *)MALLOC( sizeof( tParticle ) * miNumParticlesAlloc );
    maParticleVerts = (tParticleVertex *)MALLOC( sizeof( tParticleVertex ) * miNumParticlesAlloc * 6 );
    
    memset( maParticles, 0, sizeof( tParticle ) * miNumParticlesAlloc );
#endif // USE_RENDER_HEAP
    
    if( mfParticleLifespan == 0.0f )
    {
        mfEmissionRate = (float)miMaxParticles / mfParticleLifespanVariance;
    }
    else
    {
        mfEmissionRate = (float)miMaxParticles / mfParticleLifespan;
    }
    
#if defined( ANDROID_NDK )
    FREE( acBuffer );
#endif // ANDROID_NDK
}


void CEmitter::update( double fDelta )
{
    if( mbActive )
    {
        if( mfEmissionRate > 0.0f )
        {
            float fRate = 1.0f / mfEmissionRate;
            mfEmitCounter += (float)fDelta;
            while( miParticleCount < miMaxParticles && mfEmitCounter > fRate )
            {
                addParticle();
                mfEmitCounter -= fRate;
            }   
            
            // stop emitter after timey
            mfElapsedTime += fDelta;
            if( mfDuration >= 0.0 && mfDuration < mfElapsedTime )
            {            
                stop();
                if( mpfnCallBack )
                {
                    mpfnCallBack();
                }
            }   // stop
        }   // if emission rate > 0
    }   // if active
       
    // while still has particles
    miParticleIndex = 0;
    while( miParticleIndex < miParticleCount )
    {
        tParticle* pCurrParticle = &maParticles[miParticleIndex];
        pCurrParticle->mfAngle += pCurrParticle->mfDegreePerSecond * (float)fDelta * 5.0f;
        pCurrParticle->mfSpriteAngle += pCurrParticle->mfSpriteAngleDelta * (float)fDelta;
        
        // particle is still alive
        pCurrParticle->mfTimeToLive -= fDelta;
        if( pCurrParticle->mfTimeToLive > 0 )
        {
            // if type == radial
            if( miType == PARTICLETYPE_RADIAL )
            {
                pCurrParticle->mfAngle += pCurrParticle->mfDegreePerSecond * (float)fDelta;
                pCurrParticle->mfRadius -= pCurrParticle->mfRadiusDelta;
        
                Vector2 tempPos =
                {
                    mSourcePosition.fX - cosf( pCurrParticle->mfAngle ) * pCurrParticle->mfRadius,
                    mSourcePosition.fY - sinf( pCurrParticle->mfAngle ) * pCurrParticle->mfRadius,
                };
                
                pCurrParticle->mPosition.fX = tempPos.fX;
                pCurrParticle->mPosition.fY = tempPos.fY;
        
                if( pCurrParticle->mfRadius < mfMinRadius )
                {
                    pCurrParticle->mfTimeToLive = 0.0f;
                }
            }
            else
            {
                tVector4 temp = { 0.0f, 0.0f, 0.0f, 1.0f }; 
                tVector4 radial = { 0.0f, 0.0f, 0.0f, 1.0f };
                tVector4 tangential = { 0.0f, 0.0f, 0.0f, 1.0f };
                
                tVector4 diff;
                Vector4Subtract( &diff, &pCurrParticle->mStartPosition, &radial );
                
                tVector4 copy;
                memcpy( &copy, &pCurrParticle->mPosition, sizeof( copy ) );
                Vector4Subtract( &pCurrParticle->mPosition, &copy, &diff );
                pCurrParticle->mPosition.fW = 1.0f;
                
                if( pCurrParticle->mPosition.fX || 
                    pCurrParticle->mPosition.fY )
                {
                    Vector4Normalize( &radial, &pCurrParticle->mPosition );
                }
                
                tangential.fX = radial.fX;
                tangential.fY = radial.fY;
                radial.fX *= pCurrParticle->mfRadialAcceleration;
                radial.fY *= pCurrParticle->mfRadialAcceleration;
                
                float fNewY = tangential.fX;
                tangential.fX = -tangential.fY;
                tangential.fY = fNewY;
                tangential.fX *= pCurrParticle->mfTangentialAcceleration;
                tangential.fY *= pCurrParticle->mfTangentialAcceleration;
                
                temp.fX = ( radial.fX + tangential.fX + mGravity.fX ) * (float)fDelta;
                temp.fY = ( radial.fY + tangential.fY + mGravity.fY ) * (float)fDelta;
                temp.fZ = 0.0f;
                temp.fW = 1.0f;
                
                pCurrParticle->mDirection.fX += temp.fX;
                pCurrParticle->mDirection.fY += temp.fY;
                pCurrParticle->mDirection.fZ = 0.0f;
                pCurrParticle->mDirection.fW = 1.0f;
                
                temp.fX = pCurrParticle->mDirection.fX * (float)fDelta;
                temp.fY = pCurrParticle->mDirection.fY * (float)fDelta;
            
                pCurrParticle->mPosition.fX += temp.fX;
                pCurrParticle->mPosition.fY += temp.fY;
                pCurrParticle->mPosition.fZ += temp.fZ;
                
                pCurrParticle->mPosition.fX += diff.fX;
                pCurrParticle->mPosition.fY += diff.fY;
                pCurrParticle->mPosition.fZ += diff.fZ;
                
                pCurrParticle->mPosition.fW = 1.0f;
                
            }   // else particle is still showing
            
            // update size
            pCurrParticle->mfParticleSize += pCurrParticle->mfParticleSizeDelta;
            if( pCurrParticle->mfParticleSize < 0.0f )
            {
                pCurrParticle->mfParticleSize = 0.0f;
            }
            
            // update color
            pCurrParticle->mColor.fX += pCurrParticle->mDeltaColor.fX;
            pCurrParticle->mColor.fY += pCurrParticle->mDeltaColor.fY;
            pCurrParticle->mColor.fZ += pCurrParticle->mDeltaColor.fZ;
            pCurrParticle->mColor.fW += pCurrParticle->mDeltaColor.fW;
            
            //placeParticleForRender( pCurrParticle );
            
            ++miParticleIndex;
        }   // if current particle time to live > 0
        else
        {
            if( miParticleIndex != miParticleCount - 1 )
            {
                maParticles[miParticleIndex] = maParticles[miParticleCount-1];
            }
            
            --miParticleCount;
        }   // else particle time expires
        
    }   // while particle index < particle count
}


void CEmitter::render( void )
{
    if( miParticleCount <= 0 )
    {
        return;
    }
    
    float fScale = 1.0f;
    unsigned int iShaderID = CShaderManager::instance()->getShader( "generic" );
    if( mbFullScreen )
    {
        iShaderID = CShaderManager::instance()->getShader( "noperspective" );
    }
    
    //int iPrevShader = -1;
    //glGetIntegerv( GL_CURRENT_PROGRAM, &iPrevShader );
    
    //glUseProgram( iShaderID );
    
    float fHalfScreenWidth = (float)SCREEN_WIDTH * 0.5f;
    float fHalfScreenHeight = (float)SCREEN_HEIGHT * 0.5f;
    
    tTexture* pTexture = CTextureManager::instance()->getTexture( mszTextureName );
    WTFASSERT2( pTexture, "can't find texture : %s", mszTextureName );
    float fU = (float)pTexture->miWidth / (float)pTexture->miGLWidth;
    float fV = (float)pTexture->miHeight / (float)pTexture->miGLHeight;
    
    glBlendFunc( miBlendFuncSource, miBlendFuncDestination );
    for( int i = 0; i < miParticleCount; i++ )
    {        
        WTFASSERT2( i < miNumParticlesAlloc * 4, "Particle vertex is out of bounds %d", i );
        
        // z rotate 
        // cos(x)  -sin(y)
        // sin(x)  cos(y)
        
        tParticle* pParticle = &maParticles[i];
        if( pParticle->mColor.fW <= 0.0f )
        {
            continue;
        }
        
        float fHalfSize = pParticle->mfParticleSize;
        
        tMatrix44 rotateZ;
        Matrix44RotateZ( &rotateZ, pParticle->mfSpriteAngle * ( 3.14159f / 180.0f ) );
        
        tVector4 aV[4] =
        {
            { -fHalfSize, -fHalfSize, 0.0f, 1.0f },
            { fHalfSize, -fHalfSize, 0.0f, 1.0f },
            { -fHalfSize, fHalfSize, 0.0f, 1.0f },
            { fHalfSize, fHalfSize, 0.0f, 1.0f },
        };
        
        tVector4 aXFormV[4];
        tVector4 aColors[4];
        
        for( int j = 0; j < 4; j++ )
        {
            Matrix44Transform( &aXFormV[j], &aV[j], &rotateZ );
            aXFormV[j].fX += pParticle->mPosition.fX;
            aXFormV[j].fY += pParticle->mPosition.fY;
            aXFormV[j].fZ = pParticle->mPosition.fZ;
            aXFormV[j].fW = 1.0f;
            
            aXFormV[j].fX *= fScale;
            aXFormV[j].fY *= fScale;
            
            if( mbFullScreen )
            {
                aXFormV[j].fX = ( aXFormV[j].fX - fHalfScreenWidth ) / fHalfScreenWidth; 
                aXFormV[j].fY = ( fHalfScreenHeight - aXFormV[j].fY ) / fHalfScreenHeight;
                aXFormV[j].fZ = 0.0f;
            }
            
            if( mbBillBoard )
            {
                tMatrix44 const* pBillboardMatrix = CCamera::instance()->getInvRotMatrix();
                tVector4 copy;
                memcpy( &copy, &aXFormV[j], sizeof( tVector4 ) );
                Matrix44Transform( &aXFormV[j], &copy, pBillboardMatrix );
            }
            
            memcpy( &aColors[j], &pParticle->mColor, sizeof( tVector4 ) );
        }   // for j = 0 to 4
        
        tVector2 aUV[] =
        {
            { 0.0f, 0.0f },
            { 0.0f, fU },
            { fV, 0.0f },
            { fU, fV },
        };
    
        renderQuad( mszTextureName, aXFormV, aColors, aUV, iShaderID, true, miBlendFuncSource, miBlendFuncDestination );
    }   // for i = 0 to num particles
    
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    
    //glUseProgram( iPrevShader );
}


void CEmitter::addParticle()
{
    if( miParticleCount + 1 < miNumParticlesAlloc )
    {
        assert( maParticles );
        tParticle* pParticle = &maParticles[miParticleCount];
        initParticle( pParticle );
        ++miParticleCount;
    }
}


void CEmitter::setShader( int iShader )
{
    miShader = iShader;
    miPosition = glGetAttribLocation( miShader, "position" );
    miTexCoord = glGetAttribLocation( miShader, "textureUV" );
    miColor = glGetAttribLocation( miShader, "color" );
}

static float randomNegOneToOne()
{    
    return ( (float)( ( rand() % 200 ) - 100 ) * 0.01f );
}


void CEmitter::initParticle( tParticle* pParticle )
{
    float fDegreeToRadian = 3.14159f / 180.0f;
    
    pParticle->mPosition.fX = mSourcePosition.fX + ( mSourcePositionVariance.fX * randomNegOneToOne() );
    pParticle->mPosition.fY = mSourcePosition.fY + ( mSourcePositionVariance.fY * randomNegOneToOne() );
    pParticle->mPosition.fZ = mSourcePosition.fZ;
    pParticle->mPosition.fW = 1.0f;
    
    if( mbBillBoard )
    {
        tVector4 copy;
        memcpy( &copy, &pParticle->mPosition, sizeof( tVector4 ) );
        tMatrix44 const* pBillBoardMatrix = CCamera::instance()->getInvRotMatrix();
        Matrix44Transform( &pParticle->mPosition, &copy, pBillBoardMatrix );
    }
    
    pParticle->mStartPosition.fX = mSourcePosition.fX;
    pParticle->mStartPosition.fY = mSourcePosition.fY;
    pParticle->mStartPosition.fZ = mSourcePosition.fZ;
    pParticle->mStartPosition.fW = 1.0f;
    
    float fNewAngle = (float)( mfAngle + mfAngleVariance * randomNegOneToOne() ) * fDegreeToRadian;
    tVector4 vector = 
    {
        cosf( fNewAngle ),
        sinf( fNewAngle ),
        0.0f,
        1.0f
    };
    
    float fVectorSpeed = mfSpeed + mfSpeedVariance * randomNegOneToOne();
    
    
    // direction
    pParticle->mDirection.fX = vector.fX * fVectorSpeed;
    pParticle->mDirection.fY = vector.fY * fVectorSpeed;
    pParticle->mDirection.fZ = 0.0f;
    pParticle->mDirection.fW = 1.0f;
    
    // radius
    pParticle->mfRadius = mfMaxRadius + mfMaxRadiusVariance * randomNegOneToOne();
    pParticle->mfRadiusDelta = ( mfMaxRadius / mfParticleLifespan ) * ( 1.0f / MAX_UPDATE_RATE );
    pParticle->mfAngle = ( mfAngle + mfAngleVariance * randomNegOneToOne() ) * fDegreeToRadian;
    pParticle->mfDegreePerSecond = ( mfRotatePerSecond + mfRotatePerSecondVariance * randomNegOneToOne() ) * fDegreeToRadian;
    
    pParticle->mfRadialAcceleration = mfRadialAcceleration;
    pParticle->mfTangentialAcceleration = mfTangentialAcceleration;
    
    // lifespan
    pParticle->mfTimeToLive = mfParticleLifespan + mfParticleLifespanVariance * randomNegOneToOne();
    if( pParticle->mfTimeToLive < 0.0f )
    {
        pParticle->mfTimeToLive = 0.0f;
    }
    
    float fTimeToLive = pParticle->mfTimeToLive;
    if( fTimeToLive == 0.0f )
    {
        fTimeToLive = 1.0f;
    }
    pParticle->mfSpriteAngle = mfRotationStart;
    pParticle->mfSpriteAngleDelta = ( ( mfRotationEnd + mfRotationStartVariance * randomNegOneToOne() ) - 
                                    ( mfRotationStart + mfRotationEndVariance * randomNegOneToOne() ) ) /
                                    fTimeToLive;
    
    // particle size 
    float fParticleStartSize = mfStartParticleSize + mfStartParticleSizeVariance * randomNegOneToOne();
    float fParticleFinishSize = mfFinishParticleSize + mfFinishParticleSizeVariance * randomNegOneToOne();
    pParticle->mfParticleSizeDelta = ( ( fParticleFinishSize - fParticleStartSize ) / pParticle->mfTimeToLive ) * ( 1.0f / MAX_UPDATE_RATE );
    pParticle->mfParticleSize = fParticleStartSize;
    if( pParticle->mfParticleSize < 0.0f )
    {
        pParticle->mfParticleSize = 0.0;
    }
    
    // start particle color
    tVector4 start = { 0.0f, 0.0f, 0.0f, 0.0f };
    start.fX = mStartColor.fX + mStartColorVariance.fX * randomNegOneToOne();
    start.fY = mStartColor.fY + mStartColorVariance.fY * randomNegOneToOne();
    start.fZ = mStartColor.fZ + mStartColorVariance.fZ * randomNegOneToOne();
    start.fW = mStartColor.fW + mStartColorVariance.fW * randomNegOneToOne();
    
    if( start.fW < 0.0f )
    {
        start.fW = 0.0f;
    }
    
    if( start.fW > 1.0f )
    {
        start.fW = 1.0f;
    }
    
    // end particle color
    tVector4 end = { 0.0f, 0.0f, 0.0f, 0.0f };
    end.fX = mFinishColor.fX + mFinishColorVariance.fX * randomNegOneToOne();
    end.fY = mFinishColor.fY + mFinishColorVariance.fY * randomNegOneToOne();
    end.fZ = mFinishColor.fZ + mFinishColorVariance.fZ * randomNegOneToOne();
    end.fW = mFinishColor.fW + mFinishColorVariance.fW * randomNegOneToOne();
     
    if( end.fW < 0.0f )
    {
        end.fW = 0.0f;
    }
    
    if( end.fW > 1.0f )
    {
        end.fW = 1.0f;
    }
    
    memcpy( &pParticle->mColor, &start, sizeof( tVector4 ) );
    pParticle->mDeltaColor.fX = ( ( end.fX - start.fX ) / (float)pParticle->mfTimeToLive ) * ( 1.0f / MAX_UPDATE_RATE );
    pParticle->mDeltaColor.fY = ( ( end.fY - start.fY ) / (float)pParticle->mfTimeToLive ) * ( 1.0f / MAX_UPDATE_RATE );
    pParticle->mDeltaColor.fZ = ( ( end.fZ - start.fZ ) / (float)pParticle->mfTimeToLive ) * ( 1.0f / MAX_UPDATE_RATE );
    pParticle->mDeltaColor.fW = ( ( end.fW - start.fW ) / (float)pParticle->mfTimeToLive ) * ( 1.0f / MAX_UPDATE_RATE );
}

bool CEmitter::isDone()
{
    /*if( mfDuration >= 0.0f )
    {
        if( mfElapsedTime >= mfDuration && miParticleCount <= 0 )
        {
            return true;
        }
    }
    
    return false;*/
    
    bool bRet = true;
    for( int i = 0; i < miMaxParticles; i++ )
    {
        if( maParticles[i].mfTimeToLive > 0.0 )
        {
            bRet = false;
            break;
        }
    }
    
    return bRet;
}

static int is_base64(char c) {
    
    if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
       (c >= '0' && c <= '9') || (c == '+')             ||
       (c == '/')             || (c == '=')) {
        
        return 1;
        
    }
    
    return 0;
    
}

static unsigned char decode(char c) {
    
    if(c >= 'A' && c <= 'Z') return(c - 'A');
    if(c >= 'a' && c <= 'z') return(c - 'a' + 26);
    if(c >= '0' && c <= '9') return(c - '0' + 52);
    if(c == '+')             return 62;
    
    return 63;
    
}

int decode_base64(unsigned char *dest, const char *src) {
    
    if(src && *src) {
        
        unsigned char *p= dest;
        int k, l= (int)strlen(src)+1;
        unsigned char *buf= (unsigned char *)calloc(sizeof(unsigned char), l);
        
        
        /* Ignore non base64 chars as per the POSIX standard */
        for(k=0, l=0; src[k]; k++) {
            
            if(is_base64(src[k])) {
                
                buf[l++]= src[k];
                
            }
        } 
        
        for(k=0; k<l; k+=4) {
            
            char c1='A', c2='A', c3='A', c4='A';
            unsigned char b1=0, b2=0, b3=0, b4=0;
            
            c1= buf[k];
            
            if(k+1<l) {
                
                c2= buf[k+1];
                
            }
            
            if(k+2<l) {
                
                c3= buf[k+2];
                
            }
            
            if(k+3<l) {
                
                c4= buf[k+3];
                
            }
            
            b1= decode(c1);
            b2= decode(c2);
            b3= decode(c3);
            b4= decode(c4);
            
            *p++=((b1<<2)|(b2>>4) );
            
            if(c3 != '=') {
                
                *p++=(((b2&0xf)<<4)|(b3>>2) );
                
            }
            
            if(c4 != '=') {
                
                *p++=(((b3&0x3)<<6)|b4 );
                
            }
            
        }
        
        FREE(buf);
        
        return (int)(p-dest);
        
    }
    
    return 0;
    
}

/*
**
*/
void CEmitter::copy( CEmitter* pCopy )
{
    //memcpy( &pCopy->mSourcePosition, &mSourcePosition, sizeof( pCopy->mSourcePosition ) );
    memcpy( &pCopy->mSourcePositionVariance, &mSourcePositionVariance, sizeof( pCopy->mSourcePositionVariance ) );
    
    pCopy->miType = miType;
    
    pCopy->mfAngle = mfAngle;
    pCopy->mfAngleVariance = mfAngleVariance;
    
    pCopy->mfSpeed = mfSpeed;
    pCopy->mfSpeedVariance = mfSpeedVariance;
    
    pCopy->mfRadialAcceleration = mfRadialAcceleration;
    pCopy->mfTangentialAcceleration = mfTangentialAcceleration;
    
    pCopy->mfRadialVariance = mfRadialVariance;
    pCopy->mfTangentialVariance = mfTangentialVariance;
    
    memcpy( &pCopy->mGravity, &mGravity, sizeof( mGravity ) );
    
    pCopy->mfParticleLifespan = mfParticleLifespan;
    pCopy->mfParticleLifespanVariance = mfParticleLifespanVariance;
    
    memcpy( &pCopy->mStartColor, &mStartColor, sizeof( mStartColor ) );
    memcpy( &pCopy->mStartColorVariance, &mStartColorVariance, sizeof( mStartColorVariance ) );
    
    memcpy( &pCopy->mFinishColor, &mFinishColor, sizeof( mFinishColor ) );
    memcpy( &pCopy->mFinishColorVariance, &mFinishColorVariance, sizeof( mFinishColorVariance ) );
    
    pCopy->mfStartParticleSize = mfStartParticleSize;
    pCopy->mfStartParticleSizeVariance = mfStartParticleSizeVariance;
    
    pCopy->mfFinishParticleSize = mfFinishParticleSize;
    pCopy->mfFinishParticleSizeVariance = mfFinishParticleSizeVariance;
    
    pCopy->miMaxParticles = miMaxParticles;
    pCopy->miParticleCount = miParticleCount;
    pCopy->miNumParticlesAlloc = miNumParticlesAlloc;
    
    pCopy->mfEmissionRate = mfEmissionRate;
    pCopy->mfEmitCounter = mfEmitCounter;
    
    pCopy->mfElapsedTime = mfElapsedTime;
    pCopy->mfDuration = mfDuration;
    
    pCopy->miBlendFuncSource = miBlendFuncSource;
    pCopy->miBlendFuncDestination = miBlendFuncDestination;
    
    pCopy->mfMaxRadius = mfMaxRadius;
    pCopy->mfMaxRadiusVariance = mfMaxRadiusVariance;
    
    pCopy->mfRadiusSpeed = mfRadiusSpeed;
    pCopy->mfMinRadius = mfMinRadius;
    
    pCopy->mfRotatePerSecond = mfRotatePerSecond;
    pCopy->mfRotatePerSecondVariance = mfRotatePerSecondVariance;
    
    pCopy->mfRotationStart = mfRotationStart;
    pCopy->mfRotationStartVariance = mfRotationStartVariance;
    
    pCopy->mfRotationEnd = mfRotationEnd;
    pCopy->mfRotationEndVariance = mfRotationEndVariance;
    
    pCopy->maParticles = (tParticle *)MALLOC( sizeof( tParticle ) * miNumParticlesAlloc );
    
    pCopy->mbActive = mbActive;
    pCopy->mbUseTexture = mbUseTexture;
    pCopy->miParticleIndex = miParticleIndex;
    
    pCopy->miTextureID = miTextureID;
    
    memcpy( &pCopy->mszTextureName, mszTextureName, sizeof( pCopy->mszTextureName ) );
    
    pCopy->mbFullScreen = mbFullScreen;
    pCopy->mpfnCallBack = mpfnCallBack;
    
    pCopy->mbBillBoard = mbBillBoard;
}

/*
 **
 */

void CEmitterOpen::downSize( const float* pScaleFactor, const float* pSpeedFactor )
{
    mfSpeed *= *pSpeedFactor;
    mfSpeedVariance *= *pSpeedFactor;
    mfStartParticleSize *= *pScaleFactor;
    mfStartParticleSizeVariance *= *pScaleFactor;
    mfFinishParticleSizeVariance *= *pScaleFactor;
    mfFinishParticleSize *= *pScaleFactor;
    mGravity.fX *= *pScaleFactor;
    mGravity.fY *= *pScaleFactor;
    mfRadialAcceleration *= *pSpeedFactor;
    mfRadialVariance *= *pSpeedFactor;
    mfTangentialAcceleration *= *pSpeedFactor;
    mfTangentialVariance *= *pSpeedFactor;
}

/*
**
*/
void renderQuad( const char* szTextureName,
                 tVector4 const* aXFormV,
                 tVector4 const* aColors,
                 tVector2 const* aUV,
                 int iShaderID,
                 bool bScreenAligned,
                 int miBlendFuncSource,
                 int miBlendFuncDestination )
{

}



