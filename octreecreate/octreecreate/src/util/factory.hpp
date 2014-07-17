#include <stdlib.h>

/*
**
*/
template <typename T>
CFactory<T>::CFactory( void )
{
	miNumSlots = NUM_ENTRY_SLOTS_PER_ALLOC;
	maEntries = (T **)malloc( sizeof( T* ) * miNumSlots );
	for( unsigned int i = 0; i < miNumSlots; i++ )
	{
		maEntries[i] = (T *)malloc( sizeof( T ) * NUM_ENTRIES_PER_SLOT );
	}

	maiNumEntries = (unsigned int *)malloc( sizeof( int ) * miNumSlots );
	memset( maiNumEntries, 0, sizeof( int ) * miNumSlots );

	miNumEntriesPerSlot = NUM_ENTRIES_PER_SLOT;
}

/*
**
*/
template <typename T>
CFactory<T>::CFactory( size_t iNumToAlloc )
{
	miNumSlots = NUM_ENTRY_SLOTS_PER_ALLOC;
	maEntries = (T **)malloc( sizeof( T* ) * miNumSlots );
	for( unsigned int i = 0; i < miNumSlots; i++ )
	{
		maEntries[i] = (T *)malloc( sizeof( T ) * iNumToAlloc );
	}

	maiNumEntries = (unsigned int *)malloc( sizeof( int ) * miNumSlots );
	memset( maiNumEntries, 0, sizeof( int ) * miNumSlots );

	miNumEntriesPerSlot = (int)iNumToAlloc;
}

/*
**
*/
template <typename T>
CFactory<T>::~CFactory( void )
{
	for( unsigned int i = 0; i < miNumSlots; i++ )
	{
		free( maEntries[i] );
	}

	free( maEntries );
	free( maiNumEntries );
}

/*
**
*/
template <typename T>
T* CFactory<T>::alloc( unsigned int iNumEntries )
{
	// find free space
	unsigned int iFreeSlot = 0;
	for( iFreeSlot = 0; iFreeSlot < miNumSlots; iFreeSlot++ )
	{
		unsigned int iNumFree = miNumEntriesPerSlot - maiNumEntries[iFreeSlot];
		if( iNumFree >= iNumEntries )
		{
			break;
		}
	}

	// allocate
	if( iFreeSlot >= miNumSlots )
	{
		// need to realloc

		unsigned int iNewNumSlots = miNumSlots + NUM_ENTRY_SLOTS_PER_ALLOC;
		maEntries = (T** )realloc( maEntries, sizeof( T* ) * iNewNumSlots );
		maiNumEntries = (unsigned int *)realloc( maiNumEntries, sizeof( int ) * iNewNumSlots );
		
		for( unsigned int i = miNumSlots; i < iNewNumSlots; i++ )
		{	
			maiNumEntries[i] = 0;
			maEntries[i] = (T *)malloc( sizeof( T ) * miNumEntriesPerSlot );
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