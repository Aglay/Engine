#include "Precompile.h"
#include "ColorPickerWidget.h"

#include <wx/panel.h>

HELIUM_DEFINE_CLASS( Helium::Editor::ColorPickerWidget );

using namespace Helium;
using namespace Helium::Editor;

ColorPickerWindow::ColorPickerWindow( wxWindow* parent, ColorPickerWidget* colorPickerWidget )
: wxPanel( parent )
, m_ColorPickerWidget( colorPickerWidget )
, m_Override( false )
{
    m_ColorPicker = new ColorPicker( this, wxID_ANY );
    m_ColorPicker->EnableAutoSaveCustomColors();

    SetSizer( new wxBoxSizer( wxHORIZONTAL ) );
    wxSizer* sizer = GetSizer();
    sizer->Add( m_ColorPicker );

    Connect( wxID_ANY, wxEVT_COMMAND_COLOURPICKER_CHANGED, wxCommandEventHandler( ColorPickerWindow::OnChanged ) );
}

void ColorPickerWindow::OnChanged( wxCommandEvent& )
{
    m_ColorPickerWidget->GetControl()->Write();
}

ColorPickerWidget::ColorPickerWidget( Inspect::ColorPicker* colorPicker )
: m_ColorPickerControl( colorPicker )
, m_ColorPickerWindow( NULL )
{
    SetControl( colorPicker );
}

void ColorPickerWidget::CreateWindow( wxWindow* parent )
{
    HELIUM_ASSERT( !m_ColorPickerWindow );

    // allocate window and connect common listeners
    SetWindow( m_ColorPickerWindow = new ColorPickerWindow( parent, this ) );

    // init layout metrics
    //wxSize size( m_Control->GetCanvas()->GetDefaultSize( SingleAxes::X ), m_Control->GetCanvas()->GetDefaultSize( SingleAxes::Y ) );
    //m_ColorPickerWindow->SetSize( size );
    //m_ColorPickerWindow->SetMinSize( size );

    // add listeners
    m_ColorPickerControl->a_Highlight.Changed().AddMethod( this, &ColorPickerWidget::HighlightChanged );

    // update state of attributes that are not refreshed during Read()
    m_ColorPickerControl->a_Highlight.RaiseChanged();
}

void ColorPickerWidget::DestroyWindow()
{
    HELIUM_ASSERT( m_ColorPickerWindow );

    SetWindow( NULL );

    // remove listeners
    m_ColorPickerControl->a_Highlight.Changed().RemoveMethod( this, &ColorPickerWidget::HighlightChanged );

    // destroy window
    m_ColorPickerWindow->Destroy();
    m_ColorPickerWindow = NULL;
}

void ColorPickerWidget::Read()
{
    HELIUM_ASSERT( m_ColorPickerControl->IsBound() );

#if INSPECT_REFACTOR
    std::string str;
    m_ColorPickerControl->ReadStringData( str );
    std::stringstream stream( str );

    if ( m_ColorPickerControl->a_Alpha.Get() )
    {
        Color4 color4;
        stream >> color4;
        m_ColorPickerControl->a_Color4.Set( color4 );
        // Update the UI
        m_ColorPickerWindow->SetColor( m_ColorPickerControl->a_Color4.Get() );
    }
    else
    {
        Color3 color3;
        stream >> color3;
        m_ColorPickerControl->a_Color3.Set( color3 );
        // Update the UI
        m_ColorPickerWindow->SetColor( m_ColorPickerControl->a_Color3.Get() );
    }
#endif
}

bool ColorPickerWidget::Write()
{
    HELIUM_ASSERT( m_ColorPickerControl->IsBound() );

	Color color;
    m_ColorPickerWindow->GetColor( color );
    m_ColorPickerControl->a_Color.Set( color );

	bool result = false;
#if INSPECT_REFACTOR
    std::stringstream stream;

    if ( m_ColorPickerControl->a_Alpha.Get() )
    {
        stream << m_ColorPickerControl->a_Color4.Get();
    }
    else
    {
        stream << m_ColorPickerControl->a_Color3.Get();
    }
    m_ColorPickerWindow->SetOverride( true );
    result = m_ColorPickerControl->WriteStringData( stream.str() );
    m_ColorPickerWindow->SetOverride( false );
#endif

    return result;
}

void ColorPickerWidget::IsReadOnlyChanged( const Attribute<bool>::ChangeArgs& args )
{
    if ( args.m_NewValue )
    {
        m_ColorPickerWindow->Enable();
    }
    else
    {
        m_ColorPickerWindow->Disable();
    }
}

void ColorPickerWidget::HighlightChanged( const Attribute< bool >::ChangeArgs& args )
{
    if ( args.m_NewValue )
    {
        m_ColorPickerWindow->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
        m_ColorPickerWindow->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW));
    }
    else
    {
        m_ColorPickerWindow->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
        m_ColorPickerWindow->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    }
}
