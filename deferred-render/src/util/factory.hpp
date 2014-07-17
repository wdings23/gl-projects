#include <stdlib.h>
#include <string.h>

/*
**
*/
template <typename T>
CFactory<T>::CFactory( void )
{
	miNumSlots = NUM_ENTRY_SLOTS_PER_ALLOC;
	maEntries = (T **)MALLOC( sizeof( T* ) * miNumSlots );
	for( unsigned int i = 0; i < miNumSlots; i++ )
	{
		maEntries[i] = (T *)MALLOC( sizeof( T ) * NUM_ENTRIES_PER_SLOT );
	}

	maiNumEntries = (unsigned int *)MALLOC( sizeof( int ) * miNumSlots );
	memset( maiNumEntries, 0, sizeof( int ) * miNumSlots );
}

/*
**
*/
template <typename T>
CFactory<T>::~CFactory( void )
{
	for( unsigned int i = 0; i < miNumSlots; i++ )
	{
		FREE( maEntries[i] );
	}

	FREE( maEntries );
	FREE( maiNumEntries );
}

/*
**
*/
template <typename T>
T* CFactory<T>::alloc( unsigned int iNumEntries )
{
	// find FREE space
	unsigned int iFreeSlot = 0;
	for( iFreeSlot = 0; iFreeSlot < miNumSlots; iFreeSlot++ )
	{
		unsigned int iNumFree = NUM_ENTRIES_PER_SLOT - maiNumEntries[iFreeSlot];
		if( iNumFree >= iNumEntries )
		{
			break;
		}
	}

	// allocate
	if( iFreeSlot >= miNumSlots )
	{
		// need to realloc
		
		int iToAlloc = (int)ceilf( iNumEntries / NUM_ENTRY_SLOTS_PER_ALLOC );

		unsigned int iNewNumSlots = miNumSlots + iToAlloc;
		maEntries = (T** )REALLOC( maEntries, sizeof( T* ) * iNewNumSlots );
		maiNumEntries = (unsigned int *)REALLOC( maiNumEntries, sizeof( int ) * iNewNumSlots );
		
		for( unsigned int i = miNumSlots; i < iNewNumSlots; i++ )
		{	
			maiNumEntries[i] = 0;
			maEntries[i] = (T *)MALLOC( sizeof( T ) * NUM_ENTRIES_PER_SLOT );
		}

		iFreeSlot = miNumSlots;
		miNumSlots = iNewNumSlots;
	}
	
	// return and update number of entries in the slot
	int iFreeEntry = maiNumEntries[iFreeSlot];
	T* pRet = &maEntries[iFreeSlot][iFreeEntry];
	maiNumEntries[iFreeSlot] += iNumEntries;

	return pRet;
}