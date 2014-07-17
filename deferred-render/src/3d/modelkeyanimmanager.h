#ifndef __MODELKEYANIMMANAGER_H__
#define __MODELKEYANIMMANAGER_H__

#include "modelkeyanim.h"

struct ModelKeyAnimManager
{
	int*					maiNameHash;
	char**					maszAnimNames;
	tModelKeyAnim*			maKeyAnims;
	int						miNumKeyAnims;
	int						miNumKeyAnimAlloc;
};

typedef struct ModelKeyAnimManager tModelKeyAnimManager;

void modelKeyAnimManagerInit( tModelKeyAnimManager* pManager );
void modelKeyAnimManagerRelease( tModelKeyAnimManager* pManager );
void modelKeyAnimManagerAddAnim( tModelKeyAnimManager* pManager, const char* szFileName );

tModelKeyAnim const* modelKeyAnimManagerGetAnim( tModelKeyAnimManager* pManager, char const* szFileName );

extern tModelKeyAnimManager* gpModelKeyAnimManager;

#endif // __MODELKEYANIMMANAGER_H__