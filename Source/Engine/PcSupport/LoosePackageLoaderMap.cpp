//----------------------------------------------------------------------------------------------------------------------
// LoosePackageLoaderMap.cpp
//----------------------------------------------------------------------------------------------------------------------

#include "Precompile.h"
#include "PcSupport/LoosePackageLoaderMap.h"

#include "PcSupport/LoosePackageLoader.h"

namespace Helium
{
    /// Constructor.
    LoosePackageLoaderMap::LoosePackageLoaderMap()
    {
    }

    /// Destructor.
    LoosePackageLoaderMap::~LoosePackageLoaderMap()
    {
        ConcurrentHashMap< AssetPath, LoosePackageLoader* >::ConstAccessor loaderAccessor;
        if( m_packageLoaderMap.First( loaderAccessor ) )
        {
            do
            {
                LoosePackageLoader* pLoader = loaderAccessor->Second();
                HELIUM_ASSERT( pLoader );

                delete pLoader;

                ++loaderAccessor;
            } while( loaderAccessor.IsValid() );
        }
    }

    /// Get the package loader to use to load the object with the given path, creating it if necessary.
    ///
    /// @param[in] path  Asset path.
    ///
    /// @return  Package loader to use to load the specified object.
    LoosePackageLoader* LoosePackageLoaderMap::GetPackageLoader( AssetPath path )
    {
        HELIUM_ASSERT( !path.IsEmpty() );

        // Resolve the object's package.
        AssetPath packagePath = path;
        while( !packagePath.IsPackage() )
        {
            packagePath = packagePath.GetParent();
            if( packagePath.IsEmpty() )
            {
                HELIUM_TRACE(
                    TraceLevels::Error,
                    "LoosePackageLoaderMap::GetPackageLoader(): Cannot resolve package loader for \"%s\", as it is not located in a package.\n",
                    *path.ToString() );

                return NULL;
            }
        }

        // Locate an existing package loader.
        ConcurrentHashMap< AssetPath, LoosePackageLoader* >::ConstAccessor constMapAccessor;
        if( m_packageLoaderMap.Find( constMapAccessor, packagePath ) )
        {
            LoosePackageLoader* pLoader = constMapAccessor->Second();
            HELIUM_ASSERT( pLoader );

            return pLoader;
        }

        // Add a new package loader entry.
        ConcurrentHashMap< AssetPath, LoosePackageLoader* >::Accessor mapAccessor;
        bool bInserted = m_packageLoaderMap.Insert(
            mapAccessor,
            KeyValue< AssetPath, LoosePackageLoader* >( packagePath, NULL ) );
        if( bInserted )
        {
            // Entry added, so create and initialize the package loader.
            LoosePackageLoader* pLoader = new LoosePackageLoader;
            HELIUM_ASSERT( pLoader );

            bool bInitResult = pLoader->Initialize( packagePath );
            HELIUM_ASSERT( bInitResult );
            if( !bInitResult )
            {
                HELIUM_TRACE(
                    TraceLevels::Error,
                    "LoosePackageLoaderMap::GetPackageLoader(): Failed to initialize package loader for \"%s\".\n",
                    *packagePath.ToString() );

                m_packageLoaderMap.Remove( mapAccessor );

                return NULL;
            }

            HELIUM_VERIFY( pLoader->BeginPreload() );

            mapAccessor->Second() = pLoader;
        }

        // PMD: I am not 100% sure this is all thread safe. I think it could break if:
        // - Thread 1 inserts a key/value of path and NULL, as above
        // - Thread 2 returns false from the insert so we come straight here
        //   - Thread 2 tries to use pLoader before thread 1 assigns ploader to the value of the key
        // Note: If we did not do this assert here, we could potentially get nulls from this same thread later when
        // trying to find pLoader.
        //
        // Leaving it alone for now since I'm not sure, but if this assert gets tripped, we need to revisit this.
        // Easy fix may be to allocate and construct (but don't completely init) an LoosePackageLoader, and try
        // to insert that directly rather than the null above. If insert succeeds, finish, else ditch our loader
        // and grab the one out of the array
        LoosePackageLoader* pLoader = mapAccessor->Second();
        HELIUM_ASSERT( pLoader );

        return pLoader;
    }

    /// Tick all package loaders for a given AssetLoader tick.
    void LoosePackageLoaderMap::TickPackageLoaders()
    {        
        /// Cached list of package loaders iterated over in Tick() (separated to avoid deadlocks with concurrent hash
        /// map access).
        DynamicArray< LoosePackageLoader* > m_packageLoaderTickArray;

        // Build the list of package loaders to update this tick from the loader map (the Tick() for a given package
        // loader could require modification to the package loader map, which would cause a deadlock if we have the same
        // part of the hash map locked as which needs to be updated).
        //HELIUM_ASSERT( m_packageLoaderTickArray.IsEmpty() );

        ConcurrentHashMap< AssetPath, LoosePackageLoader* >::ConstAccessor loaderAccessor;
        if( m_packageLoaderMap.First( loaderAccessor ) )
        {
            do
            {
                LoosePackageLoader* pLoader = loaderAccessor->Second();
                HELIUM_ASSERT( pLoader );
                m_packageLoaderTickArray.Push( pLoader );

                ++loaderAccessor;
            } while( loaderAccessor.IsValid() );
        }

        // Tick each loader.
        size_t loaderCount = m_packageLoaderTickArray.GetSize();
        for( size_t loaderIndex = 0; loaderIndex < loaderCount; ++loaderIndex )
        {
            LoosePackageLoader* pLoader = m_packageLoaderTickArray[ loaderIndex ];
            HELIUM_ASSERT( pLoader );
            pLoader->Tick();
        }

        // Clear out the tick array.
        m_packageLoaderTickArray.Resize( 0 );
    }
}
