#include "Precompile.h"
#include "Engine/AssetPath.h"

#include "Foundation/FilePath.h"

#include "Foundation/ReferenceCounting.h"
#include "Engine/Asset.h"

struct Helium::AssetPath::PendingLink
{
	PendingLink *rpNext;
	//struct Entry *pObjectPath;
	AssetWPtr wpOwner;
	// TODO: Maybe this ought to be a relative pointer from pObjectPath's instance?
	AssetPtr *rpPointerToLink;
};

using namespace Helium;

AssetPath::TableBucket* AssetPath::sm_pTable = NULL;
StackMemoryHeap<>* AssetPath::sm_pEntryMemoryHeap = NULL;
ObjectPool<AssetPath::PendingLink> *AssetPath::sm_pPendingLinksPool = NULL;

/// Parse the object path in the specified string and store it in this object.
///
/// @param[in] pString  Asset path string to set.  If this is null or empty, the path will be cleared.
///
/// @return  True if the string was parsed successfully and the path was set (or cleared, if the string was null or
///          empty), false if not.
///
/// @see Clear(), ToString()
bool AssetPath::Set( const char* pString )
{
	// Check for empty strings first.
	if( !pString || pString[ 0 ] == '\0' )
	{
		m_pEntry = NULL;

		return true;
	}

	StackMemoryHeap<>& rStackHeap = ThreadLocalStackAllocator::GetMemoryHeap();
	StackMemoryHeap<>::Marker stackMarker( rStackHeap );

	Name* pEntryNames;
	uint32_t* pInstanceIndices;
	size_t nameCount;
	size_t packageCount;
	if( !Parse( pString, rStackHeap, pEntryNames, pInstanceIndices, nameCount, packageCount ) )
	{
		return false;
	}

	HELIUM_ASSERT( pEntryNames );
	HELIUM_ASSERT( pInstanceIndices );
	HELIUM_ASSERT( nameCount != 0 );

	Set( pEntryNames, pInstanceIndices, nameCount, packageCount );

	return true;
}

/// Parse the object path in the specified string and store it in this object.
///
/// @param[in] rString  Asset path string to set.  If this is empty, the path will be cleared.
///
/// @return  True if the string was parsed successfully and the path was set (or cleared, if the string was empty),
///          false if not.
///
/// @see Clear(), ToString()
bool AssetPath::Set( const String& rString )
{
	return Set( rString.GetData() );
}

/// Set this path based on the given parameters.
///
/// @param[in] name           Asset name.
/// @param[in] bPackage       True if the object is a package, false if not.
/// @param[in] parentPath     FilePath to the parent object.
/// @param[in] instanceIndex  Asset instance index.  Invalid index values are excluded from the path name string.
///
/// @return  True if the parameters can represent a valid path and the path was set, false if not.
bool AssetPath::Set( Name name, bool bPackage, AssetPath parentPath, uint32_t instanceIndex )
{
	Entry* pParentEntry = parentPath.m_pEntry;

	// Make sure we aren't trying to build a path to a package with a non-package parent.
	if( bPackage && pParentEntry && !pParentEntry->bPackage )
	{
		return false;
	}

	// Build a representation of the path table entry for the given path.
	Entry entry;
	entry.pParent = pParentEntry;
	entry.name = name;
	entry.instanceIndex = instanceIndex;
	entry.bPackage = bPackage;

	// Look up/add the entry.
	m_pEntry = Add( entry );
	HELIUM_ASSERT( m_pEntry );

	return true;
}

/// Set this path to the combination of two paths.
///
/// @param[in] rootPath  Root portion of the path.
/// @param[in] subPath   Sub-path component.
///
/// @return  True if the paths could be joined into a valid path (to which this path was set), false if joining was
///          invalid.
bool AssetPath::Join( AssetPath rootPath, AssetPath subPath )
{
	if( subPath.IsEmpty() )
	{
		m_pEntry = rootPath.m_pEntry;

		return true;
	}

	if( rootPath.IsEmpty() )
	{
		m_pEntry = subPath.m_pEntry;

		return true;
	}

	if( !rootPath.IsPackage() )
	{
		AssetPath testSubPathComponent = subPath.GetParent();
		AssetPath subPathComponent;
		do
		{
			subPathComponent = testSubPathComponent;
			testSubPathComponent = testSubPathComponent.GetParent();

			if( subPathComponent.IsPackage() )
			{
				HELIUM_TRACE(
					TraceLevels::Error,
					"AssetPath::Join(): Cannot combine \"%s\" and \"%s\" (second path is rooted in a package, while the first path ends in an object).\n",
					*rootPath.ToString(),
					*subPath.ToString() );

				return false;
			}
		} while( !testSubPathComponent.IsEmpty() );
	}

	// Assemble the list of path names in reverse order for performing the object path lookup/add.
	size_t nameCount = 0;
	size_t packageCount = 0;

	AssetPath testPath;

	for( testPath = subPath; !testPath.IsEmpty(); testPath = testPath.GetParent() )
	{
		++nameCount;
		if( subPath.IsPackage() )
		{
			++packageCount;
		}
	}

	for( testPath = rootPath; !testPath.IsEmpty(); testPath = testPath.GetParent() )
	{
		++nameCount;
		if( testPath.IsPackage() )
		{
			++packageCount;
		}
	}

	StackMemoryHeap<>& rStackHeap = ThreadLocalStackAllocator::GetMemoryHeap();
	StackMemoryHeap<>::Marker stackMarker( rStackHeap );

	Name* pEntryNames = static_cast< Name* >( rStackHeap.Allocate( sizeof( Name ) * nameCount ) );
	HELIUM_ASSERT( pEntryNames );

	uint32_t* pInstanceIndices = static_cast< uint32_t* >( rStackHeap.Allocate( sizeof( uint32_t ) * nameCount ) );
	HELIUM_ASSERT( pInstanceIndices );

	Name* pCurrentName = pEntryNames;
	uint32_t* pCurrentIndex = pInstanceIndices;

	for( testPath = subPath; !testPath.IsEmpty(); testPath = testPath.GetParent() )
	{
		*pCurrentName = testPath.GetName();
		*pCurrentIndex = testPath.GetInstanceIndex();
		++pCurrentName;
		++pCurrentIndex;
	}

	for( testPath = rootPath; !testPath.IsEmpty(); testPath = testPath.GetParent() )
	{
		*pCurrentName = testPath.GetName();
		*pCurrentIndex = testPath.GetInstanceIndex();
		++pCurrentName;
		++pCurrentIndex;
	}

	// Set the path.
	Set( pEntryNames, pInstanceIndices, nameCount, packageCount );

	return true;
}

/// Set this path to the combination of two paths.
///
/// @param[in] rootPath  Root portion of the path.
/// @param[in] pSubPath  Sub-path component.
///
/// @return  True if the paths could be joined into a valid path (to which this path was set), false if joining was
///          invalid.
bool AssetPath::Join( AssetPath rootPath, const char* pSubPath )
{
	if( !pSubPath || pSubPath[ 0 ] == '\0' )
	{
		m_pEntry = rootPath.m_pEntry;

		return true;
	}

	// Parse the sub-path into a series of names.
	StackMemoryHeap<>& rStackHeap = ThreadLocalStackAllocator::GetMemoryHeap();
	StackMemoryHeap<>::Marker stackMarker( rStackHeap );

	Name* pSubPathNames;
	uint32_t* pSubPathIndices;
	size_t subPathNameCount;
	size_t subPathPackageCount;
	if( !Parse( pSubPath, rStackHeap, pSubPathNames, pSubPathIndices, subPathNameCount, subPathPackageCount ) )
	{
		return false;
	}

	if( !rootPath.IsPackage() && subPathPackageCount != 0 )
	{
		HELIUM_TRACE(
			TraceLevels::Error,
			"AssetPath::Join(): Cannot combine \"%s\" and \"%s\" (second path is rooted in a package, while the first path ends in an object).\n",
			*rootPath.ToString(),
			pSubPath );

		return false;
	}

	// Assemble the list of path names in reverse order for performing the object path lookup/add.
	size_t nameCount = subPathNameCount;
	size_t packageCount = subPathPackageCount;

	AssetPath testPath;

	for( testPath = rootPath; !testPath.IsEmpty(); testPath = testPath.GetParent() )
	{
		++nameCount;
		if( testPath.IsPackage() )
		{
			++packageCount;
		}
	}

	Name* pEntryNames = static_cast< Name* >( rStackHeap.Allocate( sizeof( Name ) * nameCount ) );
	HELIUM_ASSERT( pEntryNames );
	ArrayCopy( pEntryNames, pSubPathNames, subPathNameCount );

	uint32_t* pInstanceIndices = static_cast< uint32_t* >( rStackHeap.Allocate( sizeof( uint32_t ) * nameCount ) );
	HELIUM_ASSERT( pInstanceIndices );
	ArrayCopy( pInstanceIndices, pSubPathIndices, subPathNameCount );

	Name* pCurrentName = &pEntryNames[ subPathNameCount ];
	uint32_t* pCurrentIndex = &pInstanceIndices[ subPathNameCount ];

	for( testPath = rootPath; !testPath.IsEmpty(); testPath = testPath.GetParent() )
	{
		*pCurrentName = testPath.GetName();
		*pCurrentIndex = testPath.GetInstanceIndex();
		++pCurrentName;
		++pCurrentIndex;
	}

	// Set the path.
	Set( pEntryNames, pInstanceIndices, nameCount, packageCount );

	return true;
}

/// Set this path to the combination of two paths.
///
/// @param[in] pRootPath  Root portion of the path.
/// @param[in] subPath    Sub-path component.
///
/// @return  True if the paths could be joined into a valid path (to which this path was set), false if joining was
///          invalid.
bool AssetPath::Join( const char* pRootPath, AssetPath subPath )
{
	if( !pRootPath || pRootPath[ 0 ] == '\0' )
	{
		m_pEntry = subPath.m_pEntry;

		return true;
	}

	// Parse the root path into a series of names.
	StackMemoryHeap<>& rStackHeap = ThreadLocalStackAllocator::GetMemoryHeap();
	StackMemoryHeap<>::Marker stackMarker( rStackHeap );

	Name* pRootPathNames;
	uint32_t* pRootPathIndices;
	size_t rootPathNameCount;
	size_t rootPathPackageCount;
	if( !Parse( pRootPath, rStackHeap, pRootPathNames, pRootPathIndices, rootPathNameCount, rootPathPackageCount ) )
	{
		return false;
	}

	if( rootPathNameCount != rootPathPackageCount )
	{
		AssetPath testSubPathComponent = subPath.GetParent();
		AssetPath subPathComponent;
		do
		{
			subPathComponent = testSubPathComponent;
			testSubPathComponent = testSubPathComponent.GetParent();

			if( subPathComponent.IsPackage() )
			{
				HELIUM_TRACE(
					TraceLevels::Error,
					"AssetPath::Join(): Cannot combine \"%s\" and \"%s\" (second path is rooted in a package, while the first path ends in an object).\n",
					pRootPath,
					*subPath.ToString() );

				return false;
			}
		} while( !testSubPathComponent.IsEmpty() );
	}

	// Assemble the list of path names in reverse order for performing the object path lookup/add.
	size_t nameCount = rootPathNameCount;
	size_t packageCount = rootPathPackageCount;

	AssetPath testPath;

	for( testPath = subPath; !testPath.IsEmpty(); testPath = testPath.GetParent() )
	{
		++nameCount;
		if( testPath.IsPackage() )
		{
			++packageCount;
		}
	}

	Name* pEntryNames = static_cast< Name* >( rStackHeap.Allocate( sizeof( Name ) * nameCount ) );
	HELIUM_ASSERT( pEntryNames );

	uint32_t* pInstanceIndices = static_cast< uint32_t* >( rStackHeap.Allocate( sizeof( uint32_t ) * nameCount ) );
	HELIUM_ASSERT( pInstanceIndices );

	Name* pCurrentName = pEntryNames;
	uint32_t* pCurrentIndex = pInstanceIndices;

	for( testPath = subPath; !testPath.IsEmpty(); testPath = testPath.GetParent() )
	{
		*pCurrentName = testPath.GetName();
		*pCurrentIndex = testPath.GetInstanceIndex();
		++pCurrentName;
		++pCurrentIndex;
	}

	ArrayCopy( pCurrentName, pRootPathNames, rootPathNameCount );
	ArrayCopy( pCurrentIndex, pRootPathIndices, rootPathNameCount );

	// Set the path.
	Set( pEntryNames, pInstanceIndices, nameCount, packageCount );

	return true;
}

/// Set this path to the combination of two paths.
///
/// @param[in] pRootPath  Root portion of the path.
/// @param[in] pSubPath   Sub-path component.
///
/// @return  True if the paths could be joined into a valid path (to which this path was set), false if joining was
///          invalid.
bool AssetPath::Join( const char* pRootPath, const char* pSubPath )
{
	if( !pRootPath || pRootPath[ 0 ] == '\0' )
	{
		return Set( pSubPath );
	}

	if( !pSubPath || pSubPath[ 0 ] == '\0' )
	{
		return Set( pRootPath );
	}

	// Parse both path components into separate series of names.
	StackMemoryHeap<>& rStackHeap = ThreadLocalStackAllocator::GetMemoryHeap();
	StackMemoryHeap<>::Marker stackMarker( rStackHeap );

	Name* pRootPathNames;
	uint32_t* pRootPathIndices;
	size_t rootPathNameCount;
	size_t rootPathPackageCount;
	if( !Parse( pRootPath, rStackHeap, pRootPathNames, pRootPathIndices, rootPathNameCount, rootPathPackageCount ) )
	{
		return false;
	}

	Name* pSubPathNames;
	uint32_t* pSubPathIndices;
	size_t subPathNameCount;
	size_t subPathPackageCount;
	if( !Parse( pSubPath, rStackHeap, pSubPathNames, pSubPathIndices, subPathNameCount, subPathPackageCount ) )
	{
		return false;
	}

	if( rootPathNameCount != rootPathPackageCount && subPathPackageCount != 0 )
	{
		HELIUM_TRACE(
			TraceLevels::Error,
			"AssetPath::Join(): Cannot combine \"%s\" and \"%s\" (second path is rooted in a package, while the first path ends in an object).\n",
			pRootPath,
			pSubPath );

		return false;
	}

	// Assemble the list of path names in reverse order for performing the object path lookup/add.
	size_t nameCount = rootPathNameCount + subPathNameCount;
	size_t packageCount = rootPathPackageCount + subPathPackageCount;

	Name* pEntryNames = static_cast< Name* >( rStackHeap.Allocate( sizeof( Name ) * nameCount ) );
	HELIUM_ASSERT( pEntryNames );
	ArrayCopy( pEntryNames, pSubPathNames, subPathNameCount );
	ArrayCopy( pEntryNames + subPathNameCount, pRootPathNames, rootPathNameCount );

	uint32_t* pInstanceIndices = static_cast< uint32_t* >( rStackHeap.Allocate( sizeof( uint32_t ) * nameCount ) );
	HELIUM_ASSERT( pInstanceIndices );
	ArrayCopy( pInstanceIndices, pSubPathIndices, subPathNameCount );
	ArrayCopy( pInstanceIndices + subPathNameCount, pRootPathIndices, rootPathNameCount );

	// Set the path.
	Set( pEntryNames, pInstanceIndices, nameCount, packageCount );

	return true;
}

/// Generate the string representation of this object path.
///
/// @param[out] rString  String representation of this path.
///
/// @see Set()
void AssetPath::ToString( String& rString ) const
{
	rString.Remove( 0, rString.GetSize() );

	if( !m_pEntry )
	{
		return;
	}

	EntryToString( *m_pEntry, rString );
}

/// Generate a string representation of this object path with all package and object delimiters converted to valid
/// directory delimiters for the current platform.
///
/// @param[out] rString  File path string representation of this path.
void AssetPath::ToFilePathString( String& rString ) const
{
	rString.Remove( 0, rString.GetSize() );

	if( !m_pEntry )
	{
		return;
	}

	EntryToFilePathString( *m_pEntry, rString );
}

/// Clear out this object path.
///
/// @see Set()
void AssetPath::Clear()
{
	m_pEntry = NULL;
}

/// Release the object path table and free all allocated memory.
///
/// This should only be called immediately prior to application exit.
void AssetPath::Shutdown()
{
	HELIUM_TRACE( TraceLevels::Info, "Shutting down AssetPath table.\n" );

	delete [] sm_pTable;
	sm_pTable = NULL;

	delete sm_pEntryMemoryHeap;
	sm_pEntryMemoryHeap = NULL;

	delete sm_pPendingLinksPool;
	sm_pPendingLinksPool = NULL;

	HELIUM_TRACE( TraceLevels::Info, "AssetPath table shutdown complete.\n" );
}

/// Convert the path separator characters in the given object path to valid directory delimiters for the current
/// platform.
///
/// Note that objects with instance indices are not supported for file paths.  Instance index delimiters will not be
/// converted, and may be invalid file name characters on certain platforms.
///
/// @param[out] rFilePath     Converted file path.
/// @param[in]  rPackagePath  Asset path string to convert (can be the same as the output file path).
void AssetPath::ConvertStringToFilePath( String& rFilePath, const String& rPackagePath )
{
#if HELIUM_PACKAGE_PATH_CHAR != HELIUM_PATH_SEPARATOR_CHAR && HELIUM_PACKAGE_PATH_CHAR != HELIUM_ALT_PATH_SEPARATOR_CHAR
	size_t pathLength = rPackagePath.GetSize();
	if( &rFilePath == &rPackagePath )
	{
		for( size_t characterIndex = 0; characterIndex < pathLength; ++characterIndex )
		{
			char& rCharacter = rFilePath[ characterIndex ];
			if( rCharacter == HELIUM_PACKAGE_PATH_CHAR || rCharacter == HELIUM_OBJECT_PATH_CHAR )
			{
				rCharacter = Helium::s_InternalPathSeparator;
			}
		}
	}
	else
	{
		rFilePath.Remove( 0, rFilePath.GetSize() );
		rFilePath.Reserve( rPackagePath.GetSize() );

		for( size_t characterIndex = 0; characterIndex < pathLength; ++characterIndex )
		{
			char character = rPackagePath[ characterIndex ];
			if( character == HELIUM_PACKAGE_PATH_CHAR || character == HELIUM_OBJECT_PATH_CHAR )
			{
				character = Helium::s_InternalPathSeparator;
			}

			rFilePath.Add( character );
		}
	}
#else
	rFilePath = rPackagePath;
#endif
}

/// Set this object path based on the given parameters.
///
/// @param[in] pNames            Array of object names in the path, starting from the bottom level on up.
/// @param[in] pInstanceIndices  Array of object instance indices in the path, starting from the bottom level on up.
/// @param[in] nameCount         Number of object names.
/// @param[in] packageCount      Number of object names that are packages.
void AssetPath::Set( const Name* pNames, const uint32_t* pInstanceIndices, size_t nameCount, size_t packageCount )
{
	HELIUM_ASSERT( pNames );
	HELIUM_ASSERT( pInstanceIndices );
	HELIUM_ASSERT( nameCount != 0 );

	// Set up the entry for this path.
	Entry entry;
	entry.pParent = NULL;
	entry.name = pNames[ 0 ];
	entry.instanceIndex = pInstanceIndices[ 0 ];
	entry.bPackage = ( nameCount <= packageCount );

	if( nameCount > 1 )
	{
		size_t parentNameCount = nameCount - 1;

		AssetPath parentPath;
		parentPath.Set( pNames + 1, pInstanceIndices + 1, parentNameCount, Min( parentNameCount, packageCount ) );
		entry.pParent = parentPath.m_pEntry;
		HELIUM_ASSERT( entry.pParent );
	}

	// Look up/add the entry.
	m_pEntry = Add( entry );
	HELIUM_ASSERT( m_pEntry );
}

/// Parse a string into separate path name components.
///
/// @param[in]  pString            String to parse.  This must *not* be empty.
/// @param[in]  rStackHeap         Stack memory heap from which to allocate the resulting name array.
/// @param[out] rpNames            Parsed array of names, in reverse order (top-level path name stored at the end of
///                                the array).  Note that this will be allocated using the given heap and must be
///                                deallocated by the caller.
/// @param[out] rpInstanceIndices  Parsed array of instance indices, with each index corresponding to each name
///                                entry in the parsed names array.  This is also allocated using the given heap and
///                                must be deallocated by the caller.
/// @param[out] rNameCount         Number of names in the parsed array.
/// @param[out] rPackageCount      Number of names specifying packages.
///
/// @return  True if the string was parsed successfully, false if not.
bool AssetPath::Parse(
	const char* pString,
	StackMemoryHeap<>& rStackHeap,
	Name*& rpNames,
	uint32_t*& rpInstanceIndices,
	size_t& rNameCount,
	size_t& rPackageCount )
{
	HELIUM_ASSERT( pString );
	HELIUM_ASSERT( pString[ 0 ] != '\0' );

	rpNames = NULL;
	rpInstanceIndices = NULL;
	rNameCount = 0;
	rPackageCount = 0;

	// Make sure the entry specifies an absolute path.
	if( pString[ 0 ] != HELIUM_PACKAGE_PATH_CHAR && pString[ 0 ] != HELIUM_OBJECT_PATH_CHAR )
	{
		HELIUM_TRACE(
			TraceLevels::Warning,
			"AssetPath: FilePath string \"%s\" does not contain a leading path separator.\n",
			pString );

		return false;
	}

	// Count the number of path separators in the path.
	size_t nameCount = 0;
	size_t packageCount = 0;

	size_t nameLengthMax = 0;

	const char* pTestCharacter = pString;
	const char* pNameStartPos = pTestCharacter;
	for( ; ; )
	{
		char character = *pTestCharacter;
		if( character == '\0' )
		{
			size_t nameLength = static_cast< size_t >( pTestCharacter - pNameStartPos );
			if( nameLength > nameLengthMax )
			{
				nameLengthMax = nameLength;
			}

			break;
		}

		// Any adjacent colons (i.e. like in /Types:Helium::ConfigAsset) but being careful to not look behind beginning of pString
		// So it is not legal for a name to start or end with a :, but we can support fully qualified C++ types as names
		bool bIsObjectPathChar = (character == HELIUM_OBJECT_PATH_CHAR && 
			pTestCharacter[1] != HELIUM_OBJECT_PATH_CHAR && 
			(pTestCharacter == pString || pTestCharacter[-1] != HELIUM_OBJECT_PATH_CHAR));

		if( character == HELIUM_PACKAGE_PATH_CHAR )
		{
			if( packageCount != nameCount )
			{
				HELIUM_TRACE(
					TraceLevels::Warning,
					"AssetPath: Unexpected package path separator at character %" PRIdPD " of path string \"%s\".\n",
					pTestCharacter - pString,
					pString );

				return false;
			}

			++nameCount;
			++packageCount;

			size_t nameLength = static_cast< size_t >( pTestCharacter - pNameStartPos );
			if( nameLength > nameLengthMax )
			{
				nameLengthMax = nameLength;
			}

			pNameStartPos = pTestCharacter + 1;
		}
		else if( bIsObjectPathChar )
		{
			++nameCount;

			size_t nameLength = static_cast< size_t >( pTestCharacter - pNameStartPos );
			if( nameLength > nameLengthMax )
			{
				nameLengthMax = nameLength;
			}

			pNameStartPos = pTestCharacter + 1;
		}

		++pTestCharacter;
	}

	HELIUM_ASSERT( nameCount != 0 );

	// Parse the names from the string.
	rpNames = static_cast< Name* >( rStackHeap.Allocate( sizeof( Name ) * nameCount ) );
	HELIUM_ASSERT( rpNames );

	rpInstanceIndices = static_cast< uint32_t* >( rStackHeap.Allocate( sizeof( uint32_t ) * nameCount ) );
	HELIUM_ASSERT( rpInstanceIndices );

	char* pTempNameString = static_cast< char* >( rStackHeap.Allocate(
		sizeof( char ) * ( nameLengthMax + 1 ) ) );
	HELIUM_ASSERT( pTempNameString );
	char* pTempNameCharacter = pTempNameString;

	Name* pTargetName = &rpNames[ nameCount - 1 ];
	uint32_t* pTargetIndex = &rpInstanceIndices[ nameCount - 1 ];

	bool bParsingName = true;

	pTestCharacter = pString + 1;
	for( ; ; )
	{
		char character = *pTestCharacter;

		// Any adjacent colons (i.e. like in /Types:Helium::ConfigAsset). Since the string begins with '/' and ends with '\0' we can just index away
		// So it is not legal for a name to start or end with a :, but we can support fully qualified C++ types as names
		bool bIsObjectPathChar = false;
		if (character == HELIUM_OBJECT_PATH_CHAR && pTestCharacter[1] != HELIUM_OBJECT_PATH_CHAR && pTestCharacter[-1] != HELIUM_OBJECT_PATH_CHAR)
		{
			bIsObjectPathChar = true;
		}

		if( character != HELIUM_PACKAGE_PATH_CHAR && !bIsObjectPathChar && character != '\0' )
		{
			// Make sure the character is a valid number when parsing the instance index.
			if( !bParsingName && ( character < '0' || character > '9' ) )
			{
				HELIUM_TRACE(
					TraceLevels::Error,
					"AssetPath: Encountered non-numeric instance index value in path string \"%s\".\n",
					*pString );

				return false;
			}

			if( bParsingName && character == HELIUM_INSTANCE_PATH_CHAR )
			{
				// Encountered a separator for the instance index, so begin parsing it.
				*pTempNameCharacter = '\0';

				pTargetName->Set( pTempNameString );
				--pTargetName;

				pTempNameCharacter = pTempNameString;
				bParsingName = false;
			}
			else
			{
				HELIUM_ASSERT( static_cast< size_t >( pTempNameCharacter - pTempNameString ) < nameLengthMax );
				*pTempNameCharacter = character;
				++pTempNameCharacter;
			}
		}
		else
		{
			*pTempNameCharacter = '\0';

			if( bParsingName )
			{
				pTargetName->Set( pTempNameString );
				--pTargetName;

				SetInvalid( *pTargetIndex );
				--pTargetIndex;
			}
			else
			{
				if( pTempNameCharacter == pTempNameString )
				{
					HELIUM_TRACE(
						TraceLevels::Error,
						"AssetPath: Empty instance index encountered in path string \"%s\".\n",
						pString );

					return false;
				}

				if( pTempNameCharacter - pTempNameString > 1 && *pTempNameString == '0' )
				{
					HELIUM_TRACE(
						TraceLevels::Error,
						"AssetPath: Encountered instance index \"%s\" with leading zeros in path string \"%s\".\n",
						pTempNameString,
						pString );

					return false;
				}

				int parseCount = StringScan( pTempNameString, "%" SCNu32, pTargetIndex );
				if( parseCount != 1 )
				{
					HELIUM_TRACE(
						TraceLevels::Error,
						"AssetPath: Failed to parse object instance index \"%s\" in path string \"%s\".\n",
						pTempNameString,
						pString );

					return false;
				}

				if( IsInvalid( *pTargetIndex ) )
				{
					HELIUM_TRACE(
						TraceLevels::Error,
						"AssetPath: Instance index \"%s\" in path string \"%s\" is a reserved value.\n",
						pTempNameString,
						pString );

					return false;
				}

				--pTargetIndex;
			}

			if( character == '\0' )
			{
				break;
			}

			pTempNameCharacter = pTempNameString;
			bParsingName = true;
		}

		++pTestCharacter;
	}

	rNameCount = nameCount;
	rPackageCount = packageCount;

	return true;
}

/// Look up a table entry, adding it if it does not exist.
///
/// This also handles lazy initialization of the path table and allocator.
///
/// @param[in] rEntry  Entry to locate or add.
///
/// @return  Pointer to the actual table entry.
AssetPath::Entry* AssetPath::Add( const Entry& rEntry )
{
	// Lazily initialize the hash table.  Note that this is not inherently thread-safe, but there should always be
	// at least one path created before any sub-threads are spawned.
	if( !sm_pEntryMemoryHeap )
	{
		sm_pEntryMemoryHeap = new StackMemoryHeap<>( STACK_HEAP_BLOCK_SIZE );
		HELIUM_ASSERT( sm_pEntryMemoryHeap );

		sm_pPendingLinksPool = new ObjectPool<PendingLink>( PENDING_LINKS_POOL_BLOCK_SIZE );
		HELIUM_ASSERT( sm_pPendingLinksPool );

		HELIUM_ASSERT( !sm_pTable );
		sm_pTable = new TableBucket [ TABLE_BUCKET_COUNT ];
		HELIUM_ASSERT( sm_pTable );
	}

	HELIUM_ASSERT( sm_pTable );

	// Compute the entry's hash table index and retrieve the corresponding bucket.
	uint32_t bucketIndex = ComputeEntryStringHash( rEntry ) % TABLE_BUCKET_COUNT;
	TableBucket& rBucket = sm_pTable[ bucketIndex ];

	// Locate the entry in the table.  If it does not exist, add it.
	size_t entryCount = 0;
	Entry* pTableEntry = rBucket.Find( rEntry, entryCount );
	if( !pTableEntry )
	{
		pTableEntry = rBucket.Add( rEntry, entryCount );
		HELIUM_ASSERT( pTableEntry );
	}

	return pTableEntry;
}

/// Recursive function for building the string representation of an object path entry.
///
/// @param[in]  rEntry   FilePath entry.
/// @param[out] rString  FilePath string.
void AssetPath::EntryToString( const Entry& rEntry, String& rString )
{
	Entry* pParent = rEntry.pParent;
	if( pParent )
	{
		EntryToString( *pParent, rString );
	}

	rString += ( rEntry.bPackage ? HELIUM_PACKAGE_PATH_CHAR : HELIUM_OBJECT_PATH_CHAR );
	rString += rEntry.name.Get();
	if( IsValid( rEntry.instanceIndex ) )
	{
		char instanceIndexString[ 16 ];
		StringPrint(
			instanceIndexString,
			HELIUM_INSTANCE_PATH_CHAR_STRING "%" PRIu32,
			rEntry.instanceIndex );
		instanceIndexString[ HELIUM_ARRAY_COUNT( instanceIndexString ) - 1 ] = '\0';

		rString += instanceIndexString;
	}
}

/// Recursive function for building the file path string representation of an object path entry.
///
/// @param[in]  rEntry   FilePath entry.
/// @param[out] rString  File path string.
void AssetPath::EntryToFilePathString( const Entry& rEntry, String& rString )
{
	Entry* pParent = rEntry.pParent;
	if( pParent )
	{
		EntryToFilePathString( *pParent, rString );
	}

	rString += Helium::s_InternalPathSeparator;
	rString += rEntry.name.Get();
}

/// Compute a hash value for an object path entry based on the contents of the name strings (slow, should only be
/// used internally when a string comparison is needed).
///
/// @param[in] rEntry  Asset path entry.
///
/// @return  Hash value.
size_t AssetPath::ComputeEntryStringHash( const Entry& rEntry )
{
	size_t hash = StringHash( rEntry.name.GetDirect() );
	hash = ( ( hash * 33 ) ^ rEntry.instanceIndex );
	hash = ( ( hash * 33 ) ^
		( rEntry.bPackage
		? static_cast< size_t >( HELIUM_PACKAGE_PATH_CHAR )
		: static_cast< size_t >( HELIUM_OBJECT_PATH_CHAR ) ) );

	Entry* pParent = rEntry.pParent;
	if( pParent )
	{
		size_t parentHash = ComputeEntryStringHash( *pParent );
		hash = ( ( hash * 33 ) ^ parentHash );
	}

	return hash;
}

/// Get whether the contents of the two given object path entries match.
///
/// @param[in] rEntry0  Asset path entry.
/// @param[in] rEntry1  Asset path entry.
///
/// @return  True if the contents match, false if not.
bool AssetPath::EntryContentsMatch( const Entry& rEntry0, const Entry& rEntry1 )
{
	return ( rEntry0.name == rEntry1.name &&
		rEntry0.instanceIndex == rEntry1.instanceIndex &&
		( rEntry0.bPackage ? rEntry1.bPackage : !rEntry1.bPackage ) &&
		rEntry0.pParent == rEntry1.pParent );
}

/// Find an existing object path entry in this table.
///
/// @param[in]  rEntry       Externally defined entry to match.
/// @param[out] rEntryCount  Number of entries in this bucket when the search was performed.
///
/// @return  Table entry if found, null if not found.
///
/// @see Add()
AssetPath::Entry* AssetPath::TableBucket::Find( const Entry& rEntry, size_t& rEntryCount )
{
	ScopeReadLock readLock( m_lock );

	Entry* const * ppTableEntries = m_entries.GetData();
	size_t entryCount = m_entries.GetSize();
	HELIUM_ASSERT( ppTableEntries || entryCount == 0 );

	rEntryCount = entryCount;

	for( size_t entryIndex = 0; entryIndex < entryCount; ++entryIndex )
	{
		Entry* pTableEntry = ppTableEntries[ entryIndex ];
		HELIUM_ASSERT( pTableEntry );
		if( EntryContentsMatch( rEntry, *pTableEntry ) )
		{
			return pTableEntry;
		}
	}

	return NULL;
}

/// Add an object path entry to this table if it does not already exist.
///
/// @param[in] rEntry              Externally defined entry to locate or add.
/// @param[in] previousEntryCount  Number of entries already checked during a previous Find() call (existing entries
///                                are not expected to change).  In other words, the entry index from which to start
///                                checking for any additional string entries that may have been added since the
///                                previous Find() call.
///
/// @return  Pointer to the object path table entry.
///
/// @see Find()
AssetPath::Entry* AssetPath::TableBucket::Add( const Entry& rEntry, size_t previousEntryCount )
{
	ScopeWriteLock writeLock( m_lock );

	Entry* const * ppTableEntries = m_entries.GetData();
	size_t entryCount = m_entries.GetSize();
	HELIUM_ASSERT( ppTableEntries || entryCount == 0 );
	HELIUM_ASSERT( previousEntryCount <= entryCount );
	for( size_t entryIndex = previousEntryCount; entryIndex < entryCount; ++entryIndex )
	{
		Entry* pTableEntry = ppTableEntries[ entryIndex ];
		HELIUM_ASSERT( pTableEntry );
		if( EntryContentsMatch( rEntry, *pTableEntry ) )
		{
			return pTableEntry;
		}
	}

	HELIUM_ASSERT( sm_pEntryMemoryHeap );
	Entry* pNewEntry = static_cast< Entry* >( sm_pEntryMemoryHeap->Allocate( sizeof( Entry ) ) );
	HELIUM_ASSERT( pNewEntry );
	new( pNewEntry ) Entry( rEntry );

	m_entries.Push( pNewEntry );

	return pNewEntry;
}
