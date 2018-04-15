#include "Precompile.h"
#include "ColorPicker.h"
#include "Editor/CustomColors.h"
#include "Editor/SimpleConfig.h"

using namespace Helium;
using namespace Helium::Editor;

ColorPicker::ColorPicker( wxWindow* parent, wxWindowID id, const wxColour& col, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name )
: wxColourPickerCtrl( parent, id, col, pos, size, style, validator, name )
, m_AutoSaveCustomColors( false )
{
#if !HELIUM_OS_LINUX
    wxGenericColourButton* picker = wxDynamicCast( GetPickerCtrl(), wxGenericColourButton );
    if ( picker )
    {
        picker->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ColorPicker::OnButtonClick ), NULL, this );
    }
#else
    HELIUM_ASSERT( false ); // see what GetPickerCtrl() returns
#endif
}

ColorPicker::~ColorPicker()
{
#if !HELIUM_OS_LINUX
    wxGenericColourButton* picker = wxDynamicCast( GetPickerCtrl(), wxGenericColourButton );
    if ( picker )
    {
        picker->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ColorPicker::OnButtonClick ), NULL, this );
    }
#else
    HELIUM_ASSERT( false ); // see what GetPickerCtrl() returns
#endif
}

///////////////////////////////////////////////////////////////////////////////
// When this control is clicked on, a color picker dialog is shown.  The user
// can store custom colors on this dialog.  This function returns a string 
// representation of the custom colors that can later be loaded back.
// 
bool ColorPicker::SaveCustomColors( std::string& colors )
{
    bool isOk = false;
#if !HELIUM_OS_LINUX
    wxGenericColourButton* picker = wxDynamicCast( GetPickerCtrl(), wxGenericColourButton );
    if ( picker )
    {
        wxColourData* data = picker->GetColourData();
        if ( data )
        {
            colors = CustomColors::Save( *data );
            isOk = true;
        }
    }
#else
    HELIUM_ASSERT( false ); // see what GetPickerCtrl() returns
#endif
    return isOk;
}

///////////////////////////////////////////////////////////////////////////////
// Loads custom colors previously created by the SaveCustomColors function.
// 
bool ColorPicker::LoadCustomColors( const std::string& colors )
{
    bool isOk = false;
#if !HELIUM_OS_LINUX
    wxGenericColourButton* picker = wxDynamicCast( GetPickerCtrl(), wxGenericColourButton );
    if ( picker )
    {
        wxColourData* data = picker->GetColourData();
        if ( data )
        {
            CustomColors::Load( *data, colors );
            isOk = true;
        }
    }
#else
    HELIUM_ASSERT( false ); // see what GetPickerCtrl() returns
#endif
    return isOk;
}

///////////////////////////////////////////////////////////////////////////////
// If enable is true, custom colors will be automatically saved to the specfied
// registry location/key any time the user updates them.
// 
void ColorPicker::EnableAutoSaveCustomColors( bool enable, const std::string& key, const std::string& registryLocation )
{
    m_AutoSaveCustomColors = enable;

    if ( m_AutoSaveCustomColors )
    {
        if ( key.empty() )
        {
            m_Key = CustomColors::GetConfigKey();
        }
        else
        {
            m_Key = key;
        }

        m_RegistryLocation = registryLocation;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Disables the auto-save feature for custom colors.
// 
void ColorPicker::DisableAutoSaveCustomColors()
{
    EnableAutoSaveCustomColors( false );
}

///////////////////////////////////////////////////////////////////////////////
// Handle clicking on this button.  Overridden for auto-saving of custom colors.
// 
void ColorPicker::OnButtonClick( wxCommandEvent& args )
{
#if !HELIUM_OS_LINUX
    wxGenericColourButton* picker = wxDynamicCast( GetPickerCtrl(), wxGenericColourButton );
    if ( picker )
    {
        std::string colors;
        if ( m_AutoSaveCustomColors )
        {
            // Load custom colors from registry
            if ( SimpleConfig::GetInstance()->Read( m_RegistryLocation, m_Key, colors ) )
            {
                LoadCustomColors( colors );
            }
        }

        picker->OnButtonClick( args );

        if ( m_AutoSaveCustomColors )
        {
            if ( SaveCustomColors( colors ) )
            {
                // Save custom colors to registry
                SimpleConfig::GetInstance()->Write( m_RegistryLocation, m_Key, colors );
            }
        }
    }
#else
    HELIUM_ASSERT( false ); // see what GetPickerCtrl() returns
#endif
}
