#pragma once

#include "Engine/Engine.h"

#include "Reflect/Translator.h"
#include "Foundation/ConcurrentHashMap.h"
#include "Foundation/ObjectPool.h"
#include "Engine/AssetPath.h"
#include "Engine/Asset.h"

#define HELIUM_ASSET_CACHE_NAME TXT( "Asset" )
#define HELIUM_CONFIG_CACHE_NAME TXT( "Config" )

namespace Helium
{
	class PackageLoader;

	class HELIUM_ENGINE_API AssetIdentifier : public Reflect::ObjectIdentifier
	{
	public:
		virtual bool Identify( const Reflect::ObjectPtr& object, Name* identity ) override;
	};

	class HELIUM_ENGINE_API AssetResolver : public Reflect::ObjectResolver
	{
	public:
		// Reflect::ObjectResolver interface
		virtual bool Resolve( const Name& identity, Reflect::ObjectPtr& pointer, const Reflect::MetaClass* pointerClass );

		// Called by AssetLoader
		bool ReadyToApplyFixups();
		void ApplyFixups();
		bool TryFinishPrecachingDependencies();
		void Clear();

		// Internal fixups that must be completed
		struct Fixup
		{
			Fixup( const Fixup& rhs )
				: m_Pointer( rhs.m_Pointer )
				, m_PointerClass( rhs.m_PointerClass )
				, m_LoadRequestId( rhs.m_LoadRequestId )
			{}

			Fixup( Reflect::ObjectPtr& pointer, const Reflect::MetaClass* pointerClass, size_t loadRequestId )
				: m_Pointer( pointer )
				, m_PointerClass( pointerClass )
				, m_LoadRequestId( loadRequestId )
			{}

			Reflect::ObjectPtr&       m_Pointer;
			const Reflect::MetaClass* m_PointerClass;
			size_t                    m_LoadRequestId;
		};
		DynamicArray< Fixup >  m_Fixups;
	};

	/// Asynchronous object loading interface
	class HELIUM_ENGINE_API AssetLoader : NonCopyable
	{
	public:
		/// Number of request objects to allocate in each block of the request pool.
		static const size_t LOAD_REQUEST_POOL_BLOCK_SIZE = 64;

		friend AssetIdentifier;
		friend AssetResolver;

		/// @name Construction/Destruction
		//@{
		AssetLoader();
		virtual ~AssetLoader() = 0;
		//@}

		/// @name Loading Interface
		//@{
		virtual size_t BeginLoadObject( AssetPath path, bool forceReload = false );
		virtual bool TryFinishLoad( size_t id, AssetPtr& rspObject );
		void FinishLoad( size_t id, AssetPtr& rspObject );

		bool LoadObject( AssetPath path, AssetPtr& rspObject, bool forceReload = false );

		template <class T>
		bool LoadObject( AssetPath path, Helium::StrongPtr< T > &_ptr, bool forceReload = false)
		{
			AssetPtr ptr;
			bool returnValue = LoadObject( path, ptr, forceReload );
			_ptr.Set( Reflect::AssertCast< T >( ptr.Get() ) );
			return returnValue;
		}

#if HELIUM_TOOLS
		virtual bool CacheObject( Asset* pObject, bool bEvictPlatformPreprocessedResourceData = true );
		virtual void EnumerateRootPackages( DynamicArray< AssetPath > &packagePaths );

		static int64_t GetAssetFileTimestamp( const AssetPath &path );
#endif

		virtual void Tick();
		//@}

		/// @name Static Access
		//@{
		static AssetLoader* GetInstance();
		static void Startup();
		static void Shutdown();
		//@}

		static void HandleLinkDependency(Asset &_outer, Helium::StrongPtr<Asset> &_asset_pointer, AssetPath &_path);

		static void FinalizeLink(Asset *_asset);

	protected:
		/// Load status flags.
		enum ELoadFlag
		{
			/// Set once object preloading has completed.
			LOAD_FLAG_PRELOADED = 1 << 0,
			/// Set once object linking has completed.
			LOAD_FLAG_LINKED    = 1 << 1,
			/// Set once resource data has been precached.
			LOAD_FLAG_PRECACHED = 1 << 2,
			/// Set once the object load has been finalized.
			LOAD_FLAG_LOADED    = 1 << 3,

			/// All load progress flags.
			LOAD_FLAG_FULLY_LOADED = LOAD_FLAG_PRELOADED | LOAD_FLAG_LINKED | LOAD_FLAG_PRECACHED | LOAD_FLAG_LOADED,

			/// Set when an error has occurred in the load process.
			LOAD_FLAG_ERROR = 1 << 4,

			/// Set if resource precaching has been started.
			LOAD_FLAG_PRECACHE_STARTED = 1 << 5,

			/// Set if ticking is in progress.
			LOAD_FLAG_IN_TICK = 1 << 6,
		};

		/// Asset load request information.
		struct LoadRequest
		{
			/// Asset path.
			AssetPath path;
			/// Cached object reference.
			AssetPtr spObject;

			/// Package loader.
			PackageLoader* pPackageLoader;
			/// Asset preload request ID.
			size_t packageLoadRequestId;

			/// Loading status flags.
			volatile int32_t stateFlags;

			/// Number of load requests for this specific object.
			volatile int32_t requestCount;

			AssetResolver resolver;

			bool forceReload;
		};

		/// Load request hash map.
		ConcurrentHashMap< AssetPath, LoadRequest* > m_loadRequestMap;
		/// Load request pool.
		ObjectPool< LoadRequest > m_loadRequestPool;

		/// Singleton instance.
		static AssetLoader* sm_pInstance;

		/// @name Loading Implementation
		//@{
		virtual PackageLoader* GetPackageLoader( AssetPath path ) = 0;
		virtual void TickPackageLoaders() = 0;

		virtual void OnPrecacheReady( Asset* pObject, PackageLoader* pPackageLoader );
		virtual void OnLoadComplete( const AssetPath &path, Asset* pObject, PackageLoader* pPackageLoader );
		//@}

	private:

		/// @name Load Process Updating
		//@{
		bool TickLoadRequest( LoadRequest* pRequest );
		bool TickPreload( LoadRequest* pRequest );
		bool TickLink( LoadRequest* pRequest );
		bool TickPrecache( LoadRequest* pRequest );
		bool TickFinalizeLoad( LoadRequest* pRequest );
		//@}
	};

	///////////////////////////////////////////////////////////////////////////
	// Arguments for file save, open, close, etc...
	class AssetEventArgs
	{
	public:
		Asset* m_Asset;

		AssetEventArgs( Asset* asset )
			: m_Asset( asset )
		{
		}
	};
	typedef Helium::Signature< const AssetEventArgs& > AssetEventSignature;

#if HELIUM_TOOLS
	class HELIUM_ENGINE_API AssetTracker : NonCopyable
	{
	public:
		static AssetTracker* GetInstance();
		static void Startup();
		static void Shutdown();

		// Asset system calls these directly to let us know what's going on
		void NotifyAssetLoaded( Asset *pAsset );
		void NotifyAssetCreatedExternally( const AssetPath &pAsset );
		void NotifyAssetChangedExternally( const AssetPath &pAsset );

		// Callback registered with all loaded assets so that we can serve as a pinch point
		// for general asset change notification
		void OnAssetChanged( const Reflect::ObjectChangeArgs &args );

		AssetEventSignature::Event e_AssetLoaded;

		AssetEventSignature::Event e_AssetChanged;

		AssetEventSignature::Event e_AssetCreatedExternally;
		AssetEventSignature::Event e_AssetChangedExternally;

	private:

		/// Singleton instance.
		static AssetTracker* sm_pInstance;
	};
#endif
}
