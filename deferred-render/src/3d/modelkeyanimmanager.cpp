#include "modelkeyanimmanager.h"
#include "hashutil.h"

#define NUM_ANIM_PER_ALLOC 100

tModelKeyAnimManager gModelKeyAnimManager;
tModelKeyAnimManager* gpModelKeyAnimManager = &gModelKeyAnimManager;

/*
**
*/
void modelKeyAnimManagerInit( tModelKeyAnimManager* pManager )
{
	pManager->miNumKeyAnims = 0;
	pManager->miNumKeyAnimAlloc = NUM_ANIM_PER_ALLOC;
	pManager->maKeyAnims = (tModelKeyAnim *)malloc( sizeof( tModelKeyAnim ) * pManager->miNumKeyAnimAlloc );
	pManager->maszAnimNames = (char **)malloc( sizeof( char* ) * pManager->miNumKeyAnimAlloc );
	pManager->maiNameHash = (int *)malloc( sizeof( int ) * pManager->miNumKeyAnimAlloc );

	for( int i = 0; i < pManager->miNumKeyAnimAlloc; i++ )
	{
		pManager->maszAnimNames[i] = (char *)malloc( sizeof( char ) * 128 );
	}
}

/*
**
*/
void modelKeyAnimManagerRelease( tModelKeyAnimManager* pManager )
{
	for( int i = 0; i < pManager->miNumKeyAnimAlloc; i++ )
	{
		free( pManager->maszAnimNames[i] );
	}
	
	for( int i = 0; i < pManager->miNumKeyAnims; i++ )
	{
		modelKeyAnimRelease( &pManager->maKeyAnims[i] );
	}
	
	free( pManager->maKeyAnims );
	free( pManager->maszAnimNames );
	free( pManager->maiNameHash );
}

/*
**
*/
void modelKeyAnimManagerAddAnim( tModelKeyAnimManager* pManager, const char* szFileName )
{
	if( pManager->miNumKeyAnims + 1 >= pManager->miNumKeyAnimAlloc )
	{
		pManager->miNumKeyAnimAlloc += NUM_ANIM_PER_ALLOC;
		pManager->maKeyAnims = (tModelKeyAnim *)realloc( pManager->maKeyAnims, sizeof( tModelKeyAnim ) * pManager->miNumKeyAnimAlloc );
		pManager->maszAnimNames = (char **)realloc( pManager->maszAnimNames, sizeof( char* ) * pManager->miNumKeyAnimAlloc );
		pManager->maiNameHash = (int *)realloc( pManager->maiNameHash, sizeof( int ) * pManager->miNumKeyAnimAlloc );

		for( int i = pManager->miNumKeyAnimAlloc - NUM_ANIM_PER_ALLOC; i < pManager->miNumKeyAnimAlloc; i++ )
		{
			pManager->maszAnimNames[i] = (char *)realloc( pManager->maszAnimNames, sizeof( char ) * 128 );
		}
	}

	tModelKeyAnim* pAnim = &pManager->maKeyAnims[pManager->miNumKeyAnims];
	modelKeyAnimInit( pAnim );
	modelKeyAnimLoad( pAnim, szFileName );

	char szNoExtension[128];
	memset( szNoExtension, 0, sizeof( szNoExtension ) );
	const char* szExtensionStart = strstr( szFileName, "." );
	memcpy( szNoExtension, szFileName, (size_t)szExtensionStart - (size_t)szFileName );

	strncpy( pManager->maszAnimNames[pManager->miNumKeyAnims], szNoExtension, 128 );
	pManager->maiNameHash[pManager->miNumKeyAnims] = hash( pManager->maszAnimNames[pManager->miNumKeyAnims] );
	++pManager->miNumKeyAnims;
}

/*
**
*/
tModelKeyAnim const* modelKeyAnimManagerGetAnim( tModelKeyAnimManager* pManager, char const* szFileName )
{
	tModelKeyAnim const* pRet = NULL;

	int iHash = hash( szFileName );
	for( int i = 0; i < pManager->miNumKeyAnims; i++ )
	{
		if( pManager->maiNameHash[i] == iHash )
		{
			pRet = &pManager->maKeyAnims[i];
			break;
		}
	}

	return pRet;
}