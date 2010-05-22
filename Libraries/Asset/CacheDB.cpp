#include "Windows/Windows.h"

#include "CacheDBColumn.h"
#include "CacheDB.h"

#include "AssetClass.h"
#include "AssetInit.h"
#include "AssetFile.h"
#include "AssetFolder.h"
#include "EntityAsset.h"
#include "LevelAsset.h"
#include "ShaderAsset.h"

#include "Common/Boost/Regex.h"
#include "Common/Environment.h"
#include "Common/Exception.h"
#include "Common/String/Tokenize.h"
#include "Common/String/Utilities.h"
#include "Console/Console.h"
#include "Debug/Exception.h"
#include "FileSystem/FileSystem.h"
#include "Finder/AssetSpecs.h"
#include "Finder/ExtensionSpecs.h"
#include "Finder/Finder.h"
#include "Finder/ProjectSpecs.h"
#include "SQL/SQLite.h"
#include "Windows/Console.h"
#include "Windows/Thread.h"

#include "RCS/RCS.h"

//#include "sqlite/src/sqlite3.h"

using namespace Asset;

//
// Init/Cleanup
//

///////////////////////////////////////////////////////////////////////////////
static int g_InitCount = 0;
static CacheDB* g_GlobalCacheDB = NULL;
const char* CacheDB::s_TrackerDBVersion = "3.0";

void CacheDB::Initialize()
{
    if ( ++g_InitCount == 1 )
    {
        // Connect the DB
        std::string rootDir = Finder::ProjectAssets() + FinderSpecs::Project::ASSET_TRACKER_FOLDER.GetRelativeFolder();
        FileSystem::GuaranteeSlash( rootDir );
        FileSystem::MakePath( rootDir );

        g_GlobalCacheDB = new CacheDB( "Global-AssetCacheDB" );
        g_GlobalCacheDB->Open( FinderSpecs::Project::ASSET_TRACKER_DB.GetFile( rootDir ),
            FinderSpecs::Project::ASSET_TRACKER_CONFIGS.GetFolder(),
            s_TrackerDBVersion ); //,SQLITE_OPEN_READONLY | SQLITE_OPEN_CREATE );
    }
}

///////////////////////////////////////////////////////////////////////////////
void CacheDB::Cleanup()
{
    if ( --g_InitCount == 0 )
    {
        if ( g_GlobalCacheDB )
        {
            delete g_GlobalCacheDB;
            g_GlobalCacheDB = NULL;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
CacheDB* Asset::GlobalCacheDB()
{
    if ( !g_GlobalCacheDB )
    {
        throw Nocturnal::Exception( "GlobalCacheDB is not initialized, must call Asset::Initialize() first!" );
    }
    return g_GlobalCacheDB;
}


// max storage size for a query string
#define MAX_QUERY_LENGTH  2048
#define MAX_INSERT_LENGTH 2048

//const u64 s_InvalidRowID = 0;

// Users DB
const char* s_SelectUsersComputerIDSQL = "SELECT id FROM computers WHERE name=?;";
const char* s_InsertUsersComputerSQL = "INSERT INTO computers (name) VALUES ('%s');";

const char* s_SelectP4UserIDSQL = "SELECT id FROM rcs_users WHERE username=?;";
const char* s_InsertP4UserSQL = "INSERT INTO rcs_users (username) VALUES ('%s');";

// Assets DB
//  path_hash             BIGINT UNSIGNED NOT NULL,
//  path                  VARCHAR(255) NOT NULL,
//  name                  VARCHAR(100) DEFAULT NULL,
//  file_type_id          INTEGER UNSIGNED NOT NULL,
//  asset_type_id         INTEGER UNSIGNED DEFAULT NULL,
//  size                  INTEGER UNSIGNED NOT NULL DEFAULT '0',
//  rcs_user_id           INTEGER UNSIGNED NOT NULL DEFAULT '0',
//  rcs_revision          INTEGER UNSIGNED NOT NULL,
//  last_updated          TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
//

static const char* s_SelectAssetPathByIDSQL = "SELECT path FROM assets WHERE path_hash=?;";
static const char* s_SelectAssetByIDSQL =
"SELECT assets.id, assets.path_hash, assets.path, assets.size, assets.rcs_revision, engine_types.name, rcs_users.username \
FROM assets \
LEFT JOIN file_types ON file_types.id = assets.file_type_id \
LEFT JOIN engine_types ON engine_types.id = assets.asset_type_id \
LEFT JOIN rcs_users ON rcs_users.id = assets.rcs_user_id \
WHERE path_hash=?;";

static const char* s_SelectAssetRowIDSQL = "SELECT id FROM assets WHERE path_hash=?;";
static const char* s_InsertAssetFileSQL = "INSERT INTO assets (path_hash,path,name,file_type_id,rcs_user_id,asset_type_id,size,rcs_revision,last_updated) VALUES(%lld,'%q','%q',%lld,%lld,%lld,%lld,%lld,datetime('now','localtime'));";
static const char* s_UpdateAssetFileSQL =
"UPDATE assets \
SET path='%q', \
name='%q', \
file_type_id=%lld, \
rcs_user_id=%lld, \
asset_type_id=%lld, \
size=%lld, \
rcs_revision=%lld, \
last_updated=datetime('now','localtime') \
WHERE path_hash='%lld';";
static const char* s_DeleteAssetFileSQL = "DELETE FROM assets WHERE id=%lld;";

//static const char* s_SelectAssetLastUpdatedSQL = "SELECT last_updated FROM assets where path_hash=?;";
static const char* s_SelectAssetLastUpdatedSQL = "SELECT strftime(\"%s\", last_updated) FROM assets WHERE path_hash=?;";
static const char* s_SelectAssetP4RevisionSQL = "SELECT rcs_revision FROM assets WHERE path_hash=?;";

static const char* s_SelectDependenciesByIDSQL =
"SELECT assets.path_hash \
FROM assets \
WHERE assets.id IN \
( SELECT asset_usages.dependency_id \
FROM asset_usages \
WHERE asset_usages.asset_id IN ( SELECT assets.id FROM assets WHERE path_hash=? ) );";

static const char* s_SelectUsagesByIDSQL =
"SELECT assets.path_hash \
FROM assets \
WHERE assets.id IN \
( SELECT asset_usages.asset_id \
FROM asset_usages \
WHERE asset_usages.dependency_id IN ( SELECT assets.id FROM assets WHERE path_hash=? ) );";

static const char* s_ReplaceUsagesSQL = "REPLACE INTO asset_usages (asset_id,dependency_id) VALUES(%lld,%lld);";
static const char* s_DeleteUnrenewedUsagesSQL = "DELETE FROM asset_usages WHERE asset_id=%lld AND dependency_id NOT IN (%s);";
static const char* s_DeleteUsagesSQL = "DELETE FROM asset_usages WHERE asset_id=%lld;";

static const char* s_SelectFileTypeIDSQL = "SELECT id FROM file_types WHERE type=?;";
static const char* s_InsertFileTypeSQL = "INSERT INTO file_types (type) VALUES ('%s');";

static const char* s_SelectAssetTypeIDSQL = "SELECT id FROM engine_types WHERE name=?;";
static const char* s_InsertAssetTypeSQL = "INSERT INTO engine_types (name) VALUES ('%s');";

static const char* s_SelectAttributeIDSQL = "SELECT id FROM attributes WHERE name=?;";
static const char* s_FindAttributeIDSQL = "SELECT id FROM attributes WHERE name LIKE ? ESCAPE '@';";
static const char* s_InsertAttributeSQL = "INSERT INTO attributes (name) VALUES ('%s');";
static const char* s_SelectAttributesSQL = "SELECT * FROM attributes;";

static const char* s_ReplaceAssetAttribSQL = "REPLACE INTO asset_x_attribute (asset_id,attribute_id,value) VALUES (%lld,%lld,'%s');";
static const char* s_InsertAssetAttribSQL = "INSERT INTO asset_x_attribute (asset_id,attribute_id,value) VALUES (%lld,%lld,'%s');";
static const char* s_UpdateAssetAttribSQL = "UPDATE asset_x_attribute SET value=%s WHERE asset_id='%lld' AND attribute_id='%lld';";
static const char* s_DeleteUnrenewedAssetAttribsSQL = "DELETE FROM asset_x_attribute WHERE asset_id=%lld AND attribute_id NOT IN (%s);";
static const char* s_DeleteAssetAttribsSQL = "DELETE FROM asset_x_attribute WHERE asset_id=%lld;";

static const char* s_ReplaceEntitiesXShadersSQL = "REPLACE INTO entities_x_shaders (entity_id,shader_id) VALUES(%lld,%lld);";
static const char* s_DeleteUnrenewedEntitiesXShadersSQL = "DELETE FROM entities_x_shaders WHERE entity_id=%lld AND shader_id NOT IN (%s);";
static const char* s_DeleteEntitiesXShadersSQL = "DELETE FROM entities_x_shaders WHERE entity_id=%lld;";
static const char* s_DeleteAssetFromEntitiesXShadersSQL = "DELETE FROM entities_x_shaders WHERE entity_id=%lld OR shader_id=%lld;";

static const char* s_ReplaceLevelsXEntitiesSQL = "REPLACE INTO levels_x_entities (level_id,entity_id) VALUES(%lld,%lld);";
static const char* s_DeleteUnrenewedLevelsXEntitiesSQL = "DELETE FROM levels_x_entities WHERE level_id=%lld AND entity_id NOT IN (%s);";
static const char* s_DeleteLevelsXEntitiesSQL = "DELETE FROM levels_x_entities WHERE level_id=%lld;";
static const char* s_DeleteAssetFromLevelsXEntitiesSQL = "DELETE FROM levels_x_entities WHERE level_id=%lld OR entity_id=%lld;";

static const char* s_TouchLastUpdated = "UPDATE assets SET last_updated=datetime('now','utc') where path_hash='%lld'";

/////////////////////////////////////////////////////////////////////////////
inline bool CheckCancelQuery( bool* cancel )
{
    if ( cancel == NULL )
    {
        return false;
    }

    return *cancel;
}

/////////////////////////////////////////////////////////////////////////////
static inline void PrependFilePath( const std::string& projectAssets, std::string& path )
{
    if ( !FileSystem::HasPrefix( projectAssets, path ) )
    {
        path = projectAssets + path;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Ctor - Initialiaze the CacheDB
CacheDB::CacheDB( const char* friendlyName )
: SQL::SQLiteDB( friendlyName )
{
    m_CacheDBColumns.insert( std::make_pair( CacheDBColumnIDs::FileID, new CacheDBColumn( this, "path_hash", "assets", "path_hash", "path_hash,fileid,asset_id,assetid,tuid,id" ) ) );

    m_CacheDBColumns.insert( std::make_pair( CacheDBColumnIDs::Name, new CacheDBColumn( this, "name", "assets", "name", "name" ) ) );
    m_CacheDBColumns[CacheDBColumnIDs::Name]->UseInGenericSearch( false );

    m_CacheDBColumns.insert( std::make_pair( CacheDBColumnIDs::Path, new CacheDBColumn( this, "path", "assets", "path", "path,file" ) ) );

    m_CacheDBColumns.insert( std::make_pair( CacheDBColumnIDs::FileType, new CacheDBColumn( this, "file_type", "file_types", "type", "file_type,type" ) ) );
    m_CacheDBColumns[CacheDBColumnIDs::FileType]->SetJoin( "id", "assets", "file_type_id" );
    m_CacheDBColumns[CacheDBColumnIDs::FileType]->SetPopulateSQL( "SELECT * FROM file_types;" );

    m_CacheDBColumns.insert( std::make_pair( CacheDBColumnIDs::AssetType, new CacheDBColumn( this, "engine_type", "engine_types", "name", "engine_type,engine,eng,type" ) ) );    
    m_CacheDBColumns[CacheDBColumnIDs::AssetType]->SetJoin( "id", "assets", "asset_type_id" );
    m_CacheDBColumns[CacheDBColumnIDs::AssetType]->SetPopulateSQL( "SELECT * FROM engine_types;" );

    m_CacheDBColumns.insert( std::make_pair( CacheDBColumnIDs::P4User, new CacheDBColumn( this, "rcs_user", "rcs_users", "username", "rcs_user,user,usr" ) ) );
    m_CacheDBColumns[CacheDBColumnIDs::P4User]->SetJoin( "id", "assets", "rcs_user_id" );
    m_CacheDBColumns[CacheDBColumnIDs::P4User]->SetPopulateSQL( "SELECT * FROM rcs_users;" );
}


/////////////////////////////////////////////////////////////////////////////
// Dtor - Closes the CacheDB
CacheDB::~CacheDB()
{
    m_CacheDBColumns.clear();
}

/////////////////////////////////////////////////////////////////////////////
// Opens and Load the  DB; creating the DB if it does not exist. Adds the
// version to the dbfilename
bool CacheDB::Open( std::string& dbFilename, const std::string& configFolder, const std::string& version, int flags )
{ 
    std::string::size_type idx = dbFilename.rfind( '.' );
    dbFilename.insert( idx, "-" + version );
    return __super::Open( dbFilename, configFolder, version, flags );
}

/////////////////////////////////////////////////////////////////////////////
// Create the DB, either load it from the auth file, or from the versioned sql file
bool CacheDB::Load()
{
    std::string findVersionStr = "-" + m_DBVersion + ".";
    if ( m_DataFilename.find( findVersionStr ) == std::string::npos )
    {
        std::string::size_type idx = m_DataFilename.rfind( '.' );
        m_DataFilename.insert( idx, "-" + m_DBVersion );
    }

    return __super::Load();
}

/////////////////////////////////////////////////////////////////////////////
const M_CacheDBColumns& CacheDB::GetDBColumns() const
{
    return m_CacheDBColumns;
}

/////////////////////////////////////////////////////////////////////////////
// Prepares all of the statements.
void CacheDB::PrepareStatements()
{
    ASSETTRACKER_SCOPE_TIMER((""));

    Windows::TakeSection critSection( *m_GeneralCriticalSection );

    m_SelectUsersComputerIDHandle     = m_DBManager->CreateStatement( s_SelectUsersComputerIDSQL, "t" );
    m_SelectP4IDHandle                = m_DBManager->CreateStatement( s_SelectP4UserIDSQL, "t" );
    m_SelectAssetPathByIDHandle       = m_DBManager->CreateStatement( s_SelectAssetPathByIDSQL, "l" );
    m_SelectAssetByIDHandle           = m_DBManager->CreateStatement( s_SelectAssetByIDSQL, "l" );
    m_SelectAssetRowIDHandle          = m_DBManager->CreateStatement( s_SelectAssetRowIDSQL, "l" );
    m_SelectAssetLastUpdatedHandle    = m_DBManager->CreateStatement( s_SelectAssetLastUpdatedSQL, "l" );
    m_SelectAssetP4RevisionHandle     = m_DBManager->CreateStatement( s_SelectAssetP4RevisionSQL, "l" );
    m_SelectDependenciesByIDHandle    = m_DBManager->CreateStatement( s_SelectDependenciesByIDSQL, "l" );
    m_SelectUsagesByIDHandle          = m_DBManager->CreateStatement( s_SelectUsagesByIDSQL, "l" );
    m_SelectFileTypeIDHandle          = m_DBManager->CreateStatement( s_SelectFileTypeIDSQL, "t" );
    m_SelectAssetTypeIDHandle        = m_DBManager->CreateStatement( s_SelectAssetTypeIDSQL, "t" );
    m_SelectAttributeIDHandle         = m_DBManager->CreateStatement( s_SelectAttributeIDSQL, "t" );
    m_FindAttributeIDHandle           = m_DBManager->CreateStatement( s_FindAttributeIDSQL, "t" );
    m_SelectAttributesHandle          = m_DBManager->CreateStatement( s_SelectAttributesSQL );

    M_CacheDBColumns::iterator colItr = m_CacheDBColumns.begin();
    M_CacheDBColumns::iterator colEnd = m_CacheDBColumns.end();
    for ( ; colItr != colEnd; ++colItr )
    {
        CacheDBColumn* cacheDBColumn = ( *colItr ).second;

        if ( !cacheDBColumn->GetPopulateSQL().empty() )
        {
            cacheDBColumn->SetPopulateStmtHandle( m_DBManager->CreateStatement( cacheDBColumn->GetPopulateSQL().c_str() ) );
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// single utility function to do all the cleaning we need 
// 
void CacheDB::CleanExpressionForSQL( std::string& argument, bool wrapEscape )
{ 
    if ( wrapEscape )
    {
        argument = "*" + argument + "*";
    }

    // convert asterisks to percent 
    const boost::regex asterisk("[*]+"); 
    argument = boost::regex_replace(argument, asterisk, "%"); 

    // escape underscores
    const boost::regex underscore("[_]"); 
    argument = boost::regex_replace(argument, underscore, "@_"); 
}

/////////////////////////////////////////////////////////////////////////////
// Queries the DB using a select statement that has a single text value
// Note: If it is not in the DB it and an insert statement has been passed in
//        this function will insert the value into the DB
//
u64 CacheDB::SelectIDByName( SQL::StmtHandle select, const char* value, const char* insert, bool* cancel )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    if ( CheckCancelQuery( cancel ) )
        return 0;

    NOC_ASSERT( select );

    Windows::TakeSection critSection( *m_GeneralCriticalSection );

    u64 id = 0;
    int execResult = m_DBManager->ExecStatement( select, value );
    if ( execResult == SQLITE_ROW )
    {
        m_DBManager->GetColumnInt( select, 0, ( i32& ) id );
        m_DBManager->ResetStatement( select );
    }
    else if ( execResult != SQLITE_DONE && execResult != SQLITE_OK )
    {
        throw SQL::DBManagerException( m_DBManager, __FUNCTION__, "Failed to execute SQL" );
    }
    else if ( insert )
    {
        char* insertDumbPtr = NULL;

        char insertBuff[MAX_INSERT_LENGTH];
        sprintf_s( insertBuff, sizeof( insertBuff ), insert, value, value /*TODO: [ckosan] this needs to be a long name*/ );
        insertDumbPtr = &insertBuff[0];

        execResult = m_DBManager->ExecSQLVMPrintF( insertDumbPtr, id );
        if ( execResult == SQLITE_OK )
        {
            id = m_DBManager->GetLastInsertRowId();
        }
        else
        {
            throw SQL::DBManagerException( m_DBManager, __FUNCTION__, "Failed to execute SQL: %s", insert );
        }
    }

    return id;
}

/////////////////////////////////////////////////////////////////////////////
u64 CacheDB::FindAttributeRowID( const std::string& value )
{
    std::string cleanValue = "*" + value + "*";
    CleanExpressionForSQL( cleanValue );

    return SelectIDByName( m_FindAttributeIDHandle, cleanValue.c_str() );
}

/////////////////////////////////////////////////////////////////////////////
u32 CacheDB::GetAttributesTableData( V_string& tableData, bool* cancel )
{
    u32 result = GetPopulateTableData( m_SelectAttributesHandle, tableData, cancel );
    if ( result > 0 )
    {
        V_string::iterator itr = tableData.begin(), end = tableData.end();
        for ( ; itr != end; ++itr )
        {
            std::string::size_type classNamePos = (*itr).rfind( ':' );
            if ( classNamePos != std::string::npos &&
                ( classNamePos + 1 ) < (*itr).size() )
            {
                (*itr).erase( 0, classNamePos + 1 );
            }

            classNamePos = (*itr).find( "Attribute" );
            if ( classNamePos != std::string::npos &&
                ( classNamePos > 0 ) )
            {
                (*itr).erase( classNamePos );
            }
        }
    }

    return result;
}

/////////////////////////////////////////////////////////////////////////////
// Queries the DB using a select statement that has a 3 u64 ids
// Note: If it is not in the DB it and an insert statement has been passed in
//        this function will insert the value into the DB, also note that the 
//        insert statement must be completely filled out before being passed
//        into this function since it requires more params than we have at this
//        point.
//
u64 CacheDB::SelectAssetRowID( u64 fileId, const char* insert, const char* update, bool* cancel )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    Windows::TakeSection critSection( *m_GeneralCriticalSection );

    u64 id = 0;
    int execResult = m_DBManager->ExecStatement( m_SelectAssetRowIDHandle, (i64) fileId );
    if ( execResult == SQLITE_ROW )
    {
        m_DBManager->GetColumnI64( m_SelectAssetRowIDHandle, 0, ( i64& ) id );
        m_DBManager->ResetStatement( m_SelectAssetRowIDHandle );
        if( update )
        {
            execResult = m_DBManager->ExecSQLVMPrintF( update, ( i64 ) id );
            if ( execResult != SQLITE_OK )
            {
                throw SQL::DBManagerException( m_DBManager, __FUNCTION__, "Failed to execute SQL: %s", update );
            }
        } 
    }
    else if ( execResult != SQLITE_DONE && execResult != SQLITE_OK )
    {
        throw SQL::DBManagerException( m_DBManager, __FUNCTION__, "Failed to execute SQL" );
    }
    else if ( insert )
    {
        execResult = m_DBManager->ExecSQLVMPrintF( insert, ( i64 ) id );
        if ( execResult == SQLITE_OK )
        {
            id = m_DBManager->GetLastInsertRowId();
        }
        else
        {
            throw SQL::DBManagerException( m_DBManager, __FUNCTION__ );
        }
    }

    return id;
}

/////////////////////////////////////////////////////////////////////////////
// Insert attributes
//
void CacheDB::InsertAssetAttributes( AssetFile* assetFile, bool* cancel )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    if ( CheckCancelQuery( cancel ) )
        return;

    Windows::TakeSection critSection( *m_GeneralCriticalSection );

    int execResult = SQLITE_OK;
    std::string validAttributeIDsStr;

    M_string::const_iterator attrItr = assetFile->GetAttributes().begin();
    M_string::const_iterator attrEnd = assetFile->GetAttributes().end();
    for ( ; attrItr != attrEnd; ++attrItr )
    {
        if ( CheckCancelQuery( cancel ) )
            return;

        const std::string& attrName = attrItr->first;
        u64 attributeID = SelectIDByName( m_SelectAttributeIDHandle, attrName.c_str(), s_InsertAttributeSQL, cancel  );

        std::string attrValue = attrItr->second;    

        if ( attrValue.empty() )
        {
            attrValue = "NULL";
        }

        //    if ( sizeof( queryAttrStr ) > attrValue.length() * 2 )
        //    {
        //      memset( attrValueBuff, '\0', sizeof( queryAttrStr ) );
        ////      mysql_real_escape_string( m_DBManager->GetDBHandle(), attrValueBuff, attrValue.c_str(), (unsigned long) attrValue.length() );
        //      //sprintf_s( queryAttrStr, sizeof(queryAttrStr), s_InsertReportsEnvSQL, attrValueBuff );
        //      attrValue = "'";
        //      attrValue += attrValueBuff;
        //      attrValue += "'";
        //    }
        //    else
        //    {
        //      //sprintf_s( queryAttrStr, sizeof(queryAttrStr), s_InsertReportsEnvSQL, "Environment too large" );
        //      attrValue = "NULL";
        //    }

        execResult = m_DBManager->ExecSQLVMPrintF( s_ReplaceAssetAttribSQL, ( i64 ) assetFile->GetRowID(), ( i64 ) attributeID, attrValue.c_str() );  
        if ( execResult != SQLITE_OK )
        {
            throw SQL::DBManagerException( m_DBManager, __FUNCTION__, "Failed to insert asset attribute." );
        }

        // append to the list of valid IDs
        if ( !validAttributeIDsStr.empty() )
        {
            validAttributeIDsStr += ",";
        }

        std::stringstream idStr;
        idStr << "'" << attributeID << "'";  
        validAttributeIDsStr += idStr.str();
    }

    // Delete asset usages that are not in the given set of attributeIDs,
    if ( validAttributeIDsStr.empty() )
    {
        execResult = m_DBManager->ExecSQLVMPrintF( s_DeleteAssetAttribsSQL, assetFile->GetRowID() );  
    }
    else
    {
        execResult = m_DBManager->ExecSQLVMPrintF( s_DeleteUnrenewedAssetAttribsSQL, assetFile->GetRowID(), validAttributeIDsStr.c_str() );  
    }

    if ( execResult != SQLITE_OK )
    {
        throw SQL::DBManagerException( m_DBManager, __FUNCTION__, "Failed to delete legacy asset attributes," );
    }
}


/////////////////////////////////////////////////////////////////////////////
// Insert a single shader usage row into the AssetDependenciesDB
void CacheDB::InsertAssetUsages( AssetFile* assetFile, M_AssetFiles* assetFiles, File::S_Reference& visited, bool* cancel )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    if ( CheckCancelQuery( cancel ) )
        return;

    File::S_Reference dependencies = assetFile->GetDependencies();

    if ( !dependencies.empty() )
    {
        InsertDependencies( dependencies, 
            assetFile->GetRowID(), 
            assetFiles, 
            visited,
            s_ReplaceUsagesSQL, 
            s_DeleteUsagesSQL, 
            s_DeleteUnrenewedUsagesSQL,
            cancel );
    }
}

/////////////////////////////////////////////////////////////////////////////
// Insert all (not just direct dependency) shaders that this asset depends on 
void CacheDB::InsertAssetShaders( AssetFile* assetFile, M_AssetFiles* assetFiles, File::S_Reference& visited, bool* cancel )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    if ( CheckCancelQuery( cancel ) )
        return;

    if ( assetFile->HasDependencies() )
    {
        File::S_Reference shaders;
        assetFile->GetDependenciesOfType( assetFiles, Reflect::GetType<ShaderAsset>(), shaders );

        if ( CheckCancelQuery( cancel ) )
            return;

        InsertDependencies( shaders, 
            assetFile->GetRowID(), 
            assetFiles,
            visited,
            s_ReplaceEntitiesXShadersSQL, 
            s_DeleteEntitiesXShadersSQL, 
            s_DeleteUnrenewedEntitiesXShadersSQL,
            cancel );
    }
}

/////////////////////////////////////////////////////////////////////////////
// Insert all (not just direct dependency) entities that this level depends on 
void CacheDB::InsertLevelEntities( AssetFile* assetFile, M_AssetFiles* assetFiles, File::S_Reference& visited, bool* cancel )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    if ( CheckCancelQuery( cancel ) )
        return;

    if ( !assetFile->GetDependencies().empty() )
    {
        File::S_Reference entities;
        assetFile->GetDependenciesOfType( assetFiles, Reflect::GetType<EntityAsset>(), entities );

        if ( CheckCancelQuery( cancel ) )
            return;

        InsertDependencies( entities, 
            assetFile->GetRowID(), 
            assetFiles,
            visited,
            s_ReplaceLevelsXEntitiesSQL, 
            s_DeleteLevelsXEntitiesSQL, 
            s_DeleteUnrenewedLevelsXEntitiesSQL,
            cancel );
    }
}

/////////////////////////////////////////////////////////////////////////////
void CacheDB::InsertAssetFile( AssetFile* assetFile, M_AssetFiles* assetFiles, File::S_Reference& visited, bool* cancel )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    if ( CheckCancelQuery( cancel ) )
    {
        return;
    }

    if ( !assetFile || !assetFiles )
    {
        return;
    }

    // Insert the dependencies first since we will need their row in the DB to reference them to the parents
    if ( assetFile->HasDependencies() )
    {
        File::S_Reference assetDependencies = assetFile->GetDependencies();
        File::S_Reference::const_iterator itr = assetDependencies.begin();
        File::S_Reference::const_iterator end = assetDependencies.end();
        for ( ; itr != end; ++itr )
        {
            if ( CheckCancelQuery( cancel ) )
            {
                return;
            }

            File::ReferencePtr fileRef = (*itr);

            if ( visited.find( fileRef ) == visited.end() )
            {
                visited.insert( fileRef );

                M_AssetFiles::iterator foundFile = assetFiles->find( fileRef->GetHash() );
                if ( foundFile != assetFiles->end() )
                {
                    AssetFile* assetDependency = foundFile->second;

                    if ( HasAssetChangedOnDisk( *fileRef ) )
                    {
                        InsertAssetFile( assetDependency, assetFiles, visited, cancel );
                    }
                    else
                    {
                        // This dependency is already in the DB so make sure we put the rowID on it
                        u64 rowID = SelectAssetRowID( assetDependency->GetFileReference()->GetHash(), NULL, NULL, cancel );
                        assetDependency->SetRowID( rowID );
                    }
                }
            }
        }
    }

    if ( CheckCancelQuery( cancel ) )
    {
        return;
    }

//    u64 assetTypeID  = SelectIDByName( m_SelectAssetTypeIDHandle, assetFile->GetAssetType(), s_InsertAssetTypeSQL, cancel );

    if ( !assetFile->GetModifierSpec() )
    {
        Console::Error( "No modifier spec for file '%s'\n", assetFile->GetFilePath().c_str() );
        return;
    }

    u64 fileTypeID    = SelectIDByName( m_SelectFileTypeIDHandle, assetFile->GetModifierSpec()->GetModifier().c_str(), s_InsertFileTypeSQL, cancel );

    if ( CheckCancelQuery( cancel ) )
    {
        return;
    }

    std::string relativeFilePath = assetFile->GetFileReference()->GetRelativePath();

    RCS::File rcsFile( assetFile->GetFileReference()->GetPath() );
    rcsFile.GetInfo();

    SQL::SQLiteString insertSqlString(
        s_InsertAssetFileSQL,
        (i64) assetFile->GetFileReference()->GetHash(),
        relativeFilePath.c_str(),
        assetFile->GetShortName().c_str(),
        (i64) fileTypeID,
        (i64) 0, //p4UserID,
        (i64) 0, //assetTypeID,
        (i64) assetFile->GetSize(),
        (i64) rcsFile.m_LocalRevision );

    SQL::SQLiteString updateSqlString(
        s_UpdateAssetFileSQL,
        relativeFilePath.c_str(),
        assetFile->GetShortName().c_str(),
        (i64) fileTypeID,
        (i64) 0, //p4UserID,
        (i64) 0, //assetTypeID,
        (i64) assetFile->GetSize(),
        (i64) rcsFile.m_LocalRevision,
        (i64) assetFile->GetFileReference()->GetHash() );

    // insert the asset
    u64 rowID = SelectAssetRowID( assetFile->GetFileReference()->GetHash(), insertSqlString.GetString(), updateSqlString.GetString(), cancel );
    assetFile->SetRowID( rowID );

    InsertAssetAttributes( assetFile, cancel );
    InsertAssetUsages( assetFile, assetFiles, visited, cancel );

    if ( FileSystem::HasExtension( assetFile->GetFilePath(), FinderSpecs::Asset::ENTITY_DECORATION.GetDecoration() ) )
    {
        InsertAssetShaders( assetFile, assetFiles, visited, cancel );
    }
    else if ( FileSystem::HasExtension( assetFile->GetFilePath(), FinderSpecs::Asset::LEVEL_DECORATION.GetDecoration() ) )
    {
        InsertLevelEntities( assetFile, assetFiles, visited, cancel );
    }
}

/////////////////////////////////////////////////////////////////////////////
void CacheDB::DeleteAssetFile( AssetFile* assetFile )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    Windows::TakeSection critSection( *m_GeneralCriticalSection );

    u64 rowID = SelectAssetRowID( assetFile->GetFileReference()->GetHash() );

    if ( rowID > 0 )
    {
        int execResult = SQLITE_OK;
        execResult = m_DBManager->ExecSQLVMPrintF( s_DeleteAssetFileSQL, rowID ); 
        execResult = m_DBManager->ExecSQLVMPrintF( s_DeleteUsagesSQL, rowID ); 
        execResult = m_DBManager->ExecSQLVMPrintF( s_DeleteAssetAttribsSQL, rowID ); 
        execResult = m_DBManager->ExecSQLVMPrintF( s_DeleteAssetFromEntitiesXShadersSQL, rowID, rowID ); 
        execResult = m_DBManager->ExecSQLVMPrintF( s_DeleteAssetFromLevelsXEntitiesSQL, rowID, rowID );
    }
}

/////////////////////////////////////////////////////////////////////////////
bool CacheDB::HasAssetChangedOnDisk( File::Reference& fileRef, bool* cancel )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    if ( CheckCancelQuery( cancel ) )
        return false;

    if ( !fileRef.GetFile().Exists() )
    {
        return false;
    }

    Windows::TakeSection critSection( *m_GeneralCriticalSection );

    bool ret = false;

    int lastUpdatedSqlResult = m_DBManager->ExecStatement( m_SelectAssetLastUpdatedHandle, (i64) fileRef.GetHash() );

    if ( lastUpdatedSqlResult == SQLITE_DONE )
    {
        ret = true;
    }
    else if ( lastUpdatedSqlResult != SQLITE_ROW )
    {
        ret = false;
    }
    else if ( lastUpdatedSqlResult == SQLITE_ROW )
    {
        // Get the timestamp from the column and check to see if the last modified time on disc is more recent than in the DB
        __time64_t dbModifiedTime;

        m_DBManager->GetColumnI64( m_SelectAssetLastUpdatedHandle, 0, dbModifiedTime );
        m_DBManager->ResetStatement( m_SelectAssetLastUpdatedHandle );

        // If the file is newer on disc than in the DB
        if ( fileRef.GetFile().HasChangedSince( dbModifiedTime ) )
        {
            if ( CheckCancelQuery( cancel ) )
                return false;

            // If the file is writeable then update it
            if ( fileRef.GetFile().IsWritable() )
            {
                ret = true;
            }
            // It's not writeable but is newer then get the P4 revision and check that
            else
            {
                RCS::File rcsFile( fileRef.GetPath() );
                rcsFile.GetInfo();
                if ( rcsFile.m_LocalRevision > 0 )     // if it's not on disc then ignore it
                {
                    if ( CheckCancelQuery( cancel ) )
                        return false;

                    int sqlResult = m_DBManager->ExecStatement( m_SelectAssetP4RevisionHandle, (i64) fileRef.GetHash() );

                    if ( sqlResult == SQLITE_DONE )
                    {
                        ret = true;
                    }
                    else if ( sqlResult != SQLITE_ROW )
                    {
                        ret = false;
                    }
                    else if ( sqlResult == SQLITE_ROW )
                    {
                        // Get the timestamp from the column and check to see if the last modified time on disc is more recent than in the DB
                        i32 dbP4Revision;
                        m_DBManager->GetColumnInt( m_SelectAssetP4RevisionHandle, 0, dbP4Revision );
                        m_DBManager->ResetStatement( m_SelectAssetP4RevisionHandle );

                        if ( rcsFile.m_LocalRevision > dbP4Revision )
                        {
                            ret = true;
                        }
                        else
                        {
                            // Our timestamp is out of whack compared to the perforce revision, so update the
                            // timestamp in the db, that way we early out next time the tracker runs over this file.
                            char updateBuff[MAX_INSERT_LENGTH];
                            sprintf_s( updateBuff, sizeof( updateBuff ), s_TouchLastUpdated, (i64) fileRef.GetHash() );

                            u64 rowID = 0;
                            int execResult = m_DBManager->ExecSQLVMPrintF( &updateBuff[0], rowID );
                            if ( execResult != SQLITE_OK )
                            {
                                // Try to force the caller to update this row in the database
                                Console::Error( "Failed to update timestamp on file '%s' ["TUID_HEX_FORMAT"].\n", fileRef.GetFile().GetPath().c_str(), fileRef.GetHash() );
                                ret = true;
                            }
                        }
                    }
                }
            }
        }
    }

    return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Common function used by several of the insert functions
void CacheDB::InsertDependencies
(
 const File::S_Reference& dependencies,
 u64 assetRowID,
 M_AssetFiles* assetFiles,
 File::S_Reference& visited, 
 const char* replaceSQL, 
 const char* deleteSQL, 
 const char* deleteUnrenewedSQL, 
 bool* cancel
 )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    Windows::TakeSection critSection( *m_GeneralCriticalSection );

    int execResult = SQLITE_OK;

    // build up the string for checking which ids should be in the given graph
    std::string validDependenciesStr;

    validDependenciesStr.clear();

    File::S_Reference::const_iterator itr = dependencies.begin();
    File::S_Reference::const_iterator end = dependencies.end();
    for ( ; itr != end; ++itr )
    {
        if ( CheckCancelQuery( cancel ) )
            return;

        const File::ReferencePtr& fileRef = (*itr);

        if ( assetFiles->find( fileRef->GetHash() ) == assetFiles->end() )
        {
            // this is where we used to call the Tracker on dependency files
            // if we hit this, something has gone wrong adn the Asset::Visitor is not 
            // correctly finding asset dependencies
            NOC_BREAK();
        }

        u64 dependencyRowID = (*assetFiles)[ fileRef->GetHash() ]->GetRowID();

        if ( dependencyRowID == 0 )
        {
            InsertAssetFile( (*assetFiles)[ fileRef->GetHash() ], assetFiles, visited, cancel ); 

            dependencyRowID = (*assetFiles)[ fileRef->GetHash() ]->GetRowID();
        }

        // insert into the db
        execResult = m_DBManager->ExecSQLVMPrintF( replaceSQL, assetRowID, dependencyRowID );  
        if ( execResult != SQLITE_OK )
        {
            throw SQL::DBManagerException( m_DBManager, __FUNCTION__, "Failed to insert file usage." );
        }

        // append to the list of valid IDs
        if ( !validDependenciesStr.empty() )
        {
            validDependenciesStr += ",";
        }

        std::stringstream idStr;
        idStr << "'" << dependencyRowID << "'";  
        validDependenciesStr += idStr.str();

    }

    // Delete asset usages that are not in the given set of Dependencies,
    if ( validDependenciesStr.empty() )
    {
        execResult = m_DBManager->ExecSQLVMPrintF( deleteSQL, assetRowID );  
    }
    else
    {
        execResult = m_DBManager->ExecSQLVMPrintF( deleteUnrenewedSQL, assetRowID, validDependenciesStr.c_str() );  
    }

    if ( execResult != SQLITE_OK )
    {
        throw SQL::DBManagerException( m_DBManager, __FUNCTION__, "Failed to delete legacy file usage." );
    }
}

/////////////////////////////////////////////////////////////////////////////
u32 CacheDB::GetPopulateTableData( const CacheDBColumnID columnID, V_string& tableData, bool* cancel )
{
    M_CacheDBColumns::const_iterator findColumn = m_CacheDBColumns.find( columnID );
    if ( findColumn != m_CacheDBColumns.end() )
    {
        CacheDBColumn* cacheDBColumn = findColumn->second;
        return GetPopulateTableData( cacheDBColumn->GetPopulateStmtHandle(), tableData, cancel );
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
u32 CacheDB::GetPopulateTableData( const SQL::StmtHandle stmt, M_CacheDBTableData& tableData, bool* cancel )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    Windows::TakeSection critSection( *m_GeneralCriticalSection );

    u32 numFilesAdded = 0;

    if ( stmt != SQL::NullStatement )
    {
        int sqlResult = m_DBManager->ExecStatement( stmt );
        // if no files were found return false
        if (  sqlResult != SQLITE_DONE )
        {
            // if an unexpected error occurred throw an exception
            if ( sqlResult != SQLITE_ROW )
            {
                throw SQL::DBManagerException( m_DBManager, __FUNCTION__ );
            }

            // Step through one or more entries, and pull the data from the columns
            u64 rowID;
            std::string value;
            while ( sqlResult == SQLITE_ROW )
            {
                if ( CheckCancelQuery( cancel ) )
                    break;

                m_DBManager->GetColumnI64( stmt,  0, ( i64& ) rowID );
                m_DBManager->GetColumnText( stmt, 1, value );

                tableData.insert( M_CacheDBTableData::value_type( rowID, value ) );
                ++numFilesAdded;

                sqlResult = m_DBManager->StepStatement( stmt );
            }

            m_DBManager->ResetStatement( stmt );
        }
    }

    return numFilesAdded;
}

/////////////////////////////////////////////////////////////////////////////
u32 CacheDB::GetPopulateTableData( const SQL::StmtHandle stmt, V_string& tableData, bool* cancel )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    Windows::TakeSection critSection( *m_GeneralCriticalSection );

    u32 numFilesAdded = 0;

    if ( stmt != SQL::NullStatement )
    {
        int sqlResult = m_DBManager->ExecStatement( stmt );
        // if no files were found return false
        if (  sqlResult != SQLITE_DONE )
        {
            // if an unexpected error occurred throw an exception
            if ( sqlResult != SQLITE_ROW )
            {
                throw SQL::DBManagerException( m_DBManager, __FUNCTION__ );
            }

            // Step through one or more entries, and pull the data from the columns
            u64 rowID;
            std::string value;
            while ( sqlResult == SQLITE_ROW )
            {
                if ( CheckCancelQuery( cancel ) )
                    break;

                m_DBManager->GetColumnI64( stmt,  0, ( i64& ) rowID );
                m_DBManager->GetColumnText( stmt, 1, value );

                tableData.push_back( value );
                ++numFilesAdded;

                sqlResult = m_DBManager->StepStatement( stmt );
            }

            m_DBManager->ResetStatement( stmt );
        }
    }

    return numFilesAdded;
}

/////////////////////////////////////////////////////////////////////////////
// Returns number of files found  if anything was added to assetFiles
u32 CacheDB::Search( const CacheDBQuery* search, File::S_Reference& assetFiles, bool* cancel )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    u32 numFilesAdded = 0;

    Windows::TakeSection critSection( *m_GeneralCriticalSection );

    std::string selectStmt;
    GetSelectStmt( search, selectStmt );

    if ( selectStmt.empty() )
    {
        return 0;
    }

    SQL::StmtHandle selectHandle = m_DBManager->CreateStatement( selectStmt.c_str() );

    if ( CheckCancelQuery( cancel ) )
    {
        return 0;
    }

    int sqlResult = m_DBManager->ExecStatement( selectHandle );
    // if no files were found return false
    if (  sqlResult == SQLITE_DONE )
    {
        return 0;
    }

    // if an unexpected error occurred throw an exception
    if ( sqlResult != SQLITE_ROW )
    {
        throw SQL::DBManagerException( m_DBManager, __FUNCTION__ );
    }

    // Step through one or more entries, and pull the data from the columns
    while ( sqlResult == SQLITE_ROW )
    {
        if ( CheckCancelQuery( cancel ) )
        {
            break;
        }

        std::string path = StepSelectPath( sqlResult, selectHandle );
        File::ReferencePtr assetRef = new File::Reference( path );
        assetRef->Resolve();
        assetFiles.insert( assetRef );

        sqlResult = m_DBManager->StepStatement( selectHandle );
    }

    m_DBManager->ResetStatement( selectHandle );
    m_DBManager->FinalizeStatement( selectHandle );

    return numFilesAdded;
}

/////////////////////////////////////////////////////////////////////////////
// Populate the ManagedFilePtr with row returned from the query
// Returns true if a file was successfully read from the DB
std::string CacheDB::StepSelectPath( int sqlResult, const SQL::StmtHandle stmt, bool resetStmt )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    Windows::TakeSection critSection( *m_GeneralCriticalSection );

    if ( sqlResult == SQLITE_DONE )
    {
        return NULL;
    }
    else if ( sqlResult != SQLITE_ROW )
    {
        throw SQL::DBManagerException( m_DBManager, __FUNCTION__ );
    }
    else if ( sqlResult == SQLITE_ROW )
    {
        std::string path;
        m_DBManager->GetColumnText( stmt, 0, path );

        if ( resetStmt )
        {
            m_DBManager->ResetStatement( stmt );
        }

        return path;
    }

    return std::string();
}

///////////////////////////////////////////////////////////////////////////////
void CacheDB::GetSelectStmt( const CacheDBQuery* search, std::string& selectStmt )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    // SELECT
    std::string selectExpr;
    {
        selectExpr += "SELECT assets.path\n";
    }

    // FROM
    std::string fromExpr;
    {
        fromExpr += "FROM assets\n";
    }

    // WHERE
    // We get the whereExpr before joinExpr to see what tables we'll need to join
    std::string whereExpr;
    S_CacheDBColumnID joinTables;
    {
        search->GetExpression( m_CacheDBColumns, whereExpr, joinTables );
        if ( whereExpr.empty() )
        {
            return;
        }
        whereExpr = "WHERE " + whereExpr + ";";
    }

    // JOIN
    std::string joinExpr;
    {
        S_CacheDBColumnID::const_iterator colItr = joinTables.begin(), colEnd = joinTables.end();
        for ( ; colItr != colEnd; ++colItr )
        {
            M_CacheDBColumns::const_iterator foundCol = m_CacheDBColumns.find( *colItr );
            if ( foundCol != m_CacheDBColumns.end() )
            {
                CacheDBColumn* cacheDBColumn = ( *foundCol ).second;

                // the join statement is optional
                if ( !cacheDBColumn->GetJoin().empty() )
                {
                    joinExpr += cacheDBColumn->GetJoin();
                }
            }
            else
            {
                NOC_BREAK();
            }
        }
    }

    // Build up the complete select statement
    selectStmt = selectExpr;
    selectStmt += fromExpr;
    selectStmt += joinExpr;
    selectStmt += whereExpr;
}

/////////////////////////////////////////////////////////////////////////////
void CacheDB::SelectAssetPathByHash( const u64 pathHash, std::string& path )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    Windows::TakeSection critSection( *m_GeneralCriticalSection );

    SQL::StmtHandle stmt = m_SelectAssetPathByIDHandle;
    int sqlResult = m_DBManager->ExecStatement( stmt, (i64) pathHash );
    if ( sqlResult == SQLITE_DONE )
    {
        return;
    }
    else if ( sqlResult != SQLITE_ROW )
    {
        throw SQL::DBManagerException( m_DBManager, __FUNCTION__ );
    }
    else if ( sqlResult == SQLITE_ROW )
    {
        int index = 0;

        m_DBManager->GetColumnText( stmt, 0, path );

        // make absolute
        File::Reference fileRef( path );
        path = fileRef.GetPath();

        m_DBManager->ResetStatement( stmt );
    }
}

/////////////////////////////////////////////////////////////////////////////
// Assets DB
//  path_hash               BIGINT UNSIGNED NOT NULL,
//  path                  VARCHAR(255) NOT NULL,
//  name                  VARCHAR(100) DEFAULT NULL,
//  file_type_id          INTEGER UNSIGNED NOT NULL,
//  asset_type_id        INTEGER UNSIGNED DEFAULT NULL,
//  size                  INTEGER UNSIGNED NOT NULL DEFAULT '0',
//  rcs_user_id            INTEGER UNSIGNED NOT NULL DEFAULT '0',
//  rcs_revision           INTEGER UNSIGNED NOT NULL,
//  last_updated          TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
//
// AssetFile
//  std::string   m_ShortName;
//  const Finder::ModifierSpec* m_ModifierSpec;
//  std::string   m_Extension;
//  std::string   m_FileType;
//  std::string   m_AssetType;
//  u64           m_Size;
//  std::string   m_P4User;
//  M_string      m_Attributes;
//  S_tuid        m_Dependencies;
//  u64           m_RowID;
//  i32           m_P4LocalRevision;
//
// assets.id, assets.path_hash, assets.path, assets.size, assets.rcs_revision, file_types.type, engine_types.name, rcs_users.username
// assets.id, assets.path_hash, assets.path, assets.size, assets.rcs_revision, engine_types.name, rcs_users.username
//
void CacheDB::SelectAssetByHash( const u64 pathHash, AssetFile* assetFile )
{
    if ( !assetFile )
        return;

    ASSETTRACKER_SCOPE_TIMER((""));

    Windows::TakeSection critSection( *m_GeneralCriticalSection );

    SQL::StmtHandle stmt = m_SelectAssetByIDHandle;
    int sqlResult = m_DBManager->ExecStatement( stmt, (i64) pathHash );
    if ( sqlResult == SQLITE_DONE )
    {
        return;
    }
    else if ( sqlResult != SQLITE_ROW )
    {
        throw SQL::DBManagerException( m_DBManager, __FUNCTION__ );
    }
    else if ( sqlResult == SQLITE_ROW )
    {
        int index = -1;

        m_DBManager->GetColumnI64( stmt, ++index, (i64&) assetFile->m_RowID );

        tuid foundID = TUID::Null;
        m_DBManager->GetColumnI64( stmt, ++index, (i64&) foundID );

        std::string path;
        m_DBManager->GetColumnText( stmt, ++index, path );
        File::Reference fileRef( path );
        assetFile->SetFileReference( fileRef );

        m_DBManager->GetColumnI64( stmt, ++index, (i64&) assetFile->m_Size );

        i32 blah = 0;
        m_DBManager->GetColumnInt( stmt, ++index, (i32&) blah );

        std::string gah;
        m_DBManager->GetColumnText( stmt, ++index, gah );

        std::string bluh;
        m_DBManager->GetColumnText( stmt, ++index, bluh );

        m_DBManager->ResetStatement( stmt );
    }
}

/////////////////////////////////////////////////////////////////////////////
void CacheDB::GetAssetDependencies( File::ReferencePtr& fileRef, File::S_Reference& dependencies, bool reverse, u32 maxDepth, u32 currDepth, bool* cancel )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    Windows::TakeSection critSection( *m_GeneralCriticalSection );

    Nocturnal::Insert< File::S_Reference >::Result inserted = dependencies.insert( fileRef );
    if ( !inserted.second )
    {
        // circular dependancy?!
        return;
    }

    // are we done?
    if ( maxDepth != 0 && maxDepth <= currDepth )
        return;

    if ( CheckCancelQuery( cancel ) )
        return;

    SQL::StmtHandle selectHandle;
    if ( !reverse )
    {
        selectHandle = m_SelectDependenciesByIDHandle;
    }
    else
    {
        selectHandle = m_SelectUsagesByIDHandle;
    }

    /*
    S_tuid selectedIDs;
    u64 dep = 0;
    int execResult = m_DBManager->ExecStatement( selectHandle, (i64) fileRef->GetHash() );
    if ( execResult == SQLITE_ROW )
    {
        while( execResult == SQLITE_ROW )
        {
            if ( CheckCancelQuery( cancel ) )
                break;

            m_DBManager->GetColumnI64( selectHandle, 0, ( i64& ) dep );
            selectedIDs.insert( dep );

            execResult = m_DBManager->StepStatement( selectHandle );
        }

        m_DBManager->ResetStatement( selectHandle );
    }
    else if ( execResult != SQLITE_DONE && execResult != SQLITE_OK )
    {
        throw SQL::DBManagerException( m_DBManager, __FUNCTION__, "Failed to execute SQL" );
    }

    if ( CheckCancelQuery( cancel ) )
        return;

    for( File::S_Reference::const_iterator itr = selectedIDs.begin(), end = selectedIDs.end(); itr != end; ++itr )
    {
        if ( CheckCancelQuery( cancel ) )
            break;

        // recurse
        GetAssetDependencies( (*itr), dependencies, reverse, maxDepth, currDepth + 1, cancel );
    }
    */
}

///////////////////////////////////////////////////////////////////////////////
void CacheDB::GetDependencyGraph( File::ReferencePtr& fileRef, M_AssetFiles* assetFiles, bool reverse, u32 maxDepth, u32 currDepth, bool* cancel )
{
    ASSETTRACKER_SCOPE_TIMER((""));

    if ( CheckCancelQuery( cancel ) )
        return;

    /*
    Nocturnal::Insert<M_AssetFiles>::Result inserted = assetFiles->insert( M_AssetFiles::value_type( id, AssetFile::FindAssetFile( id, false ) ) );
    if ( !inserted.second )
    {
        // circular dependancy?!
        return;
    }

    AssetFile* assetFile = inserted.first->second;
    if ( !assetFile )
    {
        return;
    }

    assetFile->m_Dependencies.clear();
    GetAssetDependencies( assetFile->GetFileReference()->GetHash(), assetFile->m_Dependencies, reverse, 1, 0, cancel );

    // are we done?
    if ( maxDepth != 0 && maxDepth <= currDepth )
        return;

    if ( CheckCancelQuery( cancel ) )
        return;

    // recurse
    for ( S_tuid::const_iterator itr = assetFile->m_Dependencies.begin(), end = assetFile->m_Dependencies.end(); itr != end; ++itr )
    {
        if ( CheckCancelQuery( cancel ) )
            break;

        GetDependencyGraph( (*itr), assetFiles, reverse, maxDepth, currDepth + 1, cancel );
    }  
    */
}