#ifndef __MENUANIMMANAGER_H__
#define __MENUANIMMANAGER_H__

#include "animplayerDB.h"
#include <vector>

#define ANIMPLAYER_ALLOC 100

class CMenuAnimManager
{
public:
	CMenuAnimManager( void );
	virtual ~CMenuAnimManager( void );
	
	CAnimPlayerDB* getAnimPlayer( const char* szAnimPlayer );
	CAnimPlayerDB* load( const char* szFileName, std::vector<CAnimPlayerDB *> *pAnimList = NULL );

protected:
	std::vector<CAnimPlayerDB *>	mapAnimPlayers;

public:
	static CMenuAnimManager*		instance( void );

protected:
	static CMenuAnimManager*		mpInstance;
};

#endif // __MENUANIMMANAGER_H__