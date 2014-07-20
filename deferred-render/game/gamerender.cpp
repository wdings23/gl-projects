#include "gamerender.h"
#include "assetloader.h"
#include "jobmanager.h"
#include "render.h"
#include "batchsprite.h"
#include "hud.h"
#include "fontmanager.h"
#include "textureatlasmanager.h"
#include "filepathutil.h"
#include "LodePNG.h"

#if defined( _WIN32 )
#include "glee.h"
#endif // _WIN32

struct LevelJobData
{
    tLevel*         mpLevel;
    char            mszFileName[128];

	void			(*mpfnCallback)( void );
};

typedef struct LevelJobData tLevelJobData;

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

struct LightInfo
{
	tVector4		mPosition;
	tVector4		mXFormPosition;
	tVector4		mColor;
	float			mfSize;
	float			mfAngle;
	float			mfAngleInc;
};

typedef struct LightInfo tLightInfo;


static GLuint siIndexBuffer;

static GLuint siDeferredFBO;
static GLuint siPositionTexture;
static GLuint siAlbedoTexture;
static GLuint siDepthTexture;
static GLuint siNormalTexture;

static GLuint siLightFBO; 
static GLuint siLightTexture;
static GLuint siLightDepthTexture;
static GLuint siLightStencilBuffer;

static GLuint siAmbientOcclusionFBO;
static GLuint siAmbientOcclusionTexture;

static GLuint siSphereBuffer;
static GLuint siSphererIndexBuffer;

static GLuint siInstanceVBO;
static GLuint siInstanceInfoVBO;

static int siNumSphereTris;

static int siNumCubes = 100000;
static CCamera const* spCamera;

static tLightInfo* saLightInfo;
const int giNumLights = 50;

/*
**
*/
static void createSphere( void )
{
	char szFullPath[256];
	getFullPath( szFullPath, "sphere.obj" );
	FILE* fp = fopen( szFullPath, "rb" );
	fseek( fp, 0, SEEK_END );
	unsigned long iFileSize = ftell( fp );
	fseek( fp, 0, SEEK_SET );

	char* acBuffer = (char *)malloc( ( iFileSize + 1 ) * sizeof( char ) );
	fread( acBuffer, sizeof( char ), iFileSize, fp );
	acBuffer[iFileSize] = '\0';

	const int iMaxData = 1000;
	tVector4* aPos = (tVector4 *)malloc( sizeof( tVector4 ) * iMaxData );
	tVector4* aNorm = (tVector4 *)malloc( sizeof( tVector4 ) * iMaxData );
	tVector2* aUV = (tVector2 *)malloc( sizeof( tVector2 ) * iMaxData );
	tFace* aFace = (tFace *)malloc( sizeof( tFace ) * iMaxData );

	tVector2* pCurrUV = aUV;
	tVector4* pCurrPos = aPos;
	tVector4* pCurrNorm = aNorm;
	tFace* pCurrFace = aFace;

	int iNumPos = 0;
	int iNumNorm = 0;
	int iNumUV = 0;
	int iNumFaces = 0;

	char* pacCurrPos = acBuffer;
	for( ;; )
	{
		// get line
		char szLine[512];
		memset( szLine, 0, sizeof( szLine ) );
		char* pacStartPos = pacCurrPos;
		
		do
		{
			++pacCurrPos;
		} while( *pacCurrPos != '\n' && *pacCurrPos != '\0' );

		if( *pacCurrPos == '\n' )
		{
			++pacCurrPos;
		}

		unsigned long iLineSize = (unsigned long)pacCurrPos - (unsigned long)pacStartPos;
		memcpy( szLine, pacStartPos, iLineSize );

		// check the data type
		if( szLine[0] == 'v' )
		{
			// vertex position, uv, or normal

			float afVal[] = { 0.0f, 0.0f, 0.0f };
			char* pacNumStart = strstr( szLine, " " ) + 1;
			char* pacLineEnd = strstr( szLine, "\n" );
			bool bDone = false;
			for( int i = 0; i < 3; i++ )
			{
				char szNum[64];
				memset( szNum, 0, sizeof( szNum ) );
				char* pacNumEnd = strstr( pacNumStart, " " );

				if( pacNumEnd == NULL )
				{
					bDone = true;
					pacNumEnd = pacLineEnd;
				}

				unsigned long iNumSize = (unsigned long)pacNumEnd - (unsigned long)pacNumStart;
				memcpy( szNum, pacNumStart, iNumSize );
				afVal[i] = (float)atof( szNum ); 

				pacNumStart = pacNumEnd + 1;
				if( bDone )
				{
					break;
				}
			}

			if( szLine[1] == 't' )
			{
				pCurrUV->fX = afVal[0]; 
				pCurrUV->fY = afVal[1]; 
				
				++pCurrUV;
				++iNumUV;
			}
			else if( szLine[1] == 'n' )
			{
				pCurrNorm->fX = afVal[0]; 
				pCurrNorm->fY = afVal[1]; 
				pCurrNorm->fZ = afVal[2]; 
				pCurrNorm->fW = 1.0f; 

				++pCurrNorm;
				++iNumNorm;
			}
			else 
			{
				pCurrPos->fX = afVal[0]; 
				pCurrPos->fY = afVal[1]; 
				pCurrPos->fZ = afVal[2]; 
				pCurrPos->fW = 1.0f; 

				++pCurrPos;
				++iNumPos;
			}
		}
		else if( szLine[0] == 'f' )
		{
			// face
			char* pacNumStart = strstr( szLine, " " ) + 1;
			char* pacLineEnd = strstr( szLine, "\n" );
			bool bDone = false;
			for( int i = 0; i < 3; i++ )
			{
				char szNum[64];
				memset( szNum, 0, sizeof( szNum ) );
				char* pacNumEnd = strstr( pacNumStart, " " );

				if( pacNumEnd == NULL )
				{
					bDone = true;
					pacNumEnd = pacLineEnd;
				}

				unsigned long iNumSize = (unsigned long)pacNumEnd - (unsigned long)pacNumStart;
				memcpy( szNum, pacNumStart, iNumSize );
				
				char* paiPos = strtok( szNum, "/" );
				char* paiNorm = strtok( NULL, "/" );
				char* paiUV = strtok( NULL, "/ " );

				pCurrFace->maiV[i] = atoi( paiPos ) - 1;
				pCurrFace->maiUV[i] = atoi( paiUV ) - 1;
				pCurrFace->maiNorm[i] = atoi( paiNorm ) - 1;
				
				pacNumStart = pacNumEnd + 1;
				if( bDone )
				{
					break;
				}
			}	// for i = 0 to 3

			++pCurrFace;
			++iNumFaces;
		}

		unsigned long iCurrPos = (unsigned long)pacCurrPos - (unsigned long)acBuffer;
		if( iCurrPos >= iFileSize )
		{
			break;
		}
	}

	// vertices in the faces
	tInstanceVertex* aVertices = (tInstanceVertex *)malloc( sizeof( tInstanceVertex ) * iNumFaces * 3 );
	tInstanceVertex* pVert = aVertices;

	int iCount = 0;
	for( int i = 0; i < iNumFaces; i++ )
	{
		tFace const* pFace = &aFace[i];

		for( int j = 0; j < 3; j++ )
		{
			tVector4 const* pPos = &aPos[pFace->maiV[j]];
			tVector4 const* pNorm = &aNorm[pFace->maiV[j]];
			tVector2 const* pUV = &aUV[pFace->maiV[j]];

			memcpy( &pVert->mPos, pPos, sizeof( tVector4 ) );
			memcpy( &pVert->mNormal, pNorm, sizeof( tVector4 ) );
			memcpy( &pVert->mUV, pUV, sizeof( tVector2 ) );
		
			++pVert;
			++iCount;

		}	// for j = 0 to 3

		WTFASSERT2( iCount <= iNumFaces * 3, "array out of bounds" );

	}	// for i = 0 to 3

	int* aiIndices = (int *)malloc( sizeof( int ) * iNumFaces * 3 );
	for( int i = 0; i < iNumFaces * 3; i++ )
	{
		aiIndices[i] = i;
	}

	// positions
	glGenBuffers( 1, &siSphereBuffer );
	glBindBuffer( GL_ARRAY_BUFFER, siSphereBuffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( tInstanceVertex ) * iNumFaces * 3, aVertices, GL_STATIC_DRAW );

	// indices
	glGenBuffers( 1, &siSphererIndexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, siSphererIndexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( int ) * iNumFaces * 3, aiIndices, GL_STATIC_DRAW );

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );	

	siNumSphereTris = iNumFaces * 3;

	free( aiIndices );
	free( aVertices );

	free( aFace );
	free( aUV );
	free( aNorm );
	free( aPos );

	free( acBuffer );
	fclose( fp );

}

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
    
	srand( (unsigned int)time( NULL ) );
	
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
				float fZ = 0.0f - (float)( rand() % 4 ) * 0.25f;

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
	
    int iShader = CShaderManager::instance()->getShader( "instance_deferred" );
    glUseProgram( iShader );
    
	glGenBuffers( 1, &siInstanceVBO );
	glBindBuffer( GL_ARRAY_BUFFER, siInstanceVBO );
	glBufferData( GL_ARRAY_BUFFER, sizeof( tInstanceVertex ) * iNumIndices, aVerts, GL_STATIC_DRAW );
	
	// indices
	glGenBuffers( 1, &siIndexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, siIndexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( int ) * iNumIndices, aiVBOIndices, GL_STATIC_DRAW );
	
	free( aiVBOIndices );
	free( aVerts );

	glGenBuffers( 1, &siInstanceInfoVBO );
	glBindBuffer( GL_ARRAY_BUFFER, siInstanceInfoVBO );
	glBufferData( GL_ARRAY_BUFFER, sizeof( tInstanceInfo ) * siNumCubes, aInstanceInfo, GL_DYNAMIC_DRAW );
	
	free( aInstanceInfo );

    glUseProgram( 0 );
    
	// deferred fbo textures
	{
		int iFBWidth = (int)( (float)renderGetScreenWidth() * renderGetScreenScale() );
		int iFBHeight = (int)( (float)renderGetScreenHeight() * renderGetScreenScale() );

		glGenFramebuffers( 1, &siDeferredFBO );
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, siDeferredFBO );
        
		// albedo 
		glGenTextures( 1, &siAlbedoTexture );
		glBindTexture( GL_TEXTURE_2D, siAlbedoTexture );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, iFBWidth, iFBHeight, 0, GL_RGBA, GL_FLOAT, NULL );
        glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, siAlbedoTexture, 0);
		
		// position 
		glGenTextures( 1, &siPositionTexture );
		glBindTexture( GL_TEXTURE_2D, siPositionTexture );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, iFBWidth, iFBHeight, 0, GL_RGBA, GL_FLOAT, NULL );
		glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, siPositionTexture, 0);

		// normal
		glGenTextures( 1, &siNormalTexture );
		glBindTexture( GL_TEXTURE_2D, siNormalTexture );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, iFBWidth, iFBHeight, 0, GL_RGBA, GL_FLOAT, NULL );
		glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, siNormalTexture, 0);

		// depth
		glGenTextures( 1, &siDepthTexture );
		glBindTexture( GL_TEXTURE_2D, siDepthTexture );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, iFBWidth, iFBHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
		glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, siDepthTexture, 0);
		
		GLenum aBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers( 3, aBuffers );
		
		// fbo for spot lights
		glGenFramebuffers( 1, &siLightFBO );
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, siLightFBO );

		glGenTextures( 1, &siLightTexture );
		glBindTexture( GL_TEXTURE_2D, siLightTexture );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, iFBWidth, iFBHeight, 0, GL_RGBA, GL_FLOAT, NULL );
		glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, siLightTexture, 0);
		
		glGenTextures( 1, &siLightDepthTexture );
		glBindTexture( GL_TEXTURE_2D, siLightDepthTexture );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, iFBWidth, iFBHeight, 0, GL_RGBA, GL_FLOAT, NULL );
		glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, siLightDepthTexture, 0);

		glGenRenderbuffers( 1, &siLightStencilBuffer );
		glBindRenderbuffer( GL_RENDERBUFFER, siLightStencilBuffer );
		glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, iFBWidth, iFBHeight );
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, siLightStencilBuffer );
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, siLightStencilBuffer );
		
		glDrawBuffers( 1, aBuffers );

		// fbo for ambient occlusion
		glGenFramebuffers( 1, &siAmbientOcclusionFBO );
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, siAmbientOcclusionFBO );

		glGenTextures( 1, &siAmbientOcclusionTexture );
		glBindTexture( GL_TEXTURE_2D, siAmbientOcclusionTexture );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, iFBWidth, iFBHeight, 0, GL_RGBA, GL_FLOAT, NULL );
		glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, siAmbientOcclusionTexture, 0);

		glDrawBuffers( 1, aBuffers );
		
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
		glBindRenderbuffer( GL_RENDERBUFFER, 0 );
	}

	createSphere();
	
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );	

	// light info
	const int iRandRange = 1000;
	saLightInfo = (tLightInfo *)malloc( sizeof( tLightInfo ) * giNumLights );
	for( int i = 0; i < giNumLights; i++ )
	{
		tLightInfo* pLightInfo = &saLightInfo[i];
		pLightInfo->mColor.fX =(float)( rand() % iRandRange ) / (float)iRandRange;
		pLightInfo->mColor.fY = (float)( rand() % iRandRange ) / (float)iRandRange;
		pLightInfo->mColor.fZ = (float)( rand() % iRandRange ) / (float)iRandRange;
		pLightInfo->mColor.fW = 1.0f;

		pLightInfo->mPosition.fX = (float)( rand() % iRandRange ) / 50.0f + 0.0f;
		pLightInfo->mPosition.fY = (float)( rand() % iRandRange ) / 50.0f + 0.0f;
		pLightInfo->mPosition.fZ = (float)( rand() % iRandRange ) / 500.0f;
		pLightInfo->mPosition.fW = 1.0f;

		pLightInfo->mfSize = (float)( rand() % iRandRange ) / 200.0f;
		pLightInfo->mfAngle = 0.0f;
		pLightInfo->mfAngleInc = (float)( rand() % iRandRange ) / ( (float)iRandRange * 10.0f );
		pLightInfo->mfAngleInc -= (float)iRandRange * 0.5f / ( (float)iRandRange * 10.0f );;
	}

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
	GLint iViewRotationMatrix = glGetUniformLocation( iShader, "cam_rotation_matrix" );

	tMatrix44 viewMatrix, projMatrix;
	Matrix44Transpose( &viewMatrix, pViewMatrix );
	Matrix44Transpose( &projMatrix, pProjMatrix );

	tMatrix44 const* pCamRotMatrix = pCamera->getRotationMatrix();

	glUniformMatrix4fv( iViewMatrix, 1, GL_FALSE, viewMatrix.afEntries ); 
	glUniformMatrix4fv( iProjMatrix, 1, GL_FALSE, projMatrix.afEntries ); 
	glUniformMatrix4fv( iViewRotationMatrix, 1, GL_FALSE, pCamRotMatrix->afEntries );

	int iPosAttrib = glGetAttribLocation( iShader, "position" );
	int iNormAttrib = glGetAttribLocation( iShader, "normal" );
	int iColorAttrib = glGetAttribLocation( iShader, "color" );
	int iScaleAttrib = glGetAttribLocation( iShader, "scaling" );
	int iTransAttrib = glGetAttribLocation( iShader, "translation" );

	// enable attribs and set ptr	
	glBindBuffer( GL_ARRAY_BUFFER, siInstanceInfoVBO );
	
	glEnableVertexAttribArray( iTransAttrib );
	glVertexAttribPointer( iTransAttrib, 4, GL_FLOAT, GL_FALSE, sizeof( tInstanceInfo ), NULL );
    glVertexAttribDivisor( iTransAttrib, 1 );
    
	glEnableVertexAttribArray( iScaleAttrib );
	glVertexAttribPointer( iScaleAttrib, 4, GL_FLOAT, GL_FALSE, sizeof( tInstanceInfo ), (void *)sizeof( tVector4 ) );
    glVertexAttribDivisor( iScaleAttrib, 1 );
    
	glEnableVertexAttribArray( iColorAttrib );
	glVertexAttribPointer( iColorAttrib, 4, GL_FLOAT, GL_FALSE, sizeof( tInstanceInfo ), (void *)( sizeof( tVector4 ) + sizeof( tVector4 ) ) );
    glVertexAttribDivisor( iColorAttrib, 1 );
    
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, siIndexBuffer );
	glBindBuffer( GL_ARRAY_BUFFER, siInstanceVBO );
	
	glEnableVertexAttribArray( iPosAttrib );
	glVertexAttribPointer( iPosAttrib, 4, GL_FLOAT, GL_FALSE, sizeof( tInstanceVertex ), NULL );
	
	glEnableVertexAttribArray( iNormAttrib );
	glVertexAttribPointer( iNormAttrib, 4, GL_FLOAT, GL_FALSE, sizeof( tInstanceVertex ), (void *)sizeof( tVector4 ) );

	//glBindVertexArray( siModelVertexArray ); 

	// draw
	glDrawElementsInstancedEXT( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, siNumCubes );

    glDisableVertexAttribArray( iNormAttrib );
    glDisableVertexAttribArray( iPosAttrib );
    glDisableVertexAttribArray( iColorAttrib );
    glDisableVertexAttribArray( iScaleAttrib );
    glDisableVertexAttribArray( iTransAttrib );

    glVertexAttribDivisor( iColorAttrib, 0 );
    glVertexAttribDivisor( iScaleAttrib, 0 );
    glVertexAttribDivisor( iTransAttrib, 0 );
    
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	//glBindVertexArray( 0 );
}

/*
** draw light models into the light texture to be blended with the final render target
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

	for( int i = 0; i < giNumLights; i++ ) 
	{
		tLightInfo* pLight = &saLightInfo[i];

		float fCosAngle = cosf( pLight->mfAngle );
		float fSinAngle = sinf( pLight->mfAngle );

		pLight->mXFormPosition.fX = fCosAngle * pLight->mPosition.fX - fSinAngle * pLight->mPosition.fY;
		pLight->mXFormPosition.fY = fSinAngle * pLight->mPosition.fX + fCosAngle * pLight->mPosition.fY;
		pLight->mXFormPosition.fZ = pLight->mPosition.fZ;
		pLight->mXFormPosition.fW = 1.0f;

		pLight->mfAngle += pLight->mfAngleInc;
	}

	// matrix
	tMatrix44 const* pViewMatrix = spCamera->getViewMatrix();
	tMatrix44 const* pProjMatrix = spCamera->getProjectionMatrix();

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	
	// disable writing to depth buffer
	glDepthMask( GL_FALSE );

    for( int j = 0; j < 8; j++ )
    {
        glActiveTexture( GL_TEXTURE1 + j );
        glBindTexture( GL_TEXTURE_2D, 0 );
    }
    
    // position texture
    GLuint iPosTex = glGetUniformLocation( iLightShader, "posTex" );
    glActiveTexture( GL_TEXTURE0 );
    glUniform1i( iPosTex, 0 );
    glBindTexture( GL_TEXTURE_2D, siPositionTexture );
    
	for( int i = 0; i < giNumLights; i++ )
	{
		tLightInfo* pLight = &saLightInfo[i];
		
		glUseProgram( iLightShader );
		
		tMatrix44 modelViewMatrix, translateMatrix, scaleMatrix, translateScaleMatrix;
		Matrix44Translate( &translateMatrix, &pLight->mXFormPosition );
		Matrix44Scale( &scaleMatrix, pLight->mfSize, pLight->mfSize, pLight->mfSize );
		Matrix44Multiply( &translateScaleMatrix, &translateMatrix, &scaleMatrix );

		//Matrix44Multiply( &modelViewMatrix, pViewMatrix, &translateScaleMatrix );
		Matrix44Multiply( &modelViewMatrix, pViewMatrix, &translateScaleMatrix );

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
		glUniform4f( iLightColor, pLight->mColor.fX,pLight->mColor.fY, pLight->mColor.fZ, pLight->mColor.fW );
		
		GLint iPos = glGetAttribLocation( iLightShader, "position" ); 
		
//glDisable( GL_CULL_FACE );
//glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

		// bind vbo
		glBindBuffer( GL_ARRAY_BUFFER, siSphereBuffer );
		glEnableVertexAttribArray( iPos );
		glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, sizeof( tInstanceVertex ), NULL );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, siSphererIndexBuffer );
		
		glEnable( GL_DEPTH_TEST );
		glEnable( GL_STENCIL_TEST );
		glEnable( GL_CULL_FACE );
		
		glStencilFunc( GL_ALWAYS, 0, 0xFF );				// always pass (write to every pixel)

		glDisable( GL_CULL_FACE );
		
		// increase on depth fail for back face, decrease on depth pass for front and check for 0 value
		glStencilOpSeparate( GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP );	
		glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR_WRAP );

		// render light into stencil
		glDrawElements( GL_TRIANGLES, siNumSphereTris, GL_UNSIGNED_INT, 0 );
		
        glDisableVertexAttribArray( iPos );
        
		// render light color
		{
			glEnable( GL_CULL_FACE );
			glDisable( GL_DEPTH_TEST );

			// replace stencil value back to 0 for next light
			glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );
			
			// draw where it's equal to 0
			glStencilFunc( GL_EQUAL, 0, 0xFF );
			glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
			
			int iLightAttenShader = CShaderManager::instance()->getShader( "light_attenuation" );
			glUseProgram( iLightAttenShader );
			
			// light's color
			GLint iLightAttenColor = glGetUniformLocation( iLightAttenShader, "lightColor" );
			//WTFASSERT2( iLightAttenColor >= 0, "invalid semantic lightColor" );
			glUniform4f( iLightAttenColor, pLight->mColor.fX, pLight->mColor.fY, pLight->mColor.fZ, pLight->mColor.fW );

			GLint iLightAttenPos = glGetUniformLocation( iLightAttenShader, "lightPos" );
			//WTFASSERT2( iLightAttenPos >= 0, "invalid semantic lightPos" );
			glUniform4f( iLightAttenPos, pLight->mXFormPosition.fX, pLight->mXFormPosition.fY, pLight->mXFormPosition.fZ, pLight->mXFormPosition.fW );

			GLint iLightSize = glGetUniformLocation( iLightAttenShader, "lightSize" );
			glUniform1f( iLightSize, 1.0f / pLight->mfSize * 0.25f );

			// set matrices
			iViewMatrix = glGetUniformLocation( iLightAttenShader, "view_matrix" );
			iProjMatrix = glGetUniformLocation( iLightAttenShader, "projection_matrix" );

			glUniformMatrix4fv( iViewMatrix, 1, GL_FALSE, viewMatrix.afEntries ); 
			glUniformMatrix4fv( iProjMatrix, 1, GL_FALSE, projMatrix.afEntries ); 

			GLint iPos = glGetAttribLocation( iInstanceShader, "position" ); 
			
			// bind vbo
			glBindBuffer( GL_ARRAY_BUFFER, siSphereBuffer );
			glEnableVertexAttribArray( iPos );
			glVertexAttribPointer( iPos, 4, GL_FLOAT, GL_FALSE, sizeof( tInstanceVertex ), NULL );
			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, siSphererIndexBuffer );

			glDrawElements( GL_TRIANGLES, siNumSphereTris, GL_UNSIGNED_INT, 0 );

			glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
            glDisableVertexAttribArray( iPos );
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
	tVector4 aScreenVerts[] =
	{
		{ -1.0f, -1.0f, 0.0f, 1.0f },
		{ 1.0f, -1.0f, 0.0f, 1.0f },
		{ -1.0f, 1.0f, 0.0f, 1.0f },
		{ 1.0f, 1.0f, 0.0f, 1.0f },
	};
    
	tVector2 aTexCoords[] =
	{
		{ 0.0f, 0.0f },
		{ 1.0, 0.0f },
		{ 0.0f, 1.0f },
		{ 1.0f, 1.0f }
	};
    
	// create ambient occlusion texture 
	{
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, siAmbientOcclusionFBO );
        //glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
        
		glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
		glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

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

		GLuint iPosTex = glGetUniformLocation( iShader, "posTex" );
		glActiveTexture( GL_TEXTURE2 );
		glUniform1i( iPosTex, 2 );
		glBindTexture( GL_TEXTURE_2D, siPositionTexture );

		tTexture* pRandomTexture = CTextureManager::instance()->getTexture( "random_dir.tga" );

		GLuint iRandomDirTex = glGetUniformLocation( iShader, "randomDirTex" );
		glActiveTexture( GL_TEXTURE3 );
		glUniform1i( iRandomDirTex, 3 );
		glBindTexture( GL_TEXTURE_2D, pRandomTexture->miID );

		int iPosition = glGetAttribLocation( iShader, "position" );
		int iUV = glGetAttribLocation( iShader, "uv" );

        glVertexAttribPointer( iUV, 2, GL_FLOAT, GL_FALSE, 0, aTexCoords );
		glEnableVertexAttribArray( iUV );
        
		glVertexAttribPointer( iPosition, 4, GL_FLOAT, GL_FALSE, 0, aScreenVerts );
		glEnableVertexAttribArray( iPosition );
    
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
        
        glDisableVertexAttribArray( iUV );
        glDisableVertexAttribArray( iPosition );
        
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
	}

	// draw final scene
	{
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        
		int iShader = CShaderManager::instance()->getShader( "deferred_shading" );
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

		GLuint iAmbientOcclusionTex = glGetUniformLocation( iShader, "ambientOcclusionTex" );
		glActiveTexture( GL_TEXTURE5 );
		glUniform1i( iAmbientOcclusionTex, 5 );
		glBindTexture( GL_TEXTURE_2D, siAmbientOcclusionTexture );

		int iPosition = glGetAttribLocation( iShader, "position" );
		int iUV = glGetAttribLocation( iShader, "uv" );

		glVertexAttribPointer( iPosition, 4, GL_FLOAT, GL_FALSE, 0, aScreenVerts );
		glEnableVertexAttribArray( iPosition );
    
		glVertexAttribPointer( iUV, 2, GL_FLOAT, GL_FALSE, 0, aTexCoords );
		glEnableVertexAttribArray( iUV );
    
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	}
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