#include "Windows/Windows.h"

#include "Tracker.h"
#include "CacheDB.h"

#include "Exceptions.h"
#include "AssetInit.h"
#include "AssetVisitor.h"
#include "EntityManifest.h"
#include "ArtFileAttribute.h"

#include "Attribute/AttributeHandle.h"
#include "AppUtils/AppUtils.h"
#include "Common/Container/Insert.h" 
#include "Common/Flags.h"
#include "Common/String/Utilities.h"
#include "Common/Types.h"
#include "Console/Console.h"
#include "Console/Listener.h"
#include "Content/ContentInit.h"
#include "Common/InitializerStack.h"
#include "FileSystem/FileSystem.h"
#include "Finder/Finder.h"
#include "Finder/LunaSpecs.h"
#include "Finder/AssetSpecs.h"
#include "Finder/ContentSpecs.h"
#include "Finder/AnimationSpecs.h"
#include "Finder/FinderSpec.h"
#include "Finder/ProjectSpecs.h"
#include "Reflect/Class.h"
#include "Reflect/Serializers.h"
#include "Reflect/Version.h"
#include "Reflect/Visitor.h"
#include "TUID/TUID.h"

#define SHUTDOWN_THREAD 0


using namespace Asset;

//
// Typedefs
//

typedef std::map< tuid, S_tuid > M_AssetDependencies; 


///////////////////////////////////////////////////////////////////////////////
static inline void PrependFilePath( const std::string& projectAssets, std::string& path )
{
    if ( !FileSystem::HasPrefix( projectAssets, path ) )
    {
        path = projectAssets + path;
    }
}

//
// Init/Cleanup
//

std::string Tracker::s_GlobalRootDirectory = "";

///////////////////////////////////////////////////////////////////////////////
static int g_InitCount = 0;
static Tracker* g_GlobalTracker = NULL;
void Tracker::Initialize()
{
    if ( ++g_InitCount == 1 )
    {
        g_GlobalTracker = new Tracker( Tracker::s_GlobalRootDirectory );
    }
}

///////////////////////////////////////////////////////////////////////////////
void Tracker::Cleanup()
{
    if ( --g_InitCount == 0 )
    {
        if ( g_GlobalTracker )
        {
            g_GlobalTracker->StopThread();

            delete g_GlobalTracker;
            g_GlobalTracker = NULL;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
Tracker* Asset::GlobalTracker()
{
    if ( !g_GlobalTracker )
    {
        throw Nocturnal::Exception( "GlobalTracker is not initialized, must call Asset::Tracker::Initialize() first!" );
    }
    return g_GlobalTracker;
}

///////////////////////////////////////////////////////////////////////////////
/// Tracker
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
Tracker::Tracker( const std::string& rootDirectory )
: m_AssetCacheDB( new Asset::CacheDB( "AssetTracker-AssetCacheDB" ) )
, m_RootDirectory( rootDirectory )
, m_Thread( NULL )
, m_ThreadID( 0x0 )
, m_StopTracking( false )
, m_InitialIndexingCompleted( false )
, m_IndexingFailed( false )
, m_Total( 0 )
, m_CurrentProgress( 0 )
{
}

Tracker::~Tracker()
{
    m_AssetCacheDB = NULL;
    m_AssetFiles.clear();
}

///////////////////////////////////////////////////////////////////////////////
// Entry point for AssetVisitor
//
bool Tracker::TrackAssetFile( File::ReferencePtr& fileRef, M_AssetFiles* assetFiles )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    if ( !assetFiles )
    {
        NOC_BREAK()
    }

    fileRef->Resolve();

    if ( !fileRef->GetFile().Exists() )
    {
        return false;
    }

    Nocturnal::Insert<M_AssetFiles>::Result inserted = assetFiles->insert( M_AssetFiles::value_type( fileRef->GetHash(), new AssetFile( *fileRef ) ) );
    if ( inserted.second )
    {
        // this tuid may represent an asset file
        if ( fileRef->GetFile().GetPath().Extension() == FinderSpecs::Extension::REFLECT_BINARY.GetExtension() )
        {
            try
            {
                Asset::AssetClassPtr assetClass = Asset::AssetClass::LoadAssetClass( fileRef->GetPath() );
                if ( assetClass && !m_StopTracking )
                {
                    AssetFilePtr& assetFile = inserted.first->second;
                    AssetVisitor assetVisitor( assetFiles, assetClass, &m_StopTracking );
                    assetClass->Host( assetVisitor );
                }
            }
            catch( const Nocturnal::Exception& ex )
            {
                Console::Warning( Console::Levels::Verbose, "Tracker: %s\n", ex.what() );
            }
        }
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
bool Tracker::TrackFile( const std::string& path )
{
    File::ReferencePtr fileRef = new File::Reference( path );
    return TrackFile( fileRef );
}

bool Tracker::TrackFile( File::ReferencePtr& fileRef )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    bool result = false;

    bool newTransOpened = false;


    try
    {
        //if ( file->m_WasDeleted || !FileSystem::Exists( file->m_Path ) )
        //{
        //  AssetFilePtr assetFile = new AssetFile( file );

        //  // make sure a transaction is open before inserting
        //  if ( !m_AssetCacheDB->IsTransOpen() )
        //  {
        //    m_AssetCacheDB->BeginTrans();
        //    newTransOpened = true;
        //  }
        //  m_AssetCacheDB->DeleteAssetFile( assetFile );
        //  result = true;
        //}
        //else
        if ( m_AssetCacheDB->HasAssetChangedOnDisk( *fileRef, &m_StopTracking ) )
        {
            File::S_Reference visited;
            if ( TrackAssetFile( fileRef, &m_AssetFiles ) )
            {
                // update the DB
                M_AssetFiles::iterator found = m_AssetFiles.find( fileRef->GetHash() );
                if ( found != m_AssetFiles.end() )
                {
                    AssetFilePtr& assetFile = found->second;

                    // make sure a transaction is open before inserting
                    if ( !m_AssetCacheDB->IsTransOpen() )
                    {
                        m_AssetCacheDB->BeginTrans();
                        newTransOpened = true;
                    }

                    m_AssetCacheDB->InsertAssetFile( assetFile, &m_AssetFiles, visited, &m_StopTracking );
                    result = true;
                }
            }

            if ( !result )
            {
                // still insert the file, even if we have no data for it
                AssetFilePtr assetFile = new AssetFile( *fileRef );

                // make sure a transaction is open before inserting
                if ( !m_AssetCacheDB->IsTransOpen() )
                {
                    m_AssetCacheDB->BeginTrans();
                    newTransOpened = true;
                }

                m_AssetCacheDB->InsertAssetFile( assetFile, &m_AssetFiles, visited, &m_StopTracking );
            }
        }
    }
    catch( ... )
    {
        if ( newTransOpened )
        {
            m_AssetCacheDB->RollbackTrans();
        }
        throw;
    }

    if ( newTransOpened )
    {
        m_AssetCacheDB->CommitTrans();
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// Sleep between runs and yield to other threads
// The complex loop is to prevent Luna from hanging on exit (max hang will be "increments" seconds)
inline void SleepBetweenTracking( bool* cancel = NULL, const u32 minutes = 1 )
{
    const u32 increments = 5;
    u32 totalSeconds = 60 * minutes;
    for ( u32 seconds = 0; seconds < totalSeconds; seconds += increments )
    {
        Sleep( increments * 1000 );

        if ( ( cancel != NULL )
            && *cancel )
        {
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Process over all of the assets and insert them into the asset DB
// 
void Tracker::TrackEverything()
{
    ASSETTRACKER_SCOPE_TIMER((""));

    m_StopTracking = false;
    m_InitialIndexingCompleted = false;

    ////////////////////////////////
    // Connect to the DBs
    m_IndexingFailed = false;
    {
        // Connect the Asset CacheDB
        std::string rootDir = Finder::ProjectAssets() + FinderSpecs::Project::ASSET_TRACKER_FOLDER.GetRelativeFolder();
        FileSystem::GuaranteeSlash( rootDir );
        FileSystem::MakePath( rootDir );
        if ( !m_AssetCacheDB->Open( FinderSpecs::Project::ASSET_TRACKER_DB.GetFile( rootDir ),
            FinderSpecs::Project::ASSET_TRACKER_CONFIGS.GetFolder(),
            m_AssetCacheDB->s_TrackerDBVersion ) )
        {
            Console::Error( "Tracker: Failed to open Asset Tracker Cache DB.\n" );
            m_IndexingFailed = true;
            m_StopTracking = true;
            return;
        }
    }

    V_string foundFiles;
    while ( !m_StopTracking )
    {  
        Console::Print( m_InitialIndexingCompleted ? Console::Levels::Verbose : Console::Levels::Default,
            m_InitialIndexingCompleted ? "Tracker: Looking for new or updated files...\n" : "Tracker: Finding asset files...\n" );

        ////////////////////////////////
        // Find Files
        {
            Profile::Timer timer;
            FileSystem::Find( m_RootDirectory, foundFiles, std::string( "*." ) + FinderSpecs::Extension::REFLECT_BINARY.GetExtension(), FileSystem::FindFlags::Recursive );
            Console::Print( m_InitialIndexingCompleted ? Console::Levels::Verbose : Console::Levels::Default, "Tracker: File reslover database lookup took %.2fms\n", timer.Elapsed() );
        }

        ////////////////////////////////
        // Track Files
        m_CurrentProgress = 0;
        m_Total = (u32)foundFiles.size();
        {
            Profile::Timer timer;
            Console::Print( m_InitialIndexingCompleted ? Console::Levels::Verbose : Console::Levels::Default, "Tracker: Scanning %d asset file(s) for changes...\n", (u32)foundFiles.size() );

            while ( !m_StopTracking && !foundFiles.empty() )
            {
                Console::Listener listener ( ~Console::Streams::Error );

                try
                {
                    TrackFile( foundFiles.back() );
                }
                catch( const Nocturnal::Exception& ex )
                {
                    Console::Warning( "Tracker: %s\n", ex.what() );
                }

                foundFiles.pop_back();
                m_CurrentProgress = m_Total - (u32)foundFiles.size();
            }

            if ( m_StopTracking )
            {
                u32 percentComplete = (u32)(((f32)m_CurrentProgress/(f32)m_Total) * 100);
                Console::Print( m_InitialIndexingCompleted ? Console::Levels::Verbose : Console::Levels::Default, "Tracker: Indexing (%d%% complete) pre-empted after %.2fm\n", percentComplete, timer.Elapsed() / 1000.f / 60.f );
            }
            else if ( !m_InitialIndexingCompleted )
            {
                m_InitialIndexingCompleted = true;
                Console::Print( "Tracker: Initial indexing completed in %.2fm\n", timer.Elapsed() / 1000.f / 60.f );
            }
            else 
            {
                Console::Print( Console::Levels::Verbose, "Tracker: Indexing updated in %.2fm\n" , timer.Elapsed() / 1000.f / 60.f );
            }
        }
        foundFiles.clear();

        m_Total = 0;
        m_CurrentProgress = 0;

        ////////////////////////////////
        // Recurse
        if ( !m_StopTracking )
        {
            // Sleep between runs and yield to other threads
            // The complex loop is to prevent Luna from hanging on exit (max hang will be "increments" seconds)
            SleepBetweenTracking( &m_StopTracking );
        }
    }

    m_AssetCacheDB->Close();
}

///////////////////////////////////////////////////////////////////////////////
bool Tracker::IsTracking() const
{
    bool ret = false;

    if ( m_Thread )
    {
        DWORD status;
        ::GetExitCodeThread( m_Thread, &status );
        if ( status == STILL_ACTIVE )
        {
            ret = true;
        }
    }

    return ret;
}

///////////////////////////////////////////////////////////////////////////////
void Tracker::StartThread()
{
    StopThread();
    m_Thread = CreateThread( NULL, NULL, TrackEverythingThread, this, NULL, &m_ThreadID );
}

///////////////////////////////////////////////////////////////////////////////
void Tracker::StopThread()
{
    if ( m_Thread )
    {
        m_StopTracking = true;

        do
        {
            Console::Print( "Waiting for Tracker Thread shutdown...\n" );
        }
        while ( WaitForSingleObject( m_Thread, 5 * 1000 /*wait 5 seconds*/ ) == WAIT_TIMEOUT );

        m_ThreadID = 0x0;
        ::CloseHandle( m_Thread );
        m_Thread = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Thread entry point to start tracking everything
//
DWORD WINAPI Tracker::TrackEverythingThread(LPVOID pvoid)
{
    g_GlobalTracker->TrackEverything();

    return (DWORD)0;
}

///////////////////////////////////////////////////////////////////////////////
// Returns the current progress through TrackEverything (just the files returned
// from the local resolver)
//
u32 Tracker::GetTrackingProgress()
{
    return m_CurrentProgress;
}

///////////////////////////////////////////////////////////////////////////////
// Returns the total number of files that TrackEverything will process
//
u32 Tracker::GetTrackingTotal()
{
    return m_Total;
}