#include "EditorPch.h"

#include "EditorEngine.h"

#if HELIUM_DIRECT3D
# include "RenderingD3D9/D3D9Renderer.h"
#elif HELIUM_OPENGL
# include "RenderingGL/GLRenderer.h"
#endif

#include "Graphics/DynamicDrawer.h"
#include "Framework/WorldManager.h"
#include "Reflect/Object.h"
#include "Graphics/BufferedDrawer.h"
#include "Engine/PackageLoader.h"

using namespace Helium;
using namespace Helium::Editor;

uint32_t                           ForciblyFullyLoadedPackageManager::sm_InitCount = 0;
ForciblyFullyLoadedPackageManager* ForciblyFullyLoadedPackageManager::sm_pInstance = NULL;

ForciblyFullyLoadedPackageManager* ForciblyFullyLoadedPackageManager::GetInstance()
{
	return sm_pInstance;
}

void ForciblyFullyLoadedPackageManager::Startup()
{
	if ( ++sm_InitCount == 1 )
	{
		HELIUM_ASSERT( !sm_pInstance );
		sm_pInstance = new ForciblyFullyLoadedPackageManager();
		HELIUM_ASSERT( sm_pInstance );
	}
}

void ForciblyFullyLoadedPackageManager::Shutdown()
{
	if ( --sm_InitCount == 0 )
	{
		HELIUM_ASSERT( sm_pInstance );
		delete sm_pInstance;
		sm_pInstance = NULL;
	}
}

void ForciblyFullyLoadedPackageManager::Tick()
{
	AssetLoader *pAssetLoader = AssetLoader::GetInstance();

	// For each editable package
	for ( DynamicArray< ForciblyFullyLoadedPackage >::Iterator packageIter = m_ForciblyFullyLoadedPackages.Begin();
		packageIter != m_ForciblyFullyLoadedPackages.End(); ++packageIter)
	{
		ForciblyFullyLoadedPackage &package = *packageIter;

		// Load the package if we need to
		if ( Helium::IsValid< size_t >( package.m_PackageLoadId ) )
		{
			HELIUM_ASSERT( package.m_Assets.IsEmpty() );
			HELIUM_ASSERT( package.m_AssetLoadIds.IsEmpty() );
			HELIUM_ASSERT( package.m_AssetPaths.IsEmpty() );
			HELIUM_ASSERT( !package.m_Package );

			AssetPtr packagePtr;
			if ( pAssetLoader->TryFinishLoad( package.m_PackageLoadId, packagePtr ) )
			{
				// Load request is finished.
				package.m_PackageLoadId = Helium::Invalid< size_t >();
				package.m_Package = Reflect::AssertCast<Package>(packagePtr);

				if ( package.m_Package )
				{
					if ( !package.m_Package->GetAllFlagsSet( Asset::FLAG_EDITOR_FORCIBLY_LOADED ) )
					{
						// Package loaded successfully, queue load requests for all children
						package.m_Package->SetFlags( Asset::FLAG_EDITOR_FORCIBLY_LOADED );
						e_AssetForciblyLoadedEvent.Raise( AssetEventArgs( package.m_Package ) );
					}

					PackageLoader *pLoader = package.m_Package->GetLoader();
					pLoader->EnumerateChildren( package.m_AssetPaths );

					package.m_Assets.Resize( package.m_AssetPaths.GetSize() );
					package.m_AssetLoadIds.Resize( package.m_AssetPaths.GetSize() );

					DynamicArray< AssetPath >::Iterator assetPathIter = package.m_AssetPaths.Begin();
					DynamicArray< size_t >::Iterator assetLoadIdIter = package.m_AssetLoadIds.Begin();

					int i = 0;
					for ( ; assetPathIter != package.m_AssetPaths.End(); ++assetPathIter, ++assetLoadIdIter )
					{
						*assetLoadIdIter = pAssetLoader->BeginLoadObject( *assetPathIter );
						HELIUM_ASSERT( !package.m_Assets[i++] );
					}
				}
				else
				{
					HELIUM_TRACE(
						TraceLevels::Warning,
						"Failed to load package '%s' for editor.",
						*package.m_PackagePath.ToString());
				}
			}
		}
	}

	// For each editable package
	for ( DynamicArray< ForciblyFullyLoadedPackage >::Iterator packageIter = m_ForciblyFullyLoadedPackages.Begin();
		packageIter != m_ForciblyFullyLoadedPackages.End(); ++packageIter)
	{
		ForciblyFullyLoadedPackage &package = *packageIter;

		// If the package is loaded
		if ( package.m_Package )
		{
			// Load the child assets if we need to
			for ( size_t i = 0; i < package.m_AssetPaths.GetSize(); ++i )
			{
				if ( Helium::IsValid<size_t>( package.m_AssetLoadIds[i] ) )
				{
					HELIUM_ASSERT( !package.m_Assets[i] );
					if ( pAssetLoader->TryFinishLoad( package.m_AssetLoadIds[i], package.m_Assets[i] ) )
					{
						package.m_AssetLoadIds[i] = Invalid< size_t >();

						if ( package.m_Assets[i] )
						{
							// Asset loaded successfully
							if ( !package.m_Assets[i]->IsPackage() && !package.m_Assets[i]->GetAllFlagsSet( Asset::FLAG_EDITOR_FORCIBLY_LOADED ) )
							{
								package.m_Assets[i]->SetFlags( Asset::FLAG_EDITOR_FORCIBLY_LOADED );
								e_AssetForciblyLoadedEvent.Raise( AssetEventArgs( package.m_Assets[i] ) );
							}
						}
						else
						{
							HELIUM_TRACE(
								TraceLevels::Warning,
								"Failed to asset '%s' for editor.",
								*package.m_PackagePath.ToString());
						}
					}
					else
					{
						HELIUM_ASSERT( !package.m_Assets[i] );
					}

					if ( Helium::IsValid<size_t>( package.m_AssetLoadIds[i] ) )
					{
						HELIUM_ASSERT( !package.m_Assets[i] );
					}
				}
			}
		}
	}
}

void ForciblyFullyLoadedPackageManager::ForceFullyLoadRootPackages()
{
	DynamicArray< AssetPath > rootPackages;
	AssetLoader::GetInstance()->EnumerateRootPackages( rootPackages );

	for (DynamicArray< AssetPath >::Iterator iter = rootPackages.Begin();
		iter != rootPackages.End(); ++iter)
	{
		//ForceFullyLoadPackage( *iter );
		AssetPtr package;
		AssetLoader::GetInstance()->LoadObject( *iter, package );
	}
}

void ForciblyFullyLoadedPackageManager::ForceFullyLoadPackage( const AssetPath &path )
{
	// For each forcibly loaded package
	if (IsPackageForcedFullyLoaded(path))
	{
		return;
	}

	ForciblyFullyLoadedPackage *pPackage = m_ForciblyFullyLoadedPackages.New();
	pPackage->m_PackagePath = path;
	pPackage->m_PackageLoadId = AssetLoader::GetInstance()->BeginLoadObject( path );
}

bool Helium::Editor::ForciblyFullyLoadedPackageManager::IsPackageForcedFullyLoaded( const AssetPath &path )
{
	// For each forcibly loaded package
	for ( DynamicArray< ForciblyFullyLoadedPackage >::Iterator packageIter = m_ForciblyFullyLoadedPackages.Begin();
		packageIter != m_ForciblyFullyLoadedPackages.End(); ++packageIter)
	{
		ForciblyFullyLoadedPackage &package = *packageIter;

		if ( package.m_PackagePath == path )
		{
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

uint32_t                        ThreadSafeAssetTrackerListener::sm_InitCount = 0;
ThreadSafeAssetTrackerListener* ThreadSafeAssetTrackerListener::sm_pInstance = NULL;

ThreadSafeAssetTrackerListener::ThreadSafeAssetTrackerListener()
	: m_GameThreadBufferIndex(0)
{
	AssetTracker::GetInstance()->e_AssetLoaded.AddMethod( this, &ThreadSafeAssetTrackerListener::OnAssetLoaded );
	AssetTracker::GetInstance()->e_AssetChanged.AddMethod( this, &ThreadSafeAssetTrackerListener::OnAssetChanged );
	AssetTracker::GetInstance()->e_AssetCreatedExternally.AddMethod( this, &ThreadSafeAssetTrackerListener::OnAssetCreatedExternally );
	AssetTracker::GetInstance()->e_AssetChangedExternally.AddMethod( this, &ThreadSafeAssetTrackerListener::OnAssetChangedExternally );
}

ThreadSafeAssetTrackerListener::~ThreadSafeAssetTrackerListener()
{
	AssetTracker::GetInstance()->e_AssetLoaded.RemoveMethod( this, &ThreadSafeAssetTrackerListener::OnAssetLoaded );
	AssetTracker::GetInstance()->e_AssetChanged.RemoveMethod( this, &ThreadSafeAssetTrackerListener::OnAssetChanged );
	AssetTracker::GetInstance()->e_AssetCreatedExternally.RemoveMethod( this, &ThreadSafeAssetTrackerListener::OnAssetCreatedExternally );
	AssetTracker::GetInstance()->e_AssetChangedExternally.RemoveMethod( this, &ThreadSafeAssetTrackerListener::OnAssetChangedExternally );
}

ThreadSafeAssetTrackerListener* ThreadSafeAssetTrackerListener::GetInstance()
{
	return sm_pInstance;
}

void ThreadSafeAssetTrackerListener::Startup()
{
	if ( ++sm_InitCount == 1 )
	{
		HELIUM_ASSERT( !sm_pInstance );
		sm_pInstance = new ThreadSafeAssetTrackerListener();
		HELIUM_ASSERT( sm_pInstance );
	}
}

void ThreadSafeAssetTrackerListener::Shutdown()
{
	if ( --sm_InitCount == 0 )
	{
		HELIUM_ASSERT( sm_pInstance );
		delete sm_pInstance;
		sm_pInstance = NULL;
	}
}

void ThreadSafeAssetTrackerListener::Sync()
{
	MutexScopeLock lock( m_Lock );

	Buffer &buffer = m_Buffers[ ( m_GameThreadBufferIndex++ ) % 2 ];
	
	for ( DynamicArray< AssetEventArgs >::Iterator iter = buffer.m_Loaded.Begin();
		iter != buffer.m_Loaded.End(); ++iter)
	{
		e_AssetLoaded.Raise( *iter );
	}

	for ( DynamicArray< AssetEventArgs >::Iterator iter = buffer.m_Changed.Begin();
		iter != buffer.m_Changed.End(); ++iter)
	{
		e_AssetChanged.Raise( *iter );
	}

	for ( DynamicArray< AssetEventArgs >::Iterator iter = buffer.m_CreatedExternally.Begin();
		iter != buffer.m_CreatedExternally.End(); ++iter)
	{
		e_AssetCreatedExternally.Raise( *iter );
	}

	for ( DynamicArray< AssetEventArgs >::Iterator iter = buffer.m_ChangedExternally.Begin();
		iter != buffer.m_ChangedExternally.End(); ++iter)
	{
		e_AssetChangedExternally.Raise( *iter );
	}

	buffer.m_Loaded.Clear();
	buffer.m_Changed.Clear();
	buffer.m_CreatedExternally.Clear();
	buffer.m_ChangedExternally.Clear();
}

void ThreadSafeAssetTrackerListener::OnAssetLoaded( const AssetEventArgs &args )
{
	MutexScopeLock lock( m_Lock );
	m_Buffers[ m_GameThreadBufferIndex % 2 ].m_Loaded.Push( args );
}

void ThreadSafeAssetTrackerListener::OnAssetChanged( const AssetEventArgs &args )
{
	MutexScopeLock lock( m_Lock );
	m_Buffers[ m_GameThreadBufferIndex % 2 ].m_Changed.Push( args );
}

void ThreadSafeAssetTrackerListener::OnAssetCreatedExternally( const AssetEventArgs &args )
{
	MutexScopeLock lock( m_Lock );
	m_Buffers[ m_GameThreadBufferIndex % 2 ].m_CreatedExternally.Push( args );

}

void ThreadSafeAssetTrackerListener::OnAssetChangedExternally( const AssetEventArgs &args )
{
	MutexScopeLock lock( m_Lock );
	m_Buffers[ m_GameThreadBufferIndex % 2 ].m_ChangedExternally.Push( args );
}

//////////////////////////////////////////////////////////////////////////

EditorEngine::EditorEngine()
	: m_SceneManager( NULL )
	, m_EngineTickTimer( this )
	, m_bTerminateAssetManagerThread( false )
{
	TaskScheduler::CalculateSchedule( TickTypes::Editor, m_Schedule );
}

EditorEngine::~EditorEngine()
{
}

bool EditorEngine::Initialize( Editor::SceneManager* sceneManager, void* hwnd )
{
	HELIUM_VERIFY( m_SceneManager = sceneManager );

#if HELIUM_DIRECT3D
	D3D9Renderer::Startup();
#elif HELIUM_OPENGL
	GLRenderer::Startup();
#endif
	Renderer* pRenderer = Renderer::GetInstance();
	if ( pRenderer )
	{
		Renderer::ContextInitParameters mainCtxInitParams;
		mainCtxInitParams.pWindow = hwnd;
		mainCtxInitParams.bFullscreen = false;
		mainCtxInitParams.bVsync = true;
		mainCtxInitParams.displayWidth = 64;
		mainCtxInitParams.displayHeight = 64;

		HELIUM_VERIFY( pRenderer->CreateMainContext( mainCtxInitParams ) );
	}

	RenderResourceManager::Startup();
	RenderResourceManager* pRenderResourceManager = RenderResourceManager::GetInstance();
	if( pRenderResourceManager )
	{
		pRenderResourceManager->UpdateMaxViewportSize( wxSystemSettings::GetMetric(wxSYS_SCREEN_X), wxSystemSettings::GetMetric(wxSYS_SCREEN_Y) );
	}

	DynamicDrawer::Startup();
	WorldManager::Startup();
	ForciblyFullyLoadedPackageManager::Startup();

	// Start engine tick
	m_EngineTickTimer.Start( 15 );

	// Make sure asset loader always gets ticked
	Helium::CallbackThread::Entry entry = &Helium::CallbackThread::EntryHelper<EditorEngine, &EditorEngine::DoAssetManagerThread>;
	HELIUM_VERIFY( m_TickAssetManagerThread.Create( entry, this, "Editor AssetLoader::Tick Thread", ThreadPriorities::Low ) );

	return true;
}

void EditorEngine::Cleanup()
{
	// We check m_SceneManager because MainFrame and App are calling this. MainFrame calls it because it owns m_SceneManager
	// and needs to get rid of this pointer and the below listeners before it destroys itself. The engine
	// belongs to app and gets destroyed after the MainFrame is destroyed. I want to revisit this ordering because I don't
	// like how ownership does not reflect destruction order, but for now this will get the editor to close cleanly.
	if ( m_SceneManager )
	{
		m_bTerminateAssetManagerThread = true;
		m_TickAssetManagerThread.Join();
		m_EngineTickTimer.Stop();

		ForciblyFullyLoadedPackageManager::Shutdown();
		WorldManager::Shutdown();
		DynamicDrawer::Shutdown();
		RenderResourceManager::Shutdown();
#if HELIUM_DIRECT3D
		D3D9Renderer::Shutdown();
#elif HELIUM_OPENGL
		GLRenderer::Shutdown();
#endif

		m_SceneManager = NULL;
	}
}

void EditorEngine::Tick()
{
	// Tick asset loader before every simulation update
	// This was moved to DoAssetManagerThread() to prevent UI lockups
	//AssetLoader::GetInstance()->Tick();

	// Do asset loading events/work that has to be done in the wx thread
	ForciblyFullyLoadedPackageManager::GetInstance()->Tick();
	ThreadSafeAssetTrackerListener::GetInstance()->Sync();

	WorldManager* pWorldManager = WorldManager::GetInstance();
	HELIUM_ASSERT( pWorldManager );
	pWorldManager->Update( m_Schedule );
}

void EditorEngine::DoAssetManagerThread()
{
	AssetAwareThreadSynchronizer assetSyncUtil;
	while ( !m_bTerminateAssetManagerThread )
	{
		assetSyncUtil.Sync();
		AssetLoader::GetInstance()->Tick();

		if ( !m_bTerminateAssetManagerThread )
		{
			Thread::Sleep( 100 );
		}
	}

	m_bTerminateAssetManagerThread = false;
}

EngineTickTimer::EngineTickTimer( EditorEngine* pEngine )
	: m_Engine( pEngine )
{

}

EngineTickTimer::~EngineTickTimer()
{
	Stop();
}

void EngineTickTimer::Notify()
{
	m_AssetSyncUtil.Sync();
	m_Engine->Tick();
}
