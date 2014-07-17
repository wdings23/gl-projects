#ifndef __FACTORY_H__
#define __FACTORY_H__

#define NUM_ENTRY_SLOTS_PER_ALLOC 20
#define NUM_ENTRIES_PER_SLOT 1000

template <typename T>
class CFactory
{
public:
	CFactory( void );
	~CFactory( void );

	T* alloc( unsigned int iNumEntries );
	void dealloc( T* pStart, unsigned int iNumEntries );

protected: 
	T**					maEntries;
	unsigned int*		maiNumEntries;
	unsigned int		miNumSlots;
};

#include "factory.hpp"

#endif // __FACTORY_H__