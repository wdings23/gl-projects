#ifndef __MODELBATCH_H__
#define __MODELBATCH_H__

#include "interleavevertex.h"
#include "matrix.h"
#include "modelinstance.h"
#include "animmodelinstance.h"
#include "textureatlasmanager.h"
#include "modelbatch.h"
#include "morphtarget.h"
#include "shadermanager.h"
#include "textureatlasmanager.h"

enum
{
	SHADER_ATTRIB_POSITION = 0,
	SHADER_ATTRIB_UV,
	SHADER_ATTRIB_NORM,
	SHADER_ATTRIB_COLOR,
	SHADER_ATTRIB_ORIG_POS,
	SHADER_ATTRIB_ORIG_NORM,
	SHADER_ATTRIB_LIGHTPOS_0,
	SHADER_ATTRIB_PROJECTIVE_UV_0,

	NUM_SHADER_ATTRIBS,
};

#define NUM_DEPTH_TEXTURES_PER_BATCH		2
#define NUM_TEXTURES_PER_BATCH				4
#define NUM_BATCH_VERT_PER_ALLOC	( 1<<16 )	

struct ModelBatch;

struct ModelBatchJobInfo
{
	struct ModelBatch*				mpModelBatch;
	tVector4*						maResultNorm;
	tVector4*						maResultPos;
	tVector4*						maResultOrigPos;
	
	tVector4 const*					maOrigNorm;
	tVector4 const*					maOrigPos;

	tMatrix44 const*				mpTotalMatrix;
	tMatrix44 const*				mpXFormMatrix;
	tMatrix44 const*				mpRotationMatrix;
	tMatrix44 const*				maLightViewProjMatrices;
	
	tMatrix44						mTotalMatrix;
	tMatrix44						maTotalLightMatrices[3];

	GLuint const*					maiDepthTextures;

	int								miNumVerts;
	int								miNumNormals;
	
	tVector4						mColor;

	unsigned int const*				maiVBOIndices;
	int								miNumVBOVerts;
	int								miNumVBOIndices;
	
	tInterleaveVert const*			maVBOVerts;

	tMatrix44 const*				mpProjectionTextureMatrix;
	tInterleaveVertMap const*		maVBOVertPtrs;
	CTextureAtlasManager const*		mpTextureAtlasManager;
	const char*						mszTexture;
	tVector2 const*					mpCloudUVOffset;

	int								miCurrBatchNumPos;
	int								miCurrBatchNumVBOVerts;
	int								miCurrBatchNumVBOIndices;

	tModelInstance const*			mpModelInstance;
	tAnimModelInstance const*		mpAnimModelInstance;
	tMorphTarget const*				mpMorphTarget;

	struct ModelBatchJobInfo*		mpOrigInfo;

	bool							mbFinishXForm;
	bool							mbFinishExec;

	int								miIndexInBatch;

	float							mfFurDistance;
};

typedef struct ModelBatchJobInfo tModelBatchJobInfo;

struct ModelBatch
{
	tInterleaveVert*				maInterleaveVerts;	// submit to GL
	tVector4*						maVertPos;			// for position transformation
	unsigned int*					maiIndices;			// submit to GL

	int								miNumPos;
	int								miNumVerts;
	int								miNumIndices;
	int								miNumModels;
	
	int								miNumVertAlloc;
	int								miNumIndexAlloc;

	tShaderProgram const*			mpShaderProgram;

	tModel const*					mpModel;
    tAnimModel const*               mpAnimModel;
    
	// GL handles
	GLuint							miVBO;
    GLuint							miIBO;

	GLint							miShader;
	
	// shader texture uniforms
	GLint							miAlbedo;
	GLint							miLightProbe;
	GLint							maiDepth[NUM_DEPTH_TEXTURES_PER_BATCH];
	GLint							miDrawAtlas;
	GLint							miLookDir;
	GLint							miProjectedShadow;
	GLint							miProjectedTexture;
    GLint                           miLightmap;
    
	// shader misc uniforms
	GLint							miLightDir;
	GLint							miCameraPos;
	
	// texture id
	GLuint							maiDepthTextureID[NUM_DEPTH_TEXTURES_PER_BATCH];
	GLuint							maiTextureID[NUM_TEXTURES_PER_BATCH];

	bool							mbBlend;

	tModelBatchJobInfo				maModelBatchJobInfo[500];
	int								miNumModelJobInfo;
};

typedef ModelBatch tModelBatch;

struct ModelBatchInstanceInfo
{
	tModelBatch*				mpModelBatch;
	
    tModelInstance const*		mpModelInstance;
	tAnimModelInstance const*	mpAnimModelInstance;
    tMorphTarget const*         mpMorphTarget;
    
	tShaderProgram const*		mpShaderProgram;

	tMatrix44 const*			mpXFormMatrix;
	tMatrix44 const*			mpViewMatrix;
	tMatrix44 const*			mpViewProjMatrix;
	tMatrix44 const*			maLightViewProjMatrices;
	tMatrix44 const*			mpProjectionTextureMatrix;
	GLuint const*				maiTextureID;
	GLuint const*				maiDepthTextures;
	tVector4 const*				mpColor;
	bool						mbBlend;

	CTextureAtlasManager const*	mpTextureAtlasManager;

	tVector2 const*				mpCloudUVOffset;
	float						mfFurDistance;
};

typedef struct ModelBatchInstanceInfo tModelBatchInstanceInfo;

void modelBatchInit( tModelBatch* pModelBatch );
void modelBatchRelease( tModelBatch* pModelBatch );
bool modelBatchAddModelInstance( tModelBatchInstanceInfo const* pInfo );
bool modelBatchAddAnimModel( tModelBatchInstanceInfo const* pInfo );
bool modelBatchAddMorphTarget( tModelBatchInstanceInfo const* pInfo );

void modelBatchSetupGL( tModelBatch* pBatch, GLint iShader );
void modelBatchSetupNormalGL( tModelBatch* pBatch, tShaderProgram const* pShader );
void modelBatchSetupShadowGL( tModelBatch* pBatch, tShaderProgram const* pShader );
void modelBatchUpdateDataGL( tModelBatch const* pBatch );
void modelBatchDraw( tModelBatch const* pBatch, GLint iTex );

void modelBatchDrawNormal( tModelBatch const* pBatch,
						   GLuint const* aiTextureID,
						   tVector4 const* pLightDir,
						   tVector4 const* pCameraPos );

void modelBatchDrawShadow( tModelBatch const* pBatch,
						   GLuint const* aiTextureID,
						   GLuint const* aiDepthTextureID,
						   tVector4 const* pLightDir,
						   tVector4 const* pCameraPos,
						   tVector4 const* pCameraDir );

void modelBatchExecJobs( tModelBatch* pModelBatch );

#endif // __MODELBATCH_H__