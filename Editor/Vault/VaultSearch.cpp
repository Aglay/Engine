#include "EditorPch.h"
#include "VaultSearch.h"

#include "Platform/Exception.h"
#include "Foundation/Regex.h"
#include "Foundation/DirectoryIterator.h"
#include "Foundation/Tokenize.h"

#include "VaultSearchResults.h"

using namespace Helium;
using namespace Helium::Editor;

namespace Helium
{
    namespace Editor
    {

        /////////////////////////////////////////////////////////////////////////////
        /// DummyWindowArgs
        /////////////////////////////////////////////////////////////////////////////
        struct DummyWindowArgs
        {
            int32_t m_ThreadID;
            DummyWindowArgs( int32_t threadID )
                : m_ThreadID( threadID )
            {
            }
        };
        typedef Helium::Signature< const DummyWindowArgs& > DummyWindowSignature;

        /////////////////////////////////////////////////////////////////////////////
        /// DummyWindow
        /////////////////////////////////////////////////////////////////////////////
        static const char* s_DummyWindowName = "DummyWindowThread";

        // Custom wxEventTypes for the VaultSearchThread to fire.
        DEFINE_EVENT_TYPE( EDITOR_EVT_BEGIN_SEARCH )
            DEFINE_EVENT_TYPE( EDITOR_EVT_RESULTS_AVAILABLE )
            DEFINE_EVENT_TYPE( EDITOR_EVT_SEARCH_COMPLETE )

        class DummyWindow : public wxFrame
        {
        public:
            DummyWindow( const char* name = NULL )
                : wxFrame( NULL, wxID_ANY, s_DummyWindowName, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE, s_DummyWindowName )
            {
                Hide();

                if ( name )
                {
                    wxString newName( s_DummyWindowName );
                    newName += "-";
                    newName += name;
                    SetName( newName );
                    SetTitle( newName );
                }
            }

            virtual ~DummyWindow()
            {

            }

            void OnBeginThread( wxCommandEvent& evt )
            {
                m_BeginListeners.Raise( DummyWindowArgs( evt.GetInt() ) );
            }

            void OnThreadUpdate( wxCommandEvent& evt )
            {
                m_UpdateListeners.Raise( DummyWindowArgs( evt.GetInt() ) );
            }

            void OnEndThread( wxCommandEvent& evt )
            {
                m_EndListeners.Raise( DummyWindowArgs( evt.GetInt() ) ); 
            }

            // 
            // Events
            //
        private:
            DummyWindowSignature::Event m_BeginListeners;
        public:
            void AddBeginListener( const DummyWindowSignature::Delegate& listener )
            {
                m_BeginListeners.Add( listener );
            }
            void RemoveBeginListener( const DummyWindowSignature::Delegate& listener )
            {
                m_BeginListeners.Remove( listener );
            }

        private:
            DummyWindowSignature::Event m_UpdateListeners;
        public:
            void AddUpdateListener( const DummyWindowSignature::Delegate& listener )
            {
                m_UpdateListeners.Add( listener );
            }
            void RemoveUpdateListener( const DummyWindowSignature::Delegate& listener )
            {
                m_UpdateListeners.Remove( listener );
            }

        private:
            DummyWindowSignature::Event m_EndListeners;
        public:
            void AddEndListener( const DummyWindowSignature::Delegate& listener )
            {
                m_EndListeners.Add( listener );
            }
            void RemoveEndListener( const DummyWindowSignature::Delegate& listener )
            {
                m_EndListeners.Remove( listener );
            }

        };


        /////////////////////////////////////////////////////////////////////////////
        /// VaultSearchThread
        /////////////////////////////////////////////////////////////////////////////
        class VaultSearchThread : public wxThread
        {
        private:
            VaultSearch* m_VaultSearch;
            int32_t m_SearchID;

        public:
            // Detached threads delete themselves once they have completed,
            // and thus must be created on the heap
            VaultSearchThread( VaultSearch* browserSearch, int32_t id )
                : wxThread( wxTHREAD_DETACHED )
                , m_VaultSearch( browserSearch )
                , m_SearchID( id )
            {
            }

            virtual ~VaultSearchThread()
            {
            }

            virtual wxThread::ExitCode Entry() override
            {
                m_VaultSearch->SearchThreadProc( m_SearchID );

                return NULL;
            }
        };
    }
}


/////////////////////////////////////////////////////////////////////////////
/// VaultSearch
/////////////////////////////////////////////////////////////////////////////
VaultSearch::VaultSearch( const FilePath& project )
: m_Project( project )
, m_SearchResults( NULL )
, m_StopSearching( true )
, m_DummyWindow( NULL )
, m_CurrentSearchID( -1 )
, m_CurrentSearchQuery( NULL )
, m_SearchInitializedEvent( true, true )
, m_EndSearchEvent( true, true )
{
}

VaultSearch::~VaultSearch()
{
    // wait for searching thread to complete
    StopSearchThreadAndWait();

    m_CurrentSearchQuery = NULL;
    m_SearchResults = NULL;
    m_FoundFiles.clear();

    if ( m_DummyWindow )
    {
        delete m_DummyWindow;
        m_DummyWindow = NULL;
    }
}

void VaultSearch::SetProject( const FilePath& project )
{
    m_Project = project;
}

///////////////////////////////////////////////////////////////////////////////
// Creates and starts the VaultSearchThread
//
// Detached threads delete themselves once they have completed,
// and thus must be created on the heap
//
// Everything comes through here
//  - folder path
//  - search query
//
bool VaultSearch::StartSearchThread( VaultSearchQuery* searchQuery )
{
    Helium::MutexScopeLock beginMutex( m_BeginSearchMutex );

    // kill current search, if any
    StopSearchThreadAndWait();

    Helium::MutexScopeLock resultsMutex( m_SearchResultsMutex );
    {
        // reset event to lockout new searches from starting
        ++m_CurrentSearchID;
        m_SearchInitializedEvent.Reset();
        m_StopSearching = false;
        m_StartSearchThreadListeners.Raise( StartSearchThreadArgs( searchQuery ) );

        // clear previous results, if any
        m_CurrentSearchQuery = searchQuery;
        m_SearchResults = new VaultSearchResults( m_CurrentSearchID );
        m_FoundFiles.clear();

        HELIUM_ASSERT( !m_DummyWindow );
        m_DummyWindow = new DummyWindow( "VaultSearch" );
        m_DummyWindow->Connect( m_DummyWindow->GetId(), EDITOR_EVT_BEGIN_SEARCH, wxCommandEventHandler( DummyWindow::OnBeginThread ), NULL, m_DummyWindow );
        m_DummyWindow->Connect( m_DummyWindow->GetId(), EDITOR_EVT_RESULTS_AVAILABLE, wxCommandEventHandler( DummyWindow::OnThreadUpdate ), NULL, m_DummyWindow );
        m_DummyWindow->Connect( m_DummyWindow->GetId(), EDITOR_EVT_SEARCH_COMPLETE, wxCommandEventHandler( DummyWindow::OnEndThread ), NULL, m_DummyWindow );

        m_DummyWindow->AddBeginListener( Editor::DummyWindowSignature::Delegate( this, &VaultSearch::OnBeginSearchThread ) );
        m_DummyWindow->AddUpdateListener( Editor::DummyWindowSignature::Delegate( this, &VaultSearch::OnSearchResultsAvailable ) );
        m_DummyWindow->AddEndListener( Editor::DummyWindowSignature::Delegate( this, &VaultSearch::OnEndSearchThread ) );

        // start the search thread
        VaultSearchThread* searchThread = new VaultSearchThread( this, m_CurrentSearchID );
        searchThread->Create();
        searchThread->Run();
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Something has requested the search thread to stop; possibly
// a new browserSearch is ready to be run, stop the old and start
// the new.
void VaultSearch::StopSearchThreadAndWait()
{
    // cant cancel a search until the search is initialized and m_SearchInitializedEvent is set
    m_SearchInitializedEvent.Wait();

    m_StopSearching = true;
    m_EndSearchEvent.Wait();

    if ( m_DummyWindow )
    {
        m_DummyWindow->Destroy();
        m_DummyWindow = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Main thread callbacks to notify listeners when the search has started,
// when results are available for use and when the search has completed
void VaultSearch::OnBeginSearchThread( const Editor::DummyWindowArgs& args )
{
    if ( args.m_ThreadID != m_CurrentSearchID )
        return;

    m_SearchThreadRunningListeners.Raise( SearchThreadRunningArgs() );
}

///////////////////////////////////////////////////////////////////////////////
void VaultSearch::OnSearchResultsAvailable( const Editor::DummyWindowArgs& args )
{
    if ( args.m_ThreadID != m_CurrentSearchID )
        return;

    Helium::MutexScopeLock mutex (m_SearchResultsMutex);

    if ( m_SearchResults
        && m_SearchResults->GetSearchID() != m_CurrentSearchID )
    {
        return;
    }

    // "Publish" these results, null our pointer, and clear FoundFilesIDs to continue searching
    uint32_t searchID = m_SearchResults->GetSearchID();
    if ( m_SearchResults && m_SearchResults->HasResults() )
    {
        m_SearchResultsAvailableListeners.Raise( SearchResultsAvailableArgs( m_CurrentSearchQuery, m_SearchResults ) );
    }
    m_SearchResults = new VaultSearchResults( searchID );
    m_FoundFiles.clear();
}

///////////////////////////////////////////////////////////////////////////////
void VaultSearch::OnEndSearchThread( const Editor::DummyWindowArgs& args )
{
    if ( args.m_ThreadID != m_CurrentSearchID )
    {
        return;
    }

    m_SearchThreadCompleteListeners.Raise( SearchThreadCompleteArgs( m_CurrentSearchQuery ) );
}



///////////////////////////////////////////////////////////////////////////////
//
// SearchThreadProc - Called from the VaultSearchThread
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Called by VaultSearchThread
// Fills out the VaultSearchResults structure to pass to SearchResultsAvailable event
//
void VaultSearch::SearchThreadProc( int32_t searchID )
{
	HELIUM_ASSERT( !m_Project.Empty() );

    SearchThreadEnter( searchID );

#ifdef TRACKER_REFACTOR
    TrackerDBGenerated trackerDB( "sqlite3", dbSpec.c_str() );

    // create tables, sequences and indexes
    trackerDB.verbose = true;

    if ( !m_Project->GetTrackerDB().Exists() )
    {
        trackerDB.create();
    }

    if ( trackerDB.needsUpgrade() )
    {
        trackerDB.upgrade();
    }

    if ( CheckSearchThreadLeave( searchID ) )
    {
        return;
    }

    std::vector<TrackedFile> assetFiles = litesql::select<TrackedFile>( trackerDB, TrackedFile::MPath.like( m_CurrentSearchQuery->GetSQLQueryString().c_str() ) ).all();

    {
        Helium::MutexScopeLock mutex (m_SearchResultsMutex);

        m_FoundFiles.clear();

        for ( std::vector< TrackedFile >::const_iterator itr = assetFiles.begin(), end = assetFiles.end(); itr != end; ++itr )
        {
            m_FoundFiles.insert( (*itr) );

            if ( CheckSearchThreadLeave( searchID ) )
            {
                m_FoundFiles.clear();
                return;
            }
        }

        m_SearchResults->SetResults( m_FoundFiles );
    }

    assetFiles.clear();

    if ( CheckSearchThreadLeave( searchID ) )
    {
        return;
    }
#endif

    SearchThreadLeave( searchID );
}


/////////////////////////////////////////////////////////////////////////////
// SearchThreadProc Helper Functions
// Used in Search() to create begin and end search events
//
inline void VaultSearch::SearchThreadEnter( int32_t searchID )
{
    m_EndSearchEvent.Reset();

    wxCommandEvent evt( EDITOR_EVT_BEGIN_SEARCH, m_DummyWindow->GetId() );
    evt.SetInt( searchID );
    wxPostEvent( m_DummyWindow, evt );

    // Main thread is deadlocked until VaultSearchThread sets this event
    m_SearchInitializedEvent.Signal();
}

///////////////////////////////////////////////////////////////////////////////
inline void VaultSearch::SearchThreadPostResults( int32_t searchID )
{
    Helium::MutexScopeLock mutex (m_SearchResultsMutex);

    if ( m_SearchResults && m_SearchResults->HasResults() )
    {
        wxCommandEvent evt( EDITOR_EVT_RESULTS_AVAILABLE, m_DummyWindow->GetId() );
        evt.SetInt( searchID );
        wxPostEvent( m_DummyWindow, evt );
    }
}

///////////////////////////////////////////////////////////////////////////////
inline bool VaultSearch::CheckSearchThreadLeave( int32_t searchID )
{
    if ( m_StopSearching )
    {
        SearchThreadLeave( searchID );
        return true;
    }
    return false;
}

inline void VaultSearch::SearchThreadLeave( int32_t searchID )
{
    m_StopSearching = true;

    SearchThreadPostResults( searchID );

    wxCommandEvent evt( EDITOR_EVT_SEARCH_COMPLETE, m_DummyWindow->GetId() );
    evt.SetInt( searchID );
    wxPostEvent( m_DummyWindow, evt );
    if ( m_DummyWindow )
    {
        m_DummyWindow->Destroy();
        m_DummyWindow = NULL;
    }

    // Main thread is deadlocked until VaultSearchThread sets this event
    m_EndSearchEvent.Signal();
}


/////////////////////////////////////////////////////////////////////////////
// SearchThreadProc Helper Functions - Wrangle VaultSearchResults
/////////////////////////////////////////////////////////////////////////////

uint32_t VaultSearch::Add( const TrackedFile& file, int32_t searchID )
{ 
    uint32_t numFilesAdded = 0;

    MutexScopeLock mutex (m_SearchResultsMutex);

#ifdef TRACKER_REFACTOR
    HELIUM_ASSERT( !FilePath( file.mPath.value() ).IsDirectory() );

    std::pair< std::set< TrackedFile >::const_iterator, bool > inserted = m_FoundFiles.insert( file );
    if ( m_SearchResults && inserted.second )
    {
        m_SearchResults->Add( file );
        ++numFilesAdded;
    }
#endif

    return numFilesAdded;
}
