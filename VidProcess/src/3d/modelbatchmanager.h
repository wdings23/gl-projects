#ifndef __MODELBATCHMANAGER_H__
#define __MODELBATCHMANAGER_H__

#include "modelbatch.h"
#include "modelinstance.h"
#include "animmodelinstance.h"
#include "morphtarget.h"
#include "shadermanager.h"

#include <vector>

struct ModelBatchManager
{
	tModelBatch*					maBatches;
	std::vector<int>		maNeedInitBatches;
	unsigned int					miNumBatches;
	unsigned int					miCurrBatch;
	unsigned int					miNumBatchAlloc;
};

typedef struct ModelBatchManager tModelBatchManager;

struct ModelBatchManagerInfo
{
	tModelBatchManager*				mpBatchManager;
    
    tModelInstance const*			mpModelInstance;
	tAnimModelInstance const*		mpAnimModelInstance;
    tMorphTarget const*             mpMorphTarget;
    
	tMatrix44 const*				mpViewProjMatrix;
	tMatrix44 const*				maLightViewProjMatrices;
    GLuint*							maiTextureID;
	GLuint*							maiDepthTextureID;
	tVector4 const*					mpColor;
	bool							mbBlend;
	tMatrix44 const*				mpViewMatrix;
	tMatrix44 const*				mpProjMatrix;
	tMatrix44 const*				mProjectionTextureMatrix;

	CTextureAtlasManager const*		mpTextureAtlasManager;

	tVector2 const*					mpCloudUVOffset;
    
    float							mfFurDistance;
	tShaderProgram const*			mpShaderProgram;
};

typedef struct ModelBatchManagerInfo tModelBatchManagerInfo;

void modelBatchManagerInit( tModelBatchManager* pBatchManager );
void modelBatchManagerRelease( tModelBatchManager* pBatchManager );

void modelBatchManagerAddModelInstance( tModelBatchManagerInfo const* pInfo );
void modelBatchManagerAddAnimModelInstance( tModelBatchManagerInfo const* pInfo );
void modelBatchManagerAddMorphTarget( tModelBatchManagerInfo const* pInfo );

void modelBatchManagerResetBatches( tModelBatchManager* pBatchManager );
void modelBatchManagerDraw( tModelBatchManager* pBatchManager, GLint iShader );

void modelBatchManagerDrawNormal( tModelBatchManager* pBatchManager, 
								  tVector4 const* pLightDir,
								  tVector4 const* pCameraPosition );

void modelBatchManagerDrawShadow( tModelBatchManager* pBatchManager, 
								  tShaderProgram const* pShaderProgram, 
								  tVector4 const* pLightDir,
								  tVector4 const* pCameraPosition,
								  tVector4 const* pCameraDir );

void modelBatchManagerReInitGL( tModelBatchManager* pBatchManager );

void modelBatchManagerExecJobs( tModelBatchManager* pBatchManager );

#endif // __MODELBATCHMANAGER_H__