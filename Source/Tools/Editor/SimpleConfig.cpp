#include "Precompile.h"
#include "SimpleConfig.h"
#include <wx/config.h>

#include "Platform/Assert.h"

using namespace Helium;
using namespace Helium::Editor;

///////////////////////////////////////////////////////////////////////////////
// Private Constructor
// This is a singleton class.  Use GetInstance to access the one and only instance.
// 
SimpleConfig::SimpleConfig()
{
    m_Config = wxConfig::Create();
    m_Config->SetPath( "Helium" );
}

///////////////////////////////////////////////////////////////////////////////
// Returns a pointer to the one and only instance of this class.
// 
SimpleConfig* SimpleConfig::GetInstance()
{
    static SimpleConfig theRegistryConfig;
    return &theRegistryConfig;
}

///////////////////////////////////////////////////////////////////////////////
// Reads a value from the registry.  You can specify a relative path and the
// actual key to read.  The value parameter will be populated with valid data
// if this function returns true.
// 
// The relativePath parameter should be separated by forward-slashes, but not 
// start with one.  Example: Foo/Bar
// 
bool SimpleConfig::Read( const std::string& relativePath, const std::string& key, std::string& value )
{
    bool wasRead = false;

    wxString oldPath = m_Config->GetPath();

    if ( !relativePath.empty() )
    {
        m_Config->SetPath( relativePath.c_str() );
    }

    wxString wxVal;
    if ( m_Config->Read( key.c_str(), &wxVal ) )
    {
        value = wxVal.c_str();
        wasRead = true;
    }
    m_Config->SetPath( oldPath );

    return wasRead;
}

///////////////////////////////////////////////////////////////////////////////
// Writes the specified value to the registry into the specified key, which
// can be in a group as indicated by relativePath.  Returns true if the value
// is successfully written to the registry.  
// 
// The relativePath parameter should be separated by forward-slashes, but not 
// start with one.  Example: Foo/Bar
// 
bool SimpleConfig::Write( const std::string& relativePath, const std::string& key, const std::string& value )
{
    wxString oldPath = m_Config->GetPath();

    if ( !relativePath.empty() )
    {
        m_Config->SetPath( relativePath.c_str() );
    }

    bool wasWritten = m_Config->Write( key.c_str(), value.c_str() );
    m_Config->SetPath( oldPath );

    return wasWritten;
}
