#pragma once

#include "API.h"

#include "AssetClass.h"
#include "AssetFlags.h"

#include "Common/Memory/SmartPtr.h"
#include "Common/Types.h"
#include "Reflect/Field.h"

namespace Asset
{
    //
    // Forwards
    //
    class CacheDB;
    typedef ASSET_API Nocturnal::SmartPtr< CacheDB > CacheDBPtr;

    class AssetFile;
    typedef Nocturnal::SmartPtr< AssetFile > AssetFilePtr;
    typedef std::map< tuid, AssetFilePtr > M_AssetFiles;

    class AssetVisitor;

    /////////////////////////////////////////////////////////////////////////////
    struct TrackerArgs
    {
        bool m_IsTracking;

        TrackerArgs( bool isTracking )
            : m_IsTracking( isTracking )
        {
        }
    };
    typedef Nocturnal::Signature< void, const TrackerArgs& > TrackerSignature;

    /////////////////////////////////////////////////////////////////////////////
    class ASSET_API Tracker
    {
    public:
        Tracker( const std::string& rootDirectory, const std::string& configDirectory );
        Tracker( const Tracker& rhs );
        Tracker& operator=( const Tracker& rhs );
        ~Tracker();

        bool IsTracking() const;
        void StartThread();
        void StopThread();

        bool InitialIndexingCompleted() const { return m_InitialIndexingCompleted; }
        bool DidIndexingFail() const { return m_IndexingFailed; }

        u32 GetTrackingProgress();
        u32 GetTrackingTotal();

    private:

        // Thread entry points
        static DWORD WINAPI TrackEverythingThread(LPVOID pTracker);

        bool TrackFile( const std::string& path );
        bool TrackFile( Nocturnal::Path& filePath );
        bool TrackAssetFile( Nocturnal::Path& filePath, M_AssetFiles* assetFiles );
        void TrackEverything();

    private:
        Asset::CacheDBPtr     m_AssetCacheDB;

        static std::string    s_GlobalRootDirectory;
        const std::string     m_RootDirectory;

        HANDLE                m_Thread;
        DWORD                 m_ThreadID;
        bool                  m_StopTracking;

        M_AssetFiles          m_AssetFiles;

        bool                  m_InitialIndexingCompleted;
        bool                  m_IndexingFailed;
        u32                   m_CurrentProgress;
        u32                   m_Total;
    };
}
