#pragma once

#include "Application/API.h"
#include "SQLite.h"
#include "Application/SQL/SQLExceptions.h"

#include "Platform/Types.h"
#include "Foundation/File/Path.h"
#include "Foundation/Memory/SmartPtr.h"

#include "Platform/Mutex.h"

namespace SQL
{ 
  // Forwards
  class SQLite;

  class APPLICATION_API SQLiteDB : public Nocturnal::RefCountBase< SQLiteDB >
  {
  public:
    SQLiteDB( const char* friendlyName );
    virtual ~SQLiteDB();

    virtual bool Open( const std::string& dbFilename,
      const std::string& configFolder,
      const std::string& version = std::string( "" ),
      int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE );

    virtual void Close();

    virtual bool Update( const std::string& dbVersion );
    virtual void Recreate();

    //
    // Transactions
    //

    virtual void BeginTrans();
    virtual void CommitTrans();
    virtual void RollbackTrans();
    virtual bool IsTransOpen();

  protected:
    virtual bool SelectDBVersion( std::string& version );
    bool SelectDBVersion( const std::string& sql, std::string& version );  

    virtual bool Load();
    virtual bool IsOutOfDate();   
    virtual void PrepareStatements() = 0;
    
    virtual void Delete();

    void UpdateDBFileVersionTable();

  protected:
    SQLite*              m_DBManager;

    Nocturnal::Path      m_DBFile;
    Nocturnal::Path      m_ConfigDir;
    Nocturnal::Path      m_DataFile;

    std::string          m_DBVersion;
    int                  m_OpenFlags;

	Platform::Mutex		 m_Mutex;

    StmtHandle           m_StmtHandleSelectDBVersion;
  };

} // namespace SQLite