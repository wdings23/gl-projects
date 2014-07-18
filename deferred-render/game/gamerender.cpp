#include "gamerender.h"
#include "assetloader.h"
#include "jobmanager.h"
#include "render.h"
#include "batchsprite.h"
#include "hud.h"
#include "fontmanager.h"
#include "textureatlasmanager.h"
#include "glee.h"
#include "filepathutil.h"
#include "LodePNG.h"

struct LevelJobData
{
    tLevel*         mpLevel;
    char            mszFileName[128];

	void			(*mpfnCallback)( void );
};

typedef struct LevelJobData tLevelJobData;

static GLuint siPositionBuffer;
static GLuint siColorBuffer;
static GLuint siScalingBuffer;
static GLuint siTranslationBuffer;
static GLuint siNormalBuffer;
static GLuint siIndexBuffer;

static float* saColors;
static float* saScaling;
static float* saTranslation;

static GLuint siDeferredFBO;
static GLuint siPositionTexture;
static GLuint siAlbedoTexture;
static GLuint siDepthTexture;
static GLuint siNormalTexture;

static GLuint siLightFBO; 
static GLuint siLightTexture;
static GLuint siLightDepthTexture;
static GLuint siLightStencilBuffer;

static GLuint siCylinderBuffer;
static GLuint siCylinderIndexBuffer;

static GLuint siInstanceVBO;
static GLuint siInstanceInfoVBO;
static GLuint siModelVertexArray;

static int siNumCylinderTris;

static int siNumCubes = 100000;
static CCamera const* spCamera;

const float gfLightRadius = 5.0f;

/*
**
*/
static void createSphere( GLfloat** paPos, 
						  GLuint** paiIndices,
						  int* piNumPos,
						  int* piNumTris,
						  float fRadius,
						  int iNumSegments )
{
	*piNumPos = iNumSegments * ( iNumSegments - 1 ) + 2;
	*piNumTris = ( ( iNumSegments - 1 ) * iNumSegments ) * 2;
	
	*paPos = (GLfloat *)malloc( *piNumPos * 4 * sizeof( float ) );
	*paiIndices = (GLuint *)malloc( *piNumTris * 3 * sizeof( GLuint ) );
	
	GLuint* aiIndices = *paiIndices;
	GLfloat* pfVal = *paPos;
	
	float fTwoPI = 2.0f * 3.14159f; 
	float fYPerSegment = fRadius / (float)( iNumSegments >> 1 );
	float fAnglePerSegment = fTwoPI / (float)iNumSegments;
	
	GLuint iCount = 0;
	int iTriangle = 0;
	for( int iY = 0; iY <= iNumSegments; iY++ )
	{
		float fY = fRadius - (float)iY * fYPerSegment;
		float fCurrRadius = sqrtf( fRadius * fRadius - fabs( fY ) * fabs( fY ) );

		OUTPUT( "segment %d\n", iY );

		if( fCurrRadius <= 0.0f )
		{
			*pfVal++ = 0.0f;
			*pfVal++ = fY;
			*pfVal++ = 0.0f;
			*pfVal++ = 1.0f;

			OUTPUT( "%d ( %f, %f, %f )\n", iCount, *(pfVal-4), *(pfVal-3), *(pfVal-2) );
			++iCount;
		}
		else
		{
			for( int iX = 0; iX < iNumSegments; iX++ )
			{
				// original vector at ( 1, 0 ), rotate about y axis

				float fCurrAngle = (float)iX * fAnglePerSegment;
				*pfVal++ = cosf( fCurrAngle ) * fCurrRadius;	// x
				*pfVal++ = fY;
				*pfVal++ = -sinf( fCurrAngle ) * fCurrRadius;	// z
				*pfVal++ = 1.0f;

				OUTPUT( "%d ( %f, %f, %f )\n", iCount, *(pfVal-4), *(pfVal-3), *(pfVal-2) );
				++iCount;
				
				if( iY == 1 )
				{
					if( iX > 0 )
					{
						*aiIndices++ = iCount - 2;
						*aiIndices++ = iCount - 1;
						*aiIndices++ = 0;
					
						OUTPUT( "triangle %d ( %d, %d, %d )\n", iTriangle, *(aiIndices-3), *(aiIndices-2), *(aiIndices-1) ); 
						++iTriangle;
					}
				}
				else
				{
					if( iX > 0 )
					{
						*aiIndices++ = iCount - iNumSegments - 2;
						*aiIndices++ = iCount - iNumSegments - 1;
						*aiIndices++ = iCount - 2;
						
						OUTPUT( "triangle %d ( %d, %d, %d )\n", iTriangle, *(aiIndices-3), *(aiIndices-2), *(aiIndices-1) ); 
						++iTriangle;

						*aiIndices++ = iCount - iNumSegments - 1;
						*aiIndices++ = iCount - 1;
						*aiIndices++ = iCount - 2;
						

						OUTPUT( "triangle %d ( %d, %d, %d )\n", iTriangle, *(aiIndices-3), *(aiIndices-2), *(aiIndices-1) ); 
						++iTriangle;
					}
				}
			}	// for x = 0 to num segments

			int iStartIndex = ( iY - 1 ) * iNumSegments + 1;

			// add the last triangle or quad
			if( iY == 1 )
			{
				*aiIndices++ = iCount - 1;
				*aiIndices++ = iStartIndex;
				*aiIndices++ = 0;
				
				OUTPUT( "triangle %d ( %d, %d, %d )\n", iTriangle, *(aiIndices-3), *(aiIndices-2), *(aiIndices-1) ); 
				++iTriangle;
			}
			else
			{
				*aiIndices++ = iCount - 1;
				*aiIndices++ = iStartIndex;
				*aiIndices++ = iStartIndex - iNumSegments;
				
				OUTPUT( "triangle %d ( %d, %d, %d )\n", iTriangle, *(aiIndices-3), *(aiIndices-2), *(aiIndices-1) ); 
				++iTriangle;

				*aiIndices++ = iCount - 1;
				*aiIndices++ = iStartIndex - iNumSegments;
				*aiIndices++ = iStartIndex - 1;
				
				OUTPUT( "triangle %d ( %d, %d, %d )\n", iTriangle, *(aiIndices-3), *(aiIndices-2), *(aiIndices-1) ); 
				++iTriangle;
			}

		}	// if current radius > 0

		if( iY == iNumSegments )
		{
			int iStartIndex = ( iY - 2 ) * iNumSegments + 1;
			for( int iX = 0; iX < iNumSegments; iX++ )
			{
				if( iX > 0 )
				{
					*aiIndices++ = iStartIndex + iX - 1;
					*aiIndices++ = iStartIndex + iX;
					*aiIndices++ = *piNumPos - 1;	

					OUTPUT( "triangle %d ( %d, %d, %d )\n", iTriangle, *(aiIndices-3), *(aiIndices-2), *(aiIndices-1) ); 
					++iTriangle;
				}
			}

			*aiIndices++ = iStartIndex;
			*aiIndices++ = iCount - 2;
			*aiIndices++ = *piNumPos - 1;

			OUTPUT( "triangle %d ( %d, %d, %d )\n", iTriangle, *(aiIndices-3), *(aiIndices-2), *(aiIndices-1) ); 
			++iTriangle;
		}

	}	// for y = 0 to num segments

	// last vertex
	*pfVal++ = 0.0f;
	*pfVal++ = -fRadius;
	*pfVal++ = 0.0f;
	*pfVal++ = 1.0f;
	
	++iCount;
}

struct InstanceVertex
{
	tVector4	mPos;
	tVector4	mNormal;
	tVector2	mUV;
};

typedef struct InstanceVertex tInstanceVertex;

struct InstanceInfo
{
	tVector4		mTranslation;
	tVector4		mScaling;
	tVector4		mColor;
};

typedef struct InstanceInfo tInstanceInfo;

/*
**
*/
static void initInstancing( void )
{
	GLfloat aCubePos[] =
	{
		-0.5f, -0.5f, 0.5f, 1.0f,		
		0.5f, -0.5f, 0.5f, 1.0f,		
		-0.5f, 0.5f, 0.5f, 1.0f,		
		0.5f, 0.5f, 0.5f, 1.0f,

		-0.5f, 0.5f, -0.5f, 1.0f,		
		0.5f, 0.5f, -0.5f, 1.0f,		
		-0.5f, -0.5f, -0.5f, 1.0f,		
		0.5f, -0.5f, -0.5f, 1.0f,
	};

	GLfloat aNormals[] = 
	{
		1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, 0.0f, 0.0f, 1.0f,

		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, -1.0f, 0.0f, 1.0f,

		0.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, -1.0f, 1.0f,
	};

	GLfloat aUV[] = 
	{
		0.0f, 0.0f,
	};

	GLint aiIndices[] = 
	{
		// back
		0, 1, 2,	
		2, 1, 3,
		
		// top
		2, 3, 4,
		4, 3, 5,
		
		// front
		4, 5, 6,
		6, 5, 7,
		
		// bottom
		6, 7, 0,
		0, 7, 1,
		
		// right
		1, 7, 3,
		3, 7, 5,
		
		// left
		6, 0, 4,
		4, 0, 2,
	};

	GLint aiNormIndices[] = 
	{
		4, 4, 4,
		4, 4, 4,

		2, 2, 2,
		2, 2, 2,

		5, 5, 5,
		5, 5, 5,

		3, 3, 3,
		3, 3, 3,

		0, 0, 0,
		0, 0, 0,

		1, 1, 1,
		1, 1, 1,
	};
	
	for( int i = 0; i < 12; i++ )
	{
		int iIndex = i * 3;
		int iTemp = aiIndices[iIndex];
		aiIndices[iIndex] = aiIndices[iIndex+2];
		aiIndices[iIndex+2] = iTemp;
	}

	srand( time( NULL ) );
	
	int iNumIndices = sizeof( aiIndices ) / sizeof( *aiIndices );
	tInstanceVertex* aVerts = (tInstanceVertex *)malloc( sizeof( tInstanceVertex ) * iNumIndices );
	for( int i = 0; i < iNumIndices; i++ )
	{
		int iPosIndex = aiIndices[i] * 4;
		int iNormIndex = aiNormIndices[i] * 4;

		memcpy( &aVerts[i].mPos, &aCubePos[iPosIndex], sizeof( tVector4 ) );
		memcpy( &aVerts[i].mNormal, &aNormals[iNormIndex], sizeof( tVector4 ) );
		memcpy( &aVerts[i].mUV, &aUV[0], sizeof( tVector2 ) );
	}
	
	int* aiVBOIndices = (int *)malloc( sizeof( int ) * iNumIndices );
	for( int i = 0; i < iNumIndices; i++ )
	{
		aiVBOIndices[i] = i;
	}

	char szFullPath[256];
	getFullPath( szFullPath, "coin.png" );

	unsigned char* acImage = NULL;
	unsigned int iWidth = 0, iHeight = 0;
	LodePNG_decode32_file( &acImage, &iWidth, &iHeight, szFullPath );
	siNumCubes = iWidth * iHeight;

	tInstanceInfo* aInstanceInfo = (tInstanceInfo *)malloc( sizeof( tInstanceInfo ) * siNumCubes );

	unsigned char* pacImage = acImage;
	int iIndex = 0;
	for( int iY = 0; iY < (int)iHeight; iY++ )
	{
		for( int iX = 0; iX < (int)iWidth; iX++ )
		{
			float fRed = (float)*pacImage / 255.0f; 
			++pacImage;
			float fGreen = (float)*pacImage / 255.0f;
			++pacImage;
			float fBlue = (float)*pacImage / 255.0f;
			++pacImage;
			float fAlpha = (float)*pacImage / 255.0f;
			++pacImage;

			if( fAlpha > 0.0f )
			{
				float fScaling = 0.25f;

				float fX = (float)( iX - (int)( iWidth / 2 ) ) * fScaling;
				float fY = (float)( (int)( iHeight / 2 ) - iY ) * fScaling;
				float fZ = 50.0f - (float)( rand() % 5 );

				aInstanceInfo[iIndex].mColor.fX = fRed;
				aInstanceInfo[iIndex].mColor.fY = fGreen;
				aInstanceInfo[iIndex].mColor.fZ = fBlue;
				aInstanceInfo[iIndex].mColor.fW = 1.0f;

				aInstanceInfo[iIndex].mScaling.fX = fScaling;
				aInstanceInfo[iIndex].mScaling.fY = fScaling;
				aInstanceInfo[iIndex].mScaling.fZ = fScaling;
				aInstanceInfo[iIndex].mScaling.fW = 1.0f;

				aInstanceInfo[iIndex].mTranslation.fX = fX;
				aInstanceInfo[iIndex].mTranslation.fY = fY;
				aInstanceInfo[iIndex].mTranslation.fZ = fZ;
				aInstanceInfo[iIndex].mTranslation.fW = 1.0f;
			}
			
			++iIndex;
		}
	}
	
	int iPosAttrib = 0;
	int iNormAttrib = 1;
	int iColorAttrib = 2;
	int iScaleAttrib = 3;
	int iTransAttrib = 4;
	
	glGenBuffers( 1, &siInstanceVBO );
	glBindBuffer( GL_ARRAY_BUFFER, siInstanceVBO );
	glBufferData( GL_ARRAY_BUFFER, sizeof( tInstanceVertex ) * iNumIndices, aVerts, GL_STATIC_DRAW );
	
	glEnableVertexAttribArray( iPosAttrib );
	glVertexAttribPointer( iPosAttrib, 4, GL_FLOAT, GL_FALSE, sizeof( tInstanceVertex ), NULL );
	
	glEnableVertexAttribArray( iNormAttrib );
	glVertexAttribPointer( iNormAttrib, 4, GL_FLOAT, GL_FALSE, sizeof( tInstanceVertex ), (void *)sizeof( tVector2 ) );
	
	// indices
	glGenBuffers( 1, &siIndexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, siIndexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( int ) * iNumIndices, aiVBOIndices, GL_STATIC_DRAW );
	
	free( aiVBOIndices );
	free( aVerts );

	glGenBuffers( 1, &siInstanceInfoVBO );
	glBindBuffer( GL_ARRAY_BUFFER, siInstanceInfoVBO );
	glBufferData( GL_ARRAY_BUFFER, sizeof( tInstanceInfo ) * siNumCubes, aInstanceInfo, GL_DYNAMIC_DRAW );
	
	glEnableVertexAttribArray( iTransAttrib );
	glVertexAttribPointer( iTransAttrib, 4, GL_FLOAT, GL_FALSE, sizeof( tInstanceInfo ), NULL );
	glVertexAttribDivisor( iTransAttrib, 1 );

	glEnableVertexAttribArray( iScaleAttrib );
	glVertexAttribPointer( iScaleAttrib, 4, GL_FLOAT, GL_FALSE, sizeof( tInstanceInfo ), (void *)sizeof( tVector4 ) );
	glVertexAttribDivisor( iScaleAttrib, 1 );

	glEnableVertexAttribArray( iColorAttrib );
	glVertexAttribPointer( iColorAttrib, 4, GL_FLOAT, GL_FALSE, sizeof( tInstanceInfo ), (void *)( sizeof( tVector4 ) + sizeof( tVector4 ) ) );
	glVertexAttribDivisor( iColorAttrib, 1 );

	free( aInstanceInfo );

	// deferred fbo textures
	{
		int iFBWidth = (int)( (float)renderGetScreenWidth() * renderGetScreenScale() );
		int iFBHeight = (int)( (float)renderGetScreenHeight() * renderGetScreenScale() );

		glGenFramebuffers( 1, &siDeferredFBO );
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, siDeferredFBO );
		
		// albedo 
		glGenTextures( 1, &siAlbedoTexture );
		glBindTexture( GL_TEXTURE_2D, siAlbedoTexture );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, iFBWidth, iFBHeight, 0, GL_RGBA, GL_FLOAT, NULL );
		glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, siAlbedoTexture, 0);
		
		// position 
		glGenTextures( 1, &siPositionTexture );
		glBindTexture( GL_TEXTURE_2D, siPositionTexture );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, iFBWidth, iFBHeight, 0, GL_RGBA, GL_FLOAT, NULL );
		glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, siPositionTexture, 0);

		// normal
		glGenTextures( 1, &siNormalTexture );
		glBindTexture( GL_TEXTURE_2D, siNormalTexture );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, iFBWidth, iFBHeight, 0, GL_RGBA, GL_FLOAT, NULL );
		glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, siNormalTexture, 0);

		// depth
		glGenTextures( 1, &siDepthTexture );
		glBindTexture( GL_TEXTURE_2D, siDepthTexture );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, iFBWidth, iFBHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
		glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, siDepthTexture, 0);
		
		GLenum aBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers( 3, aBuffers );
		
		// fbo for spot lights
		glGenFramebuffers( 1, &siLightFBO );
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, siLightFBO );

		glGenTextures( 1, &siLightTexture );
		glBindTexture( GL_TEXTURE_2D, siLightTexture );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, iFBWidth, iFBHeight, 0, GL_RGBA, GL_FLOAT, NULL );
		glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, siLightTexture, 0);
		
		glGenTextures( 1, &siLightDepthTexture );
		glBindTexture( GL_TEXTURE_2D, siLightDepthTexture );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, iFBWidth, iFBHeight, 0, GL_RGBA, GL_FLOAT, NULL );
		glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, siLightDepthTexture, 0);

		glGenRenderbuffers( 1, &siLightStencilBuffer );
		glBindRenderbuffer( GL_RENDERBUFFER, siLightStencilBuffer );
		glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, iFBWidth, iFBHeight );
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, siLightStencilBuffer );
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, siLightStencilBuffer );
		
		glDrawBuffers( 1, aBuffers );

		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
		glBindRenderbuffer( GL_RENDERBUFFER, 0 );
	}

	GLfloat* aSpherePos = NULL;
	GLuint* aiSphereIndices = NULL;
	int iNumPos = 0;
	int iNumTris = 0;

	createSphere( &aSpherePos, &aiSphereIndices, &iNumPos, &iNumTris, gfLightRadius, 8 );
	siNumCylinderTris = iNumTris * 3;

	// positions
	glGenBuffers( 1, &siCylinderBuffer );
	glBindBuffer( GL_ARRAY_BUFFER, siCylinderBuffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * iNumPos * 4, aSpherePos, GL_STATIC_DRAW );
	glEnableVertexAttribArray( iPosAttrib );
	glVertexAttribPointer( iPosAttrib, 4, GL_FLOAT, GL_FALSE, 0, NULL );

	// indices
	glGenBuffers( 1, &siCylinderIndexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, siCylinderIndexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( int ) * iNumTris * 3, aiSphereIndices, GL_STATIC_DRAW );

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );	

	// random direction texture
	CTextureManager::instance()->registerTexture( "random_dir.tga" );
}

/*
**
*/
static void drawInstances( CCamera const* pCamera, GLuint iFBO, GLuint iShader )
{
	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, iFBO );
	glUseProgram( iShader );
	
	tMatrix44 const* pViewMatrix = pCamera->getViewMatrix();
	tMatrix44 const* pProjMatrix = pCamera->getProjectionMatrix();

	GLint iViewMatrix = glGetUniformLocation( iShader, "view_matrix" );
	GLint iProjMatrix = glGetUniformLocation( iShader, "projection_matrix" );

	tMatrix44 viewMatrix, projMatrix;
	Matrix44Transpose( &viewMatrix, pViewMatrix );
	Matrix44Transpose( &projMatrix, pProjMatrix );

	glUniformMatrix4fv( iViewMatrix, 1, GL_FALSE, viewMatrix.afEntries ); 
	glUniformMatrix4fv( iProjMatrix, 1, GL_FALSE, projMatrix.afEntries ); 

	int iPosAttrib = 0;
	int iNormAttrib = 1;
	int iColorAttrib = 2;
	int iScaleAttrib = 3;
	int iTransAttrib = 4;

	// enable attribs and set ptr	
	glBindBuffer( GL_ARRAY_BUFFER, siInstanceInfoVBO );
	
	glEnableVertexAttribArray( iTransAttrib );
	glVertexAttribPointer( iTransAttrib, 4, GL_FLOAT, GL_FALSE, sizeof( tInstanceInfo ), NULL );

	glEnableVertexAttribArray( iScaleAttrib );
	glVertexAttribPointer( iScaleAttrib, 4, GL_FLOAT, GL_FALSE, sizeof( tInstanceInfo ), (void *)sizeof( tVector4 ) );

	glEnableVertexAttribArray( siColorBuffer );
	glVertexAttribPointer( siColorBuffer, 4, GL_FLOAT, GL_FALSE, sizeof( tInstanceInfo ), (void *)( sizeof( tVector4 ) + sizeof( tVector4 ) ) );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, siIndexBuffer );
	glBindBuffer( GL_ARRAY_BUFFER, siInstanceVBO );
	
	glEnableVertexAttribArray( iPosAttrib );
	glVertexAttribPointer( iPosAttrib, 4, GL_FLOAT, GL_FALSE, sizeof( tInstanceVertex ), NULL );
	
	glEnableVertexAttribArray( iNormAttrib );
	glVertexAttribPointer( iNormAttrib, 4, GL_FLOAT, GL_FALSE, sizeof( tInstanceVertex ), (void *)sizeof( tVector4 ) );

	//glBindVertexArray( siModelVertexArray ); 

	// draw
	glDrawElementsInstancedEXT( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, siNumCubes );

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );
}

/*
**
*/
static void drawLightModel( void )
{
	GLuint iInstanceShader = CShaderManager::instance()->getShader( "instance_deferred" );
	WTFASSERT2( iInstanceShader > 0, "invalid shader" );

	GLuint iLightShader = CShaderManager::instance()->getShader( "light_model" );
	WTFASSERT2( iLightShader > 0, "invalid shader" );

	GLuint iFBO = siLightFBO;
	//GLuint iFBO = 0;
	
	glCullFace( GL_BACK );

	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, iFBO );
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	// draw scene for depth values
	glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );	// don't write to color
	drawInstances( spCamera, iFBO, iInstanceShader );

	static tVector4 saLightPos[4] = 
	{
		{ 0.0f, 0.0f, 0.0f, 1.0f },
		{ -20.0f, 0.0f, 25.0f, 1.0f },
		{ 10.0f, -5.0f, 20.0f, 1.0f },
		{ -10.0f, 5.0f, 10.0f, 1.0f },
	};

	static tVector4 saLightColor[4] = 
	{
		{ 0.0f, 1.0f, 0.0f, 1.0f },
		{ 1.0f, 0.0f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 0.0f, 1.0f },
	};

	// matrix
	tMatrix44 const* pViewMatrix = spCamera->getViewMatrix();
	tMatrix44 const* pProjMatrix = spCamera->getProjectionMatrix();

	glEnable( GL_BLEND );
	glBlendFunc( GL_ONE, GL_ONE );

	// disable writing to depth buffer
	glDepthMask( GL_FALSE );

	for( int i = 0; i < 4; i++ )
	{
		glUseProgram( iLightShader );
		saLightPos[i].fX += 0.01f;

		tMatrix44 modelViewMatrix, translateMatrix;
		Matrix44Translate( &translateMatrix, &saLightPos[i] );
		Matrix44Multiply( &modelViewMatrix, pViewMatrix, &translateMatrix );

		tMatrix44 viewMatrix, projMatrix;
		Matrix44Transpose( &viewMatrix, &modelViewMatrix );
		Matrix44Transpose( &projMatrix, pProjMatrix );

		// set matrices
		GLint iViewMatrix = glGetUniformLocation( iLightShader, "view_matrix" );
		GLint iProjMatrix = glGetUniformLocation( iLightShader, "projection_matrix" );

		glUniformMatrix4fv( iViewMatrix, 1, GL_FALSE, viewMatrix.afEntries ); 
		glUniformMatrix4fv( iProjMatrix, 1, GL_FALSE, projMatrix.afEntries ); 

		GLint iLightColor = glGetUniformLocation( iLightShader, "lightColor" );
		//WTFASSERT2( iLightColor >= 0, "can't find lightColor semantic" );
		glUniform4f( iLightColor, saLightColor[i].fX, saLightColor[i].fY, saLightColor[i].fZ, saLightColor[i].fW );
		
		GLint iPos = glGetAttribLocation( iLightShader, "position" ); 
		
//glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

		// bind vbo
		glBindBuffer( GL_ARRAY_BUFFER, siCylinderBuffer );
		glEnableVertexAttribArray( iPos );
		glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, NULL );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, siCylinderIndexBuffer );
		
		glEnable( GL_DEPTH_TEST );
		glEnable( GL_STENCIL_TEST );
		glEnable( GL_CULL_FACE );
		
		glStencilFunc( GL_ALWAYS, 0, 0x0 );				// always pass (write to every pixel)
		
		glDisable( GL_CULL_FACE );
		glStencilOpSeparate( GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP );
		glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP );

		// render light into stencil
		glDrawElements( GL_TRIANGLES, siNumCylinderTris, GL_UNSIGNED_INT, 0 );
		
		// render light color
		{
			glEnable( GL_CULL_FACE );
			glDisable( GL_DEPTH_TEST );

			glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
			glStencilFunc( GL_NOTEQUAL, 0, 0xFF );
			//glStencilFunc( GL_ALWAYS, 1, 0xFF );
			glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
			
			int iLightAttenShader = CShaderManager::instance()->getShader( "light_attenuation" );
			glUseProgram( iLightAttenShader );
			
			// position texture
			GLuint iPosTex = glGetUniformLocation( iLightShader, "posTex" );
			glActiveTexture( GL_TEXTURE0 );
			glUniform1i( iPosTex, 0 );
			glBindTexture( GL_TEXTURE_2D, siPositionTexture );

			// light's color
			GLint iLightAttenColor = glGetUniformLocation( iLightAttenShader, "lightColor" );
			WTFASSERT2( iLightAttenColor >= 0, "invalid semantic lightColor" );
			glUniform4f( iLightAttenColor, saLightColor[i].fX, saLightColor[i].fY, saLightColor[i].fZ, saLightColor[i].fW );

			GLint iRadius = glGetUniformLocation( iLightAttenShader, "fRadius" );
			//WTFASSERT2( iRadius >= 0, "invalid semantic fRadius" );
			glUniform1f( iRadius, gfLightRadius );

			GLint iLightAttenPos = glGetUniformLocation( iLightAttenShader, "lightPos" );
			//WTFASSERT2( iLightAttenPos >= 0, "invalid semantic lightPos" );
			glUniform4f( iLightAttenPos, saLightPos[i].fX, saLightPos[i].fY, saLightPos[i].fZ, saLightPos[i].fW );

			// set matrices
			iViewMatrix = glGetUniformLocation( iLightAttenShader, "view_matrix" );
			iProjMatrix = glGetUniformLocation( iLightAttenShader, "projection_matrix" );

			glUniformMatrix4fv( iViewMatrix, 1, GL_FALSE, viewMatrix.afEntries ); 
			glUniformMatrix4fv( iProjMatrix, 1, GL_FALSE, projMatrix.afEntries ); 

			GLint iPos = glGetAttribLocation( iInstanceShader, "position" ); 
			
			for( int j = 0; j < 4; j++ )
			{
				glActiveTexture( GL_TEXTURE1 + j );
				glBindTexture( GL_TEXTURE_2D, 0 );
			}

			// bind vbo
			glBindBuffer( GL_ARRAY_BUFFER, siCylinderBuffer );
			glEnableVertexAttribArray( iPos );
			glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, 0, NULL );
			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, siCylinderIndexBuffer );

			glDrawElements( GL_TRIANGLES, siNumCylinderTris, GL_UNSIGNED_INT, 0 );

			glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
		}

	}	// for i = 0 to num lights
	
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	//glDisable( GL_BLEND );
	
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

	glDepthMask( GL_TRUE );
	glStencilFunc( GL_ALWAYS, 0, 0xFF );
	glDisable( GL_STENCIL_TEST );

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
	
}

/*
**
*/
static void drawDeferredScene( void )
{
	//int iShader = CShaderManager::instance()->getShader( "deferred_shading" );
	int iShader = CShaderManager::instance()->getShader( "ambient_occlusion" );
	WTFASSERT2( iShader > 0, "invalid shader" );

	glUseProgram( iShader );

	// textures
	GLuint iAlbedoTex = glGetUniformLocation( iShader, "albedoTex" );
	glActiveTexture( GL_TEXTURE0 );
	glUniform1i( iAlbedoTex, 0 );
	glBindTexture( GL_TEXTURE_2D, siAlbedoTexture );

	GLuint iNormalTex = glGetUniformLocation( iShader, "normalTex" );
	glActiveTexture( GL_TEXTURE1 );
	glUniform1i( iNormalTex, 1 );
	glBindTexture( GL_TEXTURE_2D, siNormalTexture );

	GLuint iLightTex = glGetUniformLocation( iShader, "lightTex" );
	glActiveTexture( GL_TEXTURE2 );
	glUniform1i( iLightTex, 2 );
	glBindTexture( GL_TEXTURE_2D, siLightTexture );

	GLuint iDepthTex = glGetUniformLocation( iShader, "depthTex" );
	glActiveTexture( GL_TEXTURE3 );
	glUniform1i( iDepthTex, 3 );
	glBindTexture( GL_TEXTURE_2D, siDepthTexture );

	GLuint iPosTex = glGetUniformLocation( iShader, "posTex" );
	glActiveTexture( GL_TEXTURE4 );
	glUniform1i( iPosTex, 4 );
	glBindTexture( GL_TEXTURE_2D, siPositionTexture );

	tTexture* pRandomTexture = CTextureManager::instance()->getTexture( "random_dir.tga" );

	GLuint iRandomDirTex = glGetUniformLocation( iShader, "randomDirTex" );
	glActiveTexture( GL_TEXTURE5 );
	glUniform1i( iRandomDirTex, 5 );
	glBindTexture( GL_TEXTURE_2D, pRandomTexture->miID );

	tVector4 aScreenVerts[] =
    {
		{ -1.0f, -1.0f, 0.0f, 1.0f },
		{ 1.0f, -1.0f, 0.0f, 1.0f },
		{ -1.0f, 1.0f, 0.0f, 1.0f },
		{ 1.0f, 1.0f, 0.0f, 1.0f },
    };
    
	tVector2 aUV[] = 
	{
		{ 0.0f, 0.0f },
		{ 1.0, 0.0f },
		{ 0.0f, 1.0f },
		{ 1.0f, 1.0f }
	};

	//glDisable( GL_CULL_FACE );

	int iPosition = glGetAttribLocation( iShader, "position" );
	int iUV = glGetAttribLocation( iShader, "uv" );

	glVertexAttribPointer( iPosition, 4, GL_FLOAT, GL_FALSE, 0, aScreenVerts );
    glEnableVertexAttribArray( 0 );
    
    glVertexAttribPointer( iUV, 2, GL_FLOAT, GL_FALSE, 0, aUV );
    glEnableVertexAttribArray( 1 );
    
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

	//glEnable( GL_CULL_FACE );
}

/*
**
*/
CGameRender::CGameRender( void )
{
}

/*
**
*/
CGameRender::~CGameRender( void )
{
}

/*
**
*/
void CGameRender::init( void )
{
    glClearDepth( 1.0f );
    glDepthRange( 0.0f, 1.0f );
    glDepthMask( GL_TRUE );
    glDepthFunc( GL_LEQUAL );
    glEnable( GL_DEPTH_TEST );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    
	CShaderManager::instance()->loadAllPrograms( "all_shaders.txt", setAttribs );
	
    // initialize texture manager
    CTextureManager::instance();
    
	CFontManager::instance()->loadFonts( "fonts.txt" );
    mFont.readFile( "Verdana.fnt" );
	CHUD::instance()->load( "hud.txt" );
	CHUD::instance()->enter();
	
	miShaderIndex = 0;
	mpCurrShaderProgram = CShaderManager::instance()->getShaderProgram( miShaderIndex );
	CHUD::instance()->setShaderName( mpCurrShaderProgram->mszName );

	initInstancing();
}

/*
**
*/
void CGameRender::draw( float fDT )
{
	spCamera = mpCamera;
		
	// get the color, position, normal and depth textures
	GLuint iInstanceShader = CShaderManager::instance()->getShader( "instance_deferred" );
	WTFASSERT2( iInstanceShader > 0, "invalid shader" );
	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, siDeferredFBO );
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
	drawInstances( mpCamera, siDeferredFBO, iInstanceShader );
	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );

	drawLightModel();
	drawDeferredScene();
	
	glDisable( GL_CULL_FACE );

    char szFPS[64];
    snprintf( szFPS, sizeof( szFPS ), "%.2f", 1.0f / fDT );
    
    float fScreenWidth = renderGetScreenWidth() * renderGetScreenScale();
    float fScreenHeight = renderGetScreenHeight() * renderGetScreenScale();
    
	GLint iShader = CShaderManager::instance()->getShader( "ui" );
    mFont.drawString( szFPS,
                      640.0f,
                      fScreenHeight - 40.0f,
                      20.0f,
                      fScreenWidth,
                      fScreenHeight,
                      1.0f,
                      1.0f,
                      0.0f,
                      1.0f,
                      CFont::ALIGN_RIGHT,
                      true );
    
    CBatchManager::instance()->draw();
	CBatchManager::instance()->reset();
}

/*
**
*/
void CGameRender::updateLevel( void )
{
	
}

/*
**
*/
void CGameRender::getVisibleModels( CCamera const* pCamera, 
								    tVisibleOctNodes const* pVisibleNodes,
								    tVisibleModels* pVisibleModels )
{
	int iNotDrawn = 0;

	tVector4 modelPos = { 0.0f, 0.0f, 0.0f, 1.0f };
	
	std::vector<tModelInstance *> allVisibleModelInstancesToLoad;
	pVisibleModels->miNumModels = 0;
	
	// get visible models
	std::vector<tModel const*> allVisibleModelsToLoad;
	for( int iNode = 0; iNode < pVisibleNodes->miNumNodes; iNode++ )
	{
		tOctNode const* pOctNode = pVisibleNodes->mapNodes[iNode];
		for( int i = 0; i  < pOctNode->miNumObjects; i++ )
		{
			tModelInstance* pModelInstance = (tModelInstance *)pOctNode->mapObjects[i];

			modelPos.fX = pModelInstance->mXFormMat.M( 0, 3 );
			modelPos.fY = pModelInstance->mXFormMat.M( 1, 3 );
			modelPos.fZ = pModelInstance->mXFormMat.M( 2, 3 );
			
			if( pModelInstance->mpModel )
			{
				if( pModelInstance->mpModel->miLoadState == MODEL_LOADSTATE_LOADED )
				{
					// calculate radius
					if( pModelInstance->mfRadius <= 0.0f )
					{
						pModelInstance->mfRadius = pModelInstance->mpModel->mDimension.fX * 0.5f;	
						if( pModelInstance->mpModel->mDimension.fY > pModelInstance->mpModel->mDimension.fX &&
							pModelInstance->mpModel->mDimension.fY > pModelInstance->mpModel->mDimension.fZ )
						{
							pModelInstance->mfRadius = pModelInstance->mpModel->mDimension.fY * 0.5f;
						}
						else if( pModelInstance->mpModel->mDimension.fZ > pModelInstance->mpModel->mDimension.fX &&
								 pModelInstance->mpModel->mDimension.fZ > pModelInstance->mpModel->mDimension.fY )
						{
							pModelInstance->mfRadius = pModelInstance->mpModel->mDimension.fZ * 0.5f;
						}
					}

					// check if model is in frustum
					if(  !pModelInstance->mbDrawn/* &&
						 pCamera->sphereInFrustum( &modelPos, pModelInstance->mfRadius )*/ )
					{
						WTFASSERT2( pModelInstance->mpModel, "invalid model for instance" );
						if( pVisibleModels->miNumModels + 1 >= pVisibleModels->miNumAlloc )
						{
							pVisibleModels->miNumAlloc += NUM_VISIBLE_MODELS_PER_ALLOC;
							pVisibleModels->mapModels = (void **)REALLOC( pVisibleModels->mapModels, sizeof( void* ) * pVisibleModels->miNumAlloc );
						}

						pVisibleModels->mapModels[pVisibleModels->miNumModels++] = pModelInstance;
						
						// set up gl stuff for rendering face colors
						if( pModelInstance->miGLSetupState == MODELINSTANCE_GLSETUP_STATE_NONE )
						{
							allVisibleModelInstancesToLoad.push_back( pModelInstance );
						}

						pModelInstance->mbDrawn = true;
					}
					else
					{
						++iNotDrawn;
					}
				}
				else if( pModelInstance->mpModel->miLoadState == MODEL_LOADSTATE_NOT_LOADED )
				{
					((tModel *)pModelInstance->mpModel)->miLoadState = MODEL_LOADSTATE_LOADING;
					allVisibleModelsToLoad.push_back( pModelInstance->mpModel );
				}
			
			}	// if has model at current lod
			else
			{
				++iNotDrawn;
			}	

		}	// for i = 0 to num objects in oct node
	
	}	// for node in visible nodes
	
	// load visible models
    std::vector<tModel const*>::iterator it = allVisibleModelsToLoad.begin();
    for( ; it != allVisibleModelsToLoad.end(); ++it )
    {
        tModel* pModel = (tModel *)*it;
		assetLoad( pModel, ASSET_MODEL );
	}
	
	// sort
	for( int i = 0; i < pVisibleModels->miNumModels; i++ )
	{
		tModelInstance* pModelInstance = (tModelInstance*)pVisibleModels->mapModels[i];
		tVector4 pos = 
		{
			pModelInstance->mXFormMat.M( 0, 3 ),
			pModelInstance->mXFormMat.M( 1, 3 ),
			pModelInstance->mXFormMat.M( 2, 3 ),
			1.0f
		};

		for( int j = i + 1; j < pVisibleModels->miNumModels; j++ )
		{
			tModelInstance* pCheckModelInstance = (tModelInstance*)pVisibleModels->mapModels[j];
			tVector4 checkPos = 
			{
				pCheckModelInstance->mXFormMat.M( 0, 3 ),
				pCheckModelInstance->mXFormMat.M( 1, 3 ),
				pCheckModelInstance->mXFormMat.M( 2, 3 ),
				1.0f
			};
		
			if( checkPos.fZ > pos.fZ )
			{
				void* pTemp = pVisibleModels->mapModels[i];
				pVisibleModels->mapModels[i] = pVisibleModels->mapModels[j];
				pVisibleModels->mapModels[j] = pTemp;

				pModelInstance = (tModelInstance *)pVisibleModels->mapModels[i];
			}
		}

		pModelInstance->mbDrawn = false;
	}

	/*for( int i = 0; i < pVisibleModels->miNumModels; i++ )
	{
		tModelInstance* pModelInstance = (tModelInstance*)pVisibleModels->mapModels[i];
		tVector4 pos = 
		{
			pModelInstance->mXFormMat.M( 0, 3 ),
			pModelInstance->mXFormMat.M( 1, 3 ),
			pModelInstance->mXFormMat.M( 2, 3 ),
			1.0f
		};
	
		OUTPUT( "pos = ( %f, %f, %f )\n", pos.fX, pos.fY, pos.fZ );
	}*/
}

/*
**
*/
void CGameRender::loadLevelJob( void* pData )
{
    OUTPUT( "!!! LOADING LEVEL !!!\n" );
    
    tLevelJobData* pLevelJobData = (tLevelJobData *)pData;
    
    levelOnDemandLoad( pLevelJobData->mpLevel,
                       pLevelJobData->mszFileName,
                       allocObject );
    
    OUTPUT( "!!! LEVEL LOADED !!!\n" );
}


/*
**
*/
void CGameRender::setAttribs( GLuint iShader )
{
	glBindAttribLocation( iShader, SHADER_ATTRIB_POSITION, "position" );
	glBindAttribLocation( iShader, SHADER_ATTRIB_NORM, "normal" );
	glBindAttribLocation( iShader, SHADER_ATTRIB_UV, "uv" );
	glBindAttribLocation( iShader, SHADER_ATTRIB_COLOR, "color" );

	glBindAttribLocation( iShader, SHADER_ATTRIB_ORIG_POS, "origPos" );
	glBindAttribLocation( iShader, SHADER_ATTRIB_ORIG_NORM, "origNorm" );

	glBindAttribLocation( iShader, SHADER_ATTRIB_LIGHTPOS_0, "lightPos0" );
	glBindAttribLocation( iShader, SHADER_ATTRIB_PROJECTIVE_UV_0, "projectiveUV" );
}

/*
**
*/
void* CGameRender::allocObject( const char* szName, 
							    void* pUserData0, 
							    void* pUserData1, 
							    void* pUserData2, 
							    void* pUserData3 )
{
    tLevel* pLevel = (tLevel *)pUserData0;
    void* pRet = NULL;

    // look for model instance
    for( int i = 0; i < pLevel->miNumModelInstances; i++ )
    {
		// replace colon with underscore
		const char* pszName = szName;
		char* pszColon = NULL;
		while( ( pszColon = (char *)strstr( pszName, ":" ) ) != NULL )
		{
			*pszColon = '_';
			pszName = pszColon;
		}

        if( !strcmp( pLevel->maModelInstances[i].mszName, szName ) )
        {
            pRet = &pLevel->maModelInstances[i];
            break;
        }
    }
    
    WTFASSERT2( pRet, "didn't find model instance with name: %s", szName );
    
    return pRet;
}

/*
**
*/
void CGameRender::toggleShader( void )
{
	mpCurrShaderProgram = CShaderManager::instance()->getShaderProgram( miShaderIndex );
	miShaderIndex = ( ( miShaderIndex + 1 ) % CShaderManager::instance()->getNumShaders() );
	
	setAttribs( mpCurrShaderProgram->miID );
	CHUD::instance()->setShaderName( mpCurrShaderProgram->mszName );
}