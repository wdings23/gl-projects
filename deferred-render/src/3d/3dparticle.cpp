#include "3dparticle.h"
#include "filepathutil.h"
#include "parseutil.h"
#include "tinyxml.h"
#include "timeutil.h"
#include "camera.h"
#include "shadermanager.h"
#include "render.h"
#include "quaternion.h"

static void handleCollision( tEmitter3D* pEmitter );
static void launchParticle( tEmitter3D* pEmitter );

/*
**
*/
void emitter3DInit( tEmitter3D* pEmitter )
{
	memset( pEmitter, 0, sizeof( tEmitter3D ) );
	pEmitter->miState = EMITTER_STOPPED;
	pEmitter->miSrcBlend = GL_SRC_ALPHA;
	pEmitter->miDestBlend = GL_ONE_MINUS_SRC_ALPHA;
	pEmitter->mfParticleSizeScale = 1.0f;
	pEmitter->mfParticleSpeedScale = 1.0f;
}

/*
**
*/
void emitter3DLoad( tEmitter3D* pEmitter, const char* szFileName )
{
	/*
	<emitter>
		<direction>x,y,z</direction>
		<particle_speed>x</particle_speed>
		<min_particle_size>x</min_particle_size>
		<max_particle_size>x</max_particle_size>
		<gravity>x,y,z</gravity>
		<particle_duration>x</particle_duration>
		<time_between_particles>x</time_between_particles>
		<position_variance>x,y,z</position_variance>
		<angle_variance>x,y,z</angle_variance>
		<size_variance>x,y,z</size_variance>
		<max_num_particles>x</max_num_particles>
		<texture>x</texture>
	</emitter>
	*/

	char szFullPath[256];
	getFullPath( szFullPath, szFileName );

	TiXmlDocument doc( szFullPath );
	bool bLoaded = doc.LoadFile();
	if( bLoaded )
	{
		TiXmlNode* pNode = doc.FirstChild()->FirstChild();
		while( pNode )
		{
			const char* szType = pNode->Value();
			if( !strcmp( szType, "type" ) )
            {
                const char* szValue = pNode->FirstChild()->Value();
                if( !strcmp( szValue, "radial" ) )
                {
                    pEmitter->miType = EMITTER_TYPE_RADIAL;
                }
                else if( !strcmp( szValue, "regular" ) )
                {
                    pEmitter->miType = EMITTER_TYPE_REGULAR;
                }
            }
            else if( !strcmp( szType, "direction" ) )
			{
				const char* szValue = pNode->FirstChild()->Value();
				parseVector( &pEmitter->mDirection, szValue ); 
			}
			else if( !strcmp( szType, "particle_speed" ) )
			{
				const char* szValue = pNode->FirstChild()->Value();
				pEmitter->mfParticleSpeed = (float)atof( szValue );
			}
			else if( !strcmp( szType, "gravity" ) )
			{
				const char* szValue = pNode->FirstChild()->Value();
				parseVector( &pEmitter->mGravity, szValue ); 
			}
			else if( !strcmp( szType, "particle_duration" ) )
			{
				const char* szValue = pNode->FirstChild()->Value();
				pEmitter->mfParticleDuration = (float)atof( szValue );
			}
			else if( !strcmp( szType, "time_between_particles" ) )
			{
				const char* szValue = pNode->FirstChild()->Value();
				pEmitter->mfTimeBetweenParticles = (float)atof( szValue );
			}
			else if( !strcmp( szType, "position_variance" ) )
			{
				const char* szValue = pNode->FirstChild()->Value();
				parseVector( &pEmitter->mParticlePositionRanges, szValue ); 
			}
			else if( !strcmp( szType, "angle_variance" ) )
			{
				const char* szValue = pNode->FirstChild()->Value();
				parseVector( &pEmitter->mEmitterAngleRanges, szValue );
			}
			else if( !strcmp( szType, "size_variance" ) )
			{
				const char* szValue = pNode->FirstChild()->Value();
				parseVector( &pEmitter->mParticleSizeRanges, szValue ); 
			}
			else if( !strcmp( szType, "max_num_particles" ) )
			{
				const char* szValue = pNode->FirstChild()->Value();
				pEmitter->miMaxParticles = (int)atoi( szValue ); 
			}
			else if( !strcmp( szType, "particle_size_start" ) )
			{
				const char* szValue = pNode->FirstChild()->Value();
				parseVector( &pEmitter->mParticleSizeStart, szValue );
				memcpy( &pEmitter->mParticleSize, &pEmitter->mParticleSizeStart, sizeof( tVector4 ) );
			}
			else if( !strcmp( szType, "particle_size_end" ) )
			{
				const char* szValue = pNode->FirstChild()->Value();
				parseVector( &pEmitter->mParticleSizeEnd, szValue );  
			}
			else if( !strcmp( szType, "duration" ) )
			{
				const char* szValue = pNode->FirstChild()->Value();
				pEmitter->mfDuration = (float)atof( szValue ); 
			}
            else if( !strcmp( szType, "collision" ) )
			{
				const char* szValue = pNode->FirstChild()->Value();
				pEmitter->mbCollision = ( atoi( szValue ) > 0 );
			}
			else if( !strcmp( szType, "texture" ) )
			{
				const char* szValue = pNode->FirstChild()->Value();
				strncpy( pEmitter->mszTextureName, szValue, sizeof( pEmitter->mszTextureName ) );
			}
			else if( !strcmp( szType, "color_start" ) )
			{
				const char* szValue = pNode->FirstChild()->Value();
				parseVector( &pEmitter->mColorStart, szValue ); 
				pEmitter->mColorStart.fX /= 255.0f;
				pEmitter->mColorStart.fY /= 255.0f;
				pEmitter->mColorStart.fZ /= 255.0f;
				pEmitter->mColorStart.fW /= 255.0f;
			}
			else if( !strcmp( szType, "color_end" ) )
			{
				const char* szValue = pNode->FirstChild()->Value();
				parseVector( &pEmitter->mColorEnd, szValue );
				pEmitter->mColorEnd.fX /= 255.0f;
				pEmitter->mColorEnd.fY /= 255.0f;
				pEmitter->mColorEnd.fZ /= 255.0f;
				pEmitter->mColorEnd.fW /= 255.0f;
			}
            else if( !strcmp( szType, "particle_angle_start" ) )
            {
                const char* szValue = pNode->FirstChild()->Value();
				parseVector( &pEmitter->mParticleAngleStart, szValue );
                pEmitter->mParticleAngleStart.fW = 1.0f;
            }
            else if( !strcmp( szType, "particle_angle_end" ) )
            {
                const char* szValue = pNode->FirstChild()->Value();
				parseVector( &pEmitter->mParticleAngleEnd, szValue );
                pEmitter->mParticleAngleEnd.fW = 1.0f;
            }
            else if( !strcmp( szType, "particle_angle_variance" ) )
            {
                const char* szValue = pNode->FirstChild()->Value();
				parseVector( &pEmitter->mParticleAngleRanges, szValue );
                pEmitter->mParticleAngleRanges.fW = 1.0f;
            }
            else if( !strcmp( szType, "start_radial_rotation" ) )
            {
                const char* szValue = pNode->FirstChild()->Value();
				parseVector( &pEmitter->mStartRadialRotation, szValue );
                pEmitter->mStartRadialRotation.fW = 1.0f;
            }
			else if( !strcmp( szType, "src_blend" ) )
            {
				const char* szValue = pNode->FirstChild()->Value();
				if( !strcmp( szValue, "src_alpha" ) )
				{
					pEmitter->miSrcBlend = GL_SRC_ALPHA;
				}
				else if( !strcmp( szValue, "src_one_minus_alpha" ) )
				{
					pEmitter->miSrcBlend = GL_ONE_MINUS_SRC_ALPHA;
				}
				else if( !strcmp( szValue, "one" ) )
				{
					pEmitter->miSrcBlend = GL_ONE;
				}
				else if( !strcmp( szValue, "src_color" ) )
				{
					pEmitter->miSrcBlend = GL_SRC_COLOR;
				}
				else if( !strcmp( szValue, "dst_color" ) )
				{
					pEmitter->miSrcBlend = GL_DST_COLOR;
				}
				else if( !strcmp( szValue, "dst_alpha" ) )
				{
					pEmitter->miSrcBlend = GL_DST_ALPHA;
				}
			}
			else if( !strcmp( szType, "dest_blend" ) )
            {
				const char* szValue = pNode->FirstChild()->Value();
				if( !strcmp( szValue, "src_alpha" ) )
				{
					pEmitter->miDestBlend = GL_SRC_ALPHA;
				}
				else if( !strcmp( szValue, "src_one_minus_alpha" ) )
				{
					pEmitter->miDestBlend = GL_ONE_MINUS_SRC_ALPHA;
				}
				else if( !strcmp( szValue, "one" ) )
				{
					pEmitter->miDestBlend = GL_ONE;
				}
				else if( !strcmp( szValue, "src_color" ) )
				{
					pEmitter->miDestBlend = GL_SRC_COLOR;
				}
				else if( !strcmp( szValue, "dst_color" ) )
				{
					pEmitter->miDestBlend = GL_DST_COLOR;
				}
				else if( !strcmp( szValue, "dst_alpha" ) )
				{
					pEmitter->miDestBlend = GL_DST_ALPHA;
				}
			}
            else if( !strcmp( szType, "start_radius" ) )
            {
                const char* szValue = pNode->FirstChild()->Value();
                pEmitter->mfStartRadius = (float)atof( szValue );
            }
            else if( !strcmp( szType, "radial_anim_rotation" ) )
            {
                const char* szValue = pNode->FirstChild()->Value();
                parseVector( &pEmitter->mAnimRadialRotation, szValue );
                pEmitter->mAnimRadialRotation.fW = 1.0f;
            }
			else if( !strcmp( szType, "radial_num_particles_per_emit" ) )
            {
                const char* szValue = pNode->FirstChild()->Value();
				pEmitter->miNumParticlesPerEmit = (int)atoi( szValue );
            }
			else if( !strcmp( szType, "radial_time_between_emit" ) )
            {
                const char* szValue = pNode->FirstChild()->Value();
				pEmitter->mfTimeBetweenEmit = (float)atof( szValue );
            }

			pNode = pNode->NextSibling();
		}
	}
	else
	{
		OUTPUT( "error loading %s : %s\n", szFileName, doc.ErrorDesc() );
	}

    // initialize particles
	pEmitter->maParticles = (tParticle3D *)MALLOC( sizeof( tParticle3D ) * pEmitter->miMaxParticles );
	for( int i = 0; i < pEmitter->miMaxParticles; i++ )
	{
		tParticle3D* pParticle = &pEmitter->maParticles[i];
		memset( pParticle, 0, sizeof( tParticle3D ) );

	}	// for i = 0 to max particles
    
    // radial emitter info
    if( pEmitter->miType == EMITTER_TYPE_RADIAL )
    {
        pEmitter->mfAnglePerParticle = ( 2.0f * 3.14159f ) / pEmitter->miNumParticlesPerEmit;
        //pEmitter->mfTimeBetweenParticles = 0.0f;
        //pEmitter->mGravity.fX = pEmitter->mGravity.fY = pEmitter->mGravity.fZ = 0.0f;
    }
}

/*
**
*/
void emitter3DUpdate( tEmitter3D* pEmitter, float fDT )
{
	pEmitter->mfCurrTime += fDT;
	bool bEmit = true;
	if( pEmitter->miType == EMITTER_TYPE_RADIAL )
	{
		if( pEmitter->miState != EMITTER_ACTIVE ||
			pEmitter->mfCurrTime - pEmitter->mfLastSpawnedParticleTime < pEmitter->mfTimeBetweenEmit )
		{
			bEmit = false;
		}
	}
	else
	{

	}

	// spawn particle
	if( bEmit )
	{
		int iNumEmit = 0;
		for( int i = 0; i < pEmitter->miMaxParticles; i++ )
		{
			if( pEmitter->miState == EMITTER_ACTIVE &&
				pEmitter->mfCurrTime - pEmitter->mfLastSpawnedParticleTime >= pEmitter->mfTimeBetweenParticles )
			{
				if( pEmitter->miType == EMITTER_TYPE_RADIAL )
				{
					if( iNumEmit >= pEmitter->miNumParticlesPerEmit )
					{
						break;
					}
				}

				if( pEmitter->mfCurrTime - pEmitter->mfStartTime < pEmitter->mfDuration || pEmitter->mfDuration < 0.0f )
				{
					launchParticle( pEmitter );
					++iNumEmit;
				}
			
			}	// if time difference between last and current > time between particles

		}	// for i = 0 to max particles

	}	// if emit this time

    // radial animation rotation
    tMatrix44 xformMat;
    if( pEmitter->miType == EMITTER_TYPE_RADIAL )
    {
        tVector4 xAxis = { 1.0f, 0.0f, 0.0f, 1.0f };
        tVector4 yAxis = { 0.0f, 1.0f, 0.0f, 1.0f };
        tVector4 zAxis = { 0.0f, 0.0f, 1.0f, 1.0f };
    
        float fDT = pEmitter->mfCurrTime - pEmitter->mfStartTime;
        
        pEmitter->mCurrRadialAnimRotation.fX = pEmitter->mAnimRadialRotation.fX * fDT;
        pEmitter->mCurrRadialAnimRotation.fY = pEmitter->mAnimRadialRotation.fY * fDT;
        pEmitter->mCurrRadialAnimRotation.fZ = pEmitter->mAnimRadialRotation.fZ * fDT;
        pEmitter->mCurrRadialAnimRotation.fW = 1.0f;
        
        // emitter rotation
        tQuaternion qX, qY, qZ;
        quaternionInit( &qX );
        quaternionInit( &qY );
        quaternionInit( &qZ );
        
        quaternionFromAxisAngle( &qX, &xAxis, pEmitter->mCurrRadialAnimRotation.fX );
        quaternionFromAxisAngle( &qY, &yAxis, pEmitter->mCurrRadialAnimRotation.fY );
        quaternionFromAxisAngle( &qZ, &zAxis, pEmitter->mCurrRadialAnimRotation.fZ );
        
        tQuaternion qZY, qTotal;
        quaternionMultiply( &qZY, &qZ, &qY );
        quaternionMultiply( &qTotal, &qZY, &qX );
        
        // xform matrix
        quaternionToMatrix( &xformMat, &qTotal );
    }
    
	for( int i = 0; i < pEmitter->miMaxParticles; i++ )
	{
		tParticle3D* pParticle = &pEmitter->maParticles[i];
		if( pParticle->miState == PARTICLE_ALIVE )
		{
			if( pEmitter->mfCurrTime - pParticle->mfStartTime >= pEmitter->mfParticleDuration )
			{
				// expired
				pParticle->miState = PARTICLE_DEAD;
				--pEmitter->miNumParticles;
			}
			else
			{
                // update direction
                if( pEmitter->miType == EMITTER_TYPE_RADIAL )
                {
                    // transform with animation matrix
                    tVector4 newDirection = { 0.0f, 0.0f, 0.0f, 1.0f };
                    Matrix44Transform( &newDirection, &pParticle->mDirection, &xformMat );
                    memcpy( &pParticle->mCurrDirection, &newDirection, sizeof( tVector4 ) );
                    
                    pParticle->mCurrDirection.fX += ( pEmitter->mGravity.fX * fDT );
                    pParticle->mCurrDirection.fY += ( pEmitter->mGravity.fY * fDT );
                    pParticle->mCurrDirection.fZ += ( pEmitter->mGravity.fZ * fDT );
                }
                else
                {
                    pParticle->mCurrDirection.fX += ( pEmitter->mGravity.fX * fDT );
                    pParticle->mCurrDirection.fY += ( pEmitter->mGravity.fY * fDT );
                    pParticle->mCurrDirection.fZ += ( pEmitter->mGravity.fZ * fDT );
                }
                
                // update position
				pParticle->mPosition.fX += ( pParticle->mCurrDirection.fX * fDT * pEmitter->mfParticleSpeedScale );
				pParticle->mPosition.fY += ( pParticle->mCurrDirection.fY * fDT * pEmitter->mfParticleSpeedScale );
				pParticle->mPosition.fZ += ( pParticle->mCurrDirection.fZ * fDT * pEmitter->mfParticleSpeedScale );

				// color
				float fPct = ( pEmitter->mfCurrTime - pParticle->mfStartTime ) / pEmitter->mfParticleDuration;
				pParticle->mColor.fX = pEmitter->mColorStart.fX + ( pEmitter->mColorEnd.fX - pEmitter->mColorStart.fX ) * fPct;
				pParticle->mColor.fY = pEmitter->mColorStart.fY + ( pEmitter->mColorEnd.fY - pEmitter->mColorStart.fY ) * fPct;
				pParticle->mColor.fZ = pEmitter->mColorStart.fZ + ( pEmitter->mColorEnd.fZ - pEmitter->mColorStart.fZ ) * fPct;
				pParticle->mColor.fW = pEmitter->mColorStart.fW + ( pEmitter->mColorEnd.fW - pEmitter->mColorStart.fW ) * fPct;
			
                // angle
                pParticle->mAngle.fX = pParticle->mStartAngle.fX + ( pEmitter->mParticleAngleEnd.fX - pEmitter->mParticleAngleStart.fX ) * fPct;
                pParticle->mAngle.fY = pParticle->mStartAngle.fY + ( pEmitter->mParticleAngleEnd.fY - pEmitter->mParticleAngleStart.fY ) * fPct;
                pParticle->mAngle.fZ = pParticle->mStartAngle.fZ + ( pEmitter->mParticleAngleEnd.fZ - pEmitter->mParticleAngleStart.fZ ) * fPct;
                pParticle->mAngle.fW = 1.0f;
                
				pParticle->mSize.fX = pEmitter->mParticleSizeStart.fX + ( pEmitter->mParticleSizeEnd.fX - pEmitter->mParticleSizeStart.fX ) * fPct + pParticle->mSizeInc.fX;
				pParticle->mSize.fY = pEmitter->mParticleSizeStart.fY + ( pEmitter->mParticleSizeEnd.fY - pEmitter->mParticleSizeStart.fY ) * fPct + pParticle->mSizeInc.fY;
				pParticle->mSize.fZ = pEmitter->mParticleSizeStart.fZ + ( pEmitter->mParticleSizeEnd.fZ - pEmitter->mParticleSizeStart.fZ ) * fPct + pParticle->mSizeInc.fZ;
				pParticle->mSize.fW = 1.0f;
                
				pParticle->mSize.fX *= pEmitter->mfParticleSizeScale;
				pParticle->mSize.fY *= pEmitter->mfParticleSizeScale;
				pParticle->mSize.fZ *= pEmitter->mfParticleSizeScale;

                if( pEmitter->mbCollision )
                {
                    handleCollision( pEmitter );
                }
            }

		}	// if particle is alive

	}	// for i = 0 to max particles
}

/*
**
*/
void emitter3DDraw( tEmitter3D* pEmitter, 
				    CCamera const* pCamera, 
					unsigned int iShader, 
					GLuint iSrcBlend, 
					GLuint iDestBlend )
{
	tMatrix44 const* pViewMatrix = pCamera->getViewMatrix();
	tMatrix44 const* pProjMatrix = pCamera->getProjectionMatrix();
	tMatrix44 const* pInvRotMatrix = pCamera->getInvRotMatrix();

	tMatrix44 projViewMatrix;
	Matrix44Multiply( &projViewMatrix, pProjMatrix, pViewMatrix );
	
	for( int i = 0; i < pEmitter->miMaxParticles; i++ )
	{
		tParticle3D* pParticle = &pEmitter->maParticles[i];
		if( pParticle->miState == PARTICLE_ALIVE )
		{
			tVector4 topLeft =
			{
				-pParticle->mSize.fX * 0.5f,
				pParticle->mSize.fY * 0.5f,
				0.0f,
				1.0f
			};

			tVector4 bottomRight = 
			{
				pParticle->mSize.fX * 0.5f,
			    -pParticle->mSize.fY * 0.5f,
				0.0f,
				1.0f
			};

            tMatrix44 rotMatX, rotMatY, rotMatZ, rotMatZY, totalRotMat;
            Matrix44RotateX( &rotMatX, pParticle->mAngle.fX );
            Matrix44RotateY( &rotMatY, pParticle->mAngle.fY );
            Matrix44RotateZ( &rotMatZ, pParticle->mAngle.fZ );
            
            Matrix44Multiply( &rotMatZY, &rotMatZ, &rotMatY );
            Matrix44Multiply( &totalRotMat, &rotMatZY, &rotMatX );
            
            
			tVector4 aV[] = 
			{
				{ topLeft.fX, topLeft.fY, 0.0f, 1.0f },
				{ topLeft.fX, bottomRight.fY, 0.0f, 1.0f },
				{ bottomRight.fX, topLeft.fY, 0.0f, 1.0f },
				{ bottomRight.fX, bottomRight.fY, 0.0f, 1.0f },
			};
			
            // proj * view * position * inverseRotation
			tMatrix44 totalMatrix, projViewTranslateMatrix, translateMatrix, projViewPositionInvRotMatrix;
			Matrix44Translate( &translateMatrix, &pParticle->mPosition );
			Matrix44Multiply( &projViewTranslateMatrix, &projViewMatrix, &translateMatrix );
			Matrix44Multiply( &projViewPositionInvRotMatrix, &projViewTranslateMatrix, pInvRotMatrix );
            Matrix44Multiply( &totalMatrix, &projViewPositionInvRotMatrix, &totalRotMat );
            
			tVector4 aXFormV[4];
			
			// transform to ( -1, 1 )
			for( int i = 0; i < 4; i++ )
			{
				Matrix44Transform( &aXFormV[i],
								   &aV[i],
								   &totalMatrix );
			}
			
			tVector2 aUV[] = 
			{
				{ 0.0f, 0.0f },
				{ 0.0f, 1.0f },
				{ 1.0f, 0.0f },
				{ 1.0f, 1.0f },
			};

			tVector4 aColor[] = 
			{
				{ pParticle->mColor.fX, pParticle->mColor.fY, pParticle->mColor.fZ, pParticle->mColor.fW },
				{ pParticle->mColor.fX, pParticle->mColor.fY, pParticle->mColor.fZ, pParticle->mColor.fW },
				{ pParticle->mColor.fX, pParticle->mColor.fY, pParticle->mColor.fZ, pParticle->mColor.fW },
				{ pParticle->mColor.fX, pParticle->mColor.fY, pParticle->mColor.fZ, pParticle->mColor.fW },
			};

			renderQuad( pEmitter->mszTextureName,
						aXFormV,
						aColor,
						aUV,
						iShader,
						iSrcBlend,
						iDestBlend );
		}

	}	// for i = 0 to max particles
}

/*
**
*/
void emitter3DStart( tEmitter3D* pEmitter )
{
	pEmitter->miState = EMITTER_ACTIVE;
	pEmitter->mfStartTime = pEmitter->mfCurrTime;
}

/*
**
*/
void emitter3DStop( tEmitter3D* pEmitter )
{
	pEmitter->miState = EMITTER_STOPPED;
}

/*
**
*/
static void handleCollision( tEmitter3D* pEmitter )
{
    for( int i = 0; i < pEmitter->miMaxParticles; i++ )
    {
        tParticle3D* pParticle = &pEmitter->maParticles[i];
		if( pParticle->miState == PARTICLE_ALIVE )
		{
            if( pParticle->mPosition.fY - pParticle->mSize.fY <= 0.0f )
            {
                // reflect direction
                if( pParticle->mCurrDirection.fY < 0.0f )
                {
                    pParticle->mCurrDirection.fX = pParticle->mCurrDirection.fX * 0.65f;
                    pParticle->mCurrDirection.fY = -pParticle->mCurrDirection.fY * 0.65f;
                    pParticle->mCurrDirection.fZ = pParticle->mCurrDirection.fZ * 0.65f;
                }
                
                while( pParticle->mPosition.fY - pParticle->mSize.fY < 0.0f )
                {
                    pParticle->mPosition.fX += ( pParticle->mCurrDirection.fX * 0.016f );
                    pParticle->mPosition.fY += ( pParticle->mCurrDirection.fY * 0.016f );
                    pParticle->mPosition.fZ += ( pParticle->mCurrDirection.fZ * 0.016f );
                }
			}	// if on the ground

        }	// if particle is alive
        
    }   // for i = 0 to max particles
}

/*
**
*/
static void launchParticle( tEmitter3D* pEmitter )
{
    tVector4 xAxis = { 1.0f, 0.0f, 0.0f, 1.0f };
    tVector4 yAxis = { 0.0f, 1.0f, 0.0f, 1.0f };
    tVector4 zAxis = { 0.0f, 0.0f, 1.0f, 1.0f };
    
	// look for free particle
	for( int i = 0; i < pEmitter->miMaxParticles; i++ )
	{
		tParticle3D* pParticle = &pEmitter->maParticles[i];
		if( pParticle->miState == PARTICLE_DEAD )
		{
			pParticle->miState = PARTICLE_ALIVE;
			
			memcpy( &pParticle->mPosition, &pEmitter->mSourcePos, sizeof( tVector4 ) );
			
            if( pEmitter->miType == EMITTER_TYPE_RADIAL )
            {
                float fVelocityAngle = (float)pEmitter->miCurrParticle * pEmitter->mfAnglePerParticle;
                
                // emitter rotation
                tQuaternion qX, qY, qZ;
                quaternionInit( &qX );
                quaternionInit( &qY );
                quaternionInit( &qZ );
                
                quaternionFromAxisAngle( &qX, &xAxis, pEmitter->mStartRadialRotation.fX );
                quaternionFromAxisAngle( &qY, &yAxis, pEmitter->mStartRadialRotation.fY );
                quaternionFromAxisAngle( &qZ, &zAxis, pEmitter->mStartRadialRotation.fZ );
                
                tQuaternion qZY, qTotal;
                quaternionMultiply( &qZY, &qZ, &qY );
                quaternionMultiply( &qTotal, &qZY, &qX );
                
                tMatrix44 xformMat;
                quaternionToMatrix( &xformMat, &qTotal );
                
                // transform ( 1, 0, 0 ) with z rotation matrix
                tVector4 origPos =
                {
                    cosf( fVelocityAngle ) * pEmitter->mfStartRadius,
                    sinf( fVelocityAngle ) * pEmitter->mfStartRadius,
                    0.0f,
                    1.0f
                };
                
                tVector4 xformPos = { 0.0f, 0.0f, 0.0f, 1.0f };
                Matrix44Transform( &xformPos, &origPos, &xformMat );
                
                // transform with emitter rotation
                Matrix44Transform( &pParticle->mPosition, &origPos, &xformMat );
                pParticle->mPosition.fX += pEmitter->mSourcePos.fX;
                pParticle->mPosition.fY += pEmitter->mSourcePos.fY;
                pParticle->mPosition.fZ += pEmitter->mSourcePos.fZ;
                
                // direction
                memcpy( &pParticle->mDirection, &xformPos, sizeof( tVector4 ) );
                memcpy( &pParticle->mCurrDirection, &xformPos, sizeof( tVector4 ) );
                
                // size
				float fSizeInc = (float)( rand() % 100 ) * 0.01f * pEmitter->mParticleSizeRanges.fX;
				fSizeInc -= ( pEmitter->mParticleSizeRanges.fX * 0.5f );

				if( pEmitter->mParticleSizeStart.fX - fSizeInc < 0.0f )
				{
					fSizeInc = -pEmitter->mParticleSizeStart.fX;
				}

                pParticle->mSizeInc.fX = fSizeInc;
				pParticle->mSizeInc.fY = fSizeInc;
				pParticle->mSizeInc.fZ = fSizeInc;
                
                // apply speed
                pParticle->mDirection.fX *= pEmitter->mfParticleSpeed;
                pParticle->mDirection.fY *= pEmitter->mfParticleSpeed;
                pParticle->mDirection.fZ *= pEmitter->mfParticleSpeed;
                
                pParticle->mCurrDirection.fX *= pEmitter->mfParticleSpeed;
                pParticle->mCurrDirection.fY *= pEmitter->mfParticleSpeed;
                pParticle->mCurrDirection.fZ *= pEmitter->mfParticleSpeed;
                
/*OUTPUT( "particle %d dir ( %.2f, %.2f, %.2f )\n",
        i,
        pParticle->mDirection.fX,
        pParticle->mDirection.fY,
        pParticle->mDirection.fZ );
*/

                pParticle->miIndex = pEmitter->miCurrParticle;
                pEmitter->miCurrParticle = ( pEmitter->miCurrParticle + 1 ) % pEmitter->miMaxParticles;
				
            }
            else
            {
                pParticle->mPosition.fX += ( -pEmitter->mParticlePositionRanges.fX * 0.5f + (float)( rand() % 100 ) * 0.01f * pEmitter->mParticlePositionRanges.fX );
                pParticle->mPosition.fY += ( -pEmitter->mParticlePositionRanges.fY * 0.5f + (float)( rand() % 100 ) * 0.01f * pEmitter->mParticlePositionRanges.fY );
                pParticle->mPosition.fZ += ( -pEmitter->mParticlePositionRanges.fZ * 0.5f + (float)( rand() % 100 ) * 0.01f * pEmitter->mParticlePositionRanges.fZ );
                
                memcpy( &pParticle->mSize, &pEmitter->mParticleSize, sizeof( tVector4 ) );
                pParticle->mSize.fX += ( -pEmitter->mParticleSizeRanges.fX * 0.5f + (float)( rand() % 100 ) * 0.01f * pEmitter->mParticleSizeRanges.fX );
                //pParticle->mSize.fY += ( -pEmitter->mParticleSizeRanges.fY * 0.5f + (float)( rand() % 100 ) * 0.01f * pEmitter->mParticleSizeRanges.fY );
                //pParticle->mSize.fZ += ( -pEmitter->mParticleSizeRanges.fZ * 0.5f + (float)( rand() % 100 ) * 0.01f * pEmitter->mParticleSizeRanges.fZ );
                pParticle->mSize.fY = pParticle->mSize.fX;

                float fAngleX = -pEmitter->mEmitterAngleRanges.fX * 0.5f + (float)( rand() % 100 ) * 0.01f * pEmitter->mEmitterAngleRanges.fX;
                float fAngleY = -pEmitter->mEmitterAngleRanges.fY * 0.5f + (float)( rand() % 100 ) * 0.01f * pEmitter->mEmitterAngleRanges.fY;
                float fAngleZ = -pEmitter->mEmitterAngleRanges.fZ * 0.5f + (float)( rand() % 100 ) * 0.01f * pEmitter->mEmitterAngleRanges.fZ;

                tMatrix44 rotMatrixX, rotMatrixY, rotMatrixZ, rotMatrixZY, rotMatrix;
                tVector4 rotateDir = { 0.0f, 0.0f, 0.0f, 1.0f };
                Matrix44RotateX( &rotMatrixX, fAngleX );
                Matrix44RotateX( &rotMatrixY, fAngleY );
                Matrix44RotateY( &rotMatrixZ, fAngleZ );
                Matrix44Multiply( &rotMatrixZY, &rotMatrixZ, &rotMatrixY );
                Matrix44Multiply( &rotMatrix, &rotMatrixZY, &rotMatrixX );
                
                // direction of emittance
                tVector4 normDir = { 0.0f, 0.0f, 0.0f, 1.0f };
                Vector4Normalize( &normDir, &pEmitter->mDirection );
                Matrix44Transform( &rotateDir, &normDir, &rotMatrix );
                
                memcpy( &pParticle->mDirection, &rotateDir, sizeof( tVector4 ) );
                memcpy( &pParticle->mCurrDirection, &pParticle->mDirection, sizeof( tVector4 ) );

                pParticle->mCurrDirection.fX *= pEmitter->mfParticleSpeed;
                pParticle->mCurrDirection.fY *= pEmitter->mfParticleSpeed;
                pParticle->mCurrDirection.fZ *= pEmitter->mfParticleSpeed;
                
                // angle of particle
                memcpy( &pParticle->mStartAngle,
                        &pEmitter->mParticleAngleStart,
                        sizeof( tVector4 ) );
                
                // random angles within ranges
                pParticle->mStartAngle.fX += ( -pEmitter->mParticleAngleRanges.fX * 0.5f + pEmitter->mParticleAngleRanges.fX * (float)( rand() % 100 ) * 0.01f );
                pParticle->mStartAngle.fY += ( -pEmitter->mParticleAngleRanges.fY * 0.5f + pEmitter->mParticleAngleRanges.fY * (float)( rand() % 100 ) * 0.01f );
                pParticle->mStartAngle.fZ += ( -pEmitter->mParticleAngleRanges.fZ * 0.5f + pEmitter->mParticleAngleRanges.fZ * (float)( rand() % 100 ) * 0.01f );
                
            }
            
            pParticle->mfStartTime = pEmitter->mfCurrTime;
            pParticle->mfSpeed = pEmitter->mfParticleSpeed;
            
            pEmitter->mfLastSpawnedParticleTime = pEmitter->mfCurrTime;
            ++pEmitter->miNumParticles;

			break;
		}
		
	}	// for i = 0 to num particles
}