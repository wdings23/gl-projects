#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include "texturemanager.h"
#include "vector.h"

#define MAX_MATERIAL_TEXTURES 4

struct Material
{
	char			mszName[128];
	tTexture*		mapTextures[MAX_MATERIAL_TEXTURES];
	tVector4		mColor;
	int				miNumTextures;
	int				miShaderProgram;
	int				miHash;
};

typedef struct Material tMaterial;

void materialLoad( tMaterial* pMaterial, const char* szFileName );

#endif // __MATERIAL_H__