#pragma once

#include "Engine/Engine.h"
#include "Engine/Asset.h"
#include "Engine/PackageLoader.h"

#include "Foundation/FilePath.h"

namespace Helium
{
	class LooseAssetFileWatcher;

	class HELIUM_PC_SUPPORT_API LoosePackageLoader : public PackageLoader
	{
	public:
		/// Load request pool block size.
		static const size_t LOAD_REQUEST_POOL_BLOCK_SIZE = 4;

		/// Maximum number of bytes to parse at a time.
		static const size_t PARSE_CHUNK_SIZE = 4 * 1024;

		/// Serialized object data.
		struct SerializedObjectData
		{
			/// Asset path.
			AssetPath objectPath;
			/// File path
			FilePath filePath;
			/// File time stamp
			int64_t fileTimeStamp;
			/// Type name.
			Name typeName;
			/// Template path.
			AssetPath templatePath;
			/// Is metadata good?
			bool bMetadataGood;
		};

		/// @name Construction/Destruction
		//@{
		LoosePackageLoader();
		virtual ~LoosePackageLoader();
		//@}

		/// @name Initialization
		//@{
		bool Initialize( AssetPath packagePath );
		void Cleanup();
		//@}

		/// @name Loading
		//@{
		bool BeginPreload();
		virtual bool TryFinishPreload();

		virtual size_t BeginLoadObject( AssetPath path, Reflect::ObjectResolver *pResolver, bool forceReload = false );
		virtual bool TryFinishLoadObject( size_t requestId, AssetPtr& rspObject );

		virtual void Tick();
		//@}

		/// @name Data Access
		//@{
		virtual size_t GetObjectCount() const;
		virtual AssetPath GetAssetPath( size_t index ) const;

		Package* GetPackage() const;
		AssetPath GetPackagePath() const;
		//@}

#if HELIUM_TOOLS
		/// @name Package File Information
		//@{
		virtual bool HasAssetFileState() const;
		virtual const FilePath &GetAssetFileSystemPath( const AssetPath &path ) const;
		virtual int64_t GetAssetFileSystemTimestamp( const AssetPath &path ) const;
		//@}
		
		virtual void EnumerateChildren( DynamicArray< AssetPath > &children ) const;

		virtual bool SaveAsset( Asset *pAsset ) const;
#endif

	private:
		/// Load request flags.
		enum ELoadFlag
		{
			/// Set once property preloading has completed.
			LOAD_FLAG_PROPERTY_PRELOADED            = 1 << 0,
			/// Set once persistent resource data loading has completed.
			LOAD_FLAG_PERSISTENT_RESOURCE_PRELOADED = 1 << 1,

			/// Set once all preloading has completed.
			LOAD_FLAG_PRELOADED = LOAD_FLAG_PROPERTY_PRELOADED | LOAD_FLAG_PERSISTENT_RESOURCE_PRELOADED,

			/// Set when an error has occurred in the load process.
			LOAD_FLAG_ERROR = 1 << 2
		};

		/// Asset load request data.
		struct LoadRequest
		{
			/// Temporary object reference (hold while loading is in progress).
			AssetPtr spObject;
			/// Asset index.
			size_t index;
			/// Resolver from top-level request
			Reflect::ObjectResolver *pResolver;

			/// Cached type reference.
			AssetTypePtr spType;
			/// Cached template reference.
			AssetPtr spTemplate;
			/// Cached owner reference.
			AssetPtr spOwner;
			/// Template object load request ID.
			size_t templateLoadId;
			/// Owner object load request ID.
			size_t ownerLoadId;

			/// Async load ID for persistent resource data.
			size_t persistentResourceDataLoadId;
			/// Buffer for loading cached object data (for pre-loading the persistent resource data).
			uint8_t* pCachedObjectDataBuffer;
			/// Size of the cached object data buffer.
			uint32_t cachedObjectDataBufferSize;

			/// Async load for object file
			size_t asyncFileLoadId;
			void* pAsyncFileLoadBuffer;
			size_t asyncFileLoadBufferSize;

			/// Load flags.
			uint32_t flags;

			bool forceReload;
		};

		/// Package reference.
		PackagePtr m_spPackage;
		/// Package path.
		AssetPath m_packagePath;

		/// Non-zero if the preload process has started.
		volatile int32_t m_startPreloadCounter;
		/// Non-zero if the package has been preloaded.
		volatile int32_t m_preloadedCounter;

		/// Serialized object data parsed from the json package.
		DynamicArray< SerializedObjectData > m_objects;

#if HELIUM_TOOLS
		friend LooseAssetFileWatcher;
		DynamicArray< AssetPath > m_childPackagePaths;
#endif

		/// Pending load requests.
		SparseArray< LoadRequest* > m_loadRequests;
		/// Load request pool.
		ObjectPool< LoadRequest > m_loadRequestPool;

		/// Package file path name.
		FilePath m_packageDirPath;
		
		struct FileReadRequest
		{
			Helium::FilePath filePath;
			void* pLoadBuffer;
			size_t asyncLoadId;
			uint64_t expectedSize;
			uint64_t fileTimestamp;
		};
		DynamicArray<FileReadRequest> m_fileReadRequests;

		/// Parent package load request ID.
		size_t m_parentPackageLoadId;

		/// Mutex for synchronizing access between threads.
		mutable Mutex m_accessLock;

		/// @name Private Utility Functions
		//@{
		void TickPreload();

		void TickLoadRequests();
		bool TickDeserialize( LoadRequest* pRequest );
		bool TickPersistentResourcePreload( LoadRequest* pRequest );
		//@}

		size_t FindObjectByPath( const AssetPath &path ) const;
		size_t FindObjectByName( const Name &name ) const;
	};
}
