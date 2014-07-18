#ifndef __ASSETLOADER_H__
#define __ASSETLOADER_H__

enum
{
	ASSET_MODEL = 0,
	ASSET_TEXTURE,

	NUM_ASSET_TYPES,
};

void initAssetLoad( void );
void assetLoad( void* pData, int iType );
void processAssetOnMainThread( void );

#endif // __ASSETLOADER_H__