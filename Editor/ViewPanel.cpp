#include "EditorPch.h"

#include "Editor/App.h"
#include "Editor/EditorIDs.h"
#include "Editor/EditorGeneratedWrapper.h"

#include "EditorScene/CameraSettings.h"

#include "ViewPanel.h"
#include "ArtProvider.h"

#include <wx/tglbtn.h>

using namespace Helium;
using namespace Helium::Editor;

wxSize ViewPanel::DefaultIconSize( 16, 16 );

ViewPanel::ViewPanel( SettingsManager* settingsManager, wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style )
	: ViewPanelGenerated( parent, id, pos, size, style )
{
	// TODO: Remove this block of code if/when wxFormBuilder supports wxArtProvider
	{
		//Freeze();

		m_FrameOriginBitmap->SetArtID( ArtIDs::Editor::FrameOrigin );
		m_FrameSelectionBitmap->SetArtID( ArtIDs::Editor::FrameSelected );
		m_PreviousViewBitmap->SetArtID( ArtIDs::Editor::PreviousView );
		m_NextViewBitmap->SetArtID( ArtIDs::Editor::NextView );

		m_HighlightModeToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_HighlightModeToggleBitmap->SetArtID( ArtIDs::Editor::HighlightMode );

		m_OrbitViewToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_OrbitViewToggleBitmap->SetArtID( ArtIDs::Editor::PerspectiveCamera );

		m_FrontViewToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_FrontViewToggleBitmap->SetArtID( ArtIDs::Editor::FrontOrthoCamera );

		m_SideViewToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_SideViewToggleBitmap->SetArtID( ArtIDs::Editor::SideOrthoCamera );

		m_TopViewToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_TopViewToggleBitmap->SetArtID( ArtIDs::Editor::TopOrthoCamera );

		m_ShowAxesToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_ShowAxesToggleBitmap->SetArtID( ArtIDs::Editor::ShowAxes );

		m_ShowGridToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_ShowGridToggleBitmap->SetArtID( ArtIDs::Editor::ShowGrid );

		m_ShowBoundsToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_ShowBoundsToggleBitmap->SetArtID( ArtIDs::Editor::ShowBounds );

		m_ShowStatisticsToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_ShowStatisticsToggleBitmap->SetArtID( ArtIDs::Editor::ShowStatistics );

		m_FrustumCullingToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_FrustumCullingToggleBitmap->SetArtID( ArtIDs::Editor::FrustumCulling );

		m_BackfaceCullingToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_BackfaceCullingToggleBitmap->SetArtID( ArtIDs::Editor::BackfaceCulling );

		m_WireframeShadingToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_WireframeShadingToggleBitmap->SetArtID( ArtIDs::Editor::ShadingWireframe );

		m_MaterialShadingToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_MaterialShadingToggleBitmap->SetArtID( ArtIDs::Editor::ShadingMaterial );

		m_TextureShadingToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_TextureShadingToggleBitmap->SetArtID( ArtIDs::Editor::ShadingTexture );

		m_ColorModeSceneToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_ColorModeSceneToggleBitmap->SetArtID( ArtIDs::Editor::ColorModeScene );

		m_ColorModeLayerToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_ColorModeLayerToggleBitmap->SetArtID( ArtIDs::Editor::ColorModeLayer );

		m_ColorModeTypeToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_ColorModeTypeToggleBitmap->SetArtID( ArtIDs::Editor::ColorModeNodeType );

		m_ColorModeScaleToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_ColorModeScaleToggleBitmap->SetArtID( ArtIDs::Editor::ColorModeScale );

		m_ColorModeScaleGradientToggleButton->SetOptions( PanelButtonOptions::Toggle );
		m_ColorModeScaleGradientToggleBitmap->SetArtID( ArtIDs::Editor::ColorModeScaleGradient );

		m_ToolPanel->Layout();

		Layout();
		//Thaw();
	}

	m_FrameOriginButton->SetHelpText( "Frame the origin in the viewport." );
	m_FrameSelectionButton->SetHelpText( "Frame the selected item in the viewport." );

	m_PreviousViewButton->SetHelpText( "Switch to the previous camera view." );
	m_NextViewButton->SetHelpText( "Switch to the next camera view." );

	m_HighlightModeToggleButton->SetHelpText( "Toggle Highlight mode." );

	m_OrbitViewToggleButton->SetHelpText( "Use the orbit camera." );
	m_FrontViewToggleButton->SetHelpText( "Use the front camera." );
	m_SideViewToggleButton->SetHelpText( "Use the side camera." );
	m_TopViewToggleButton->SetHelpText( "Use the top camera." );

	m_ShowAxesToggleButton->SetHelpText( "Toggle drawing the axes in the viewport." );
	m_ShowGridToggleButton->SetHelpText( "Toggle drawing the grid in the viewport." );
	m_ShowBoundsToggleButton->SetHelpText( "Toggle drawing object bounds in the viewport." );
	m_ShowStatisticsToggleButton->SetHelpText( "Toggle showing statistics for the current scene." );

	m_FrustumCullingToggleButton->SetHelpText( "Toggle frustum culling." );
	m_BackfaceCullingToggleButton->SetHelpText( "Toggle backface culling." );

	m_WireframeShadingToggleButton->SetHelpText( "Toggle wireframe mode." );
	m_MaterialShadingToggleButton->SetHelpText( "Toggle material shading mode." );
	m_TextureShadingToggleButton->SetHelpText( "Toggle texture shading mode." );

	m_ColorModeSceneToggleButton->SetHelpText( "Toggle scene coloring mode." );
	m_ColorModeLayerToggleButton->SetHelpText( "Toggle layer coloring mode." );
	m_ColorModeTypeToggleButton->SetHelpText( "Toggle type coloring mode." );
	m_ColorModeScaleToggleButton->SetHelpText( "Toggle scale coloring mode." );
	m_ColorModeScaleGradientToggleButton->SetHelpText( "Toggle scale gradient coloring mode." );

	m_ViewCanvas = new Editor::ViewCanvas( settingsManager, m_ViewContainerPanel, -1, wxPoint(0,0), wxSize(150,250), wxNO_BORDER | wxWANTS_CHARS | wxEXPAND );
	m_ViewContainerPanel->GetSizer()->Add( m_ViewCanvas, 1, wxEXPAND | wxALL, 0 );

	RefreshButtonStates();

	//ViewColorMode colorMode = MainFramePreferences()->GetViewPreferences()->GetColorMode();
	//M_IDToColorMode::const_iterator colorModeItr = m_ColorModeLookup.begin();
	//M_IDToColorMode::const_iterator colorModeEnd = m_ColorModeLookup.end();
	//for ( ; colorModeItr != colorModeEnd; ++colorModeItr )
	//{
	//    m_ViewColorMenu->Check( colorModeItr->first, colorModeItr->second == colorMode );
	//}

	Connect( wxEVT_CHAR, wxKeyEventHandler( ViewPanel::OnChar ), NULL, this );

	m_WireframeShadingToggleButton->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnRenderMode ), NULL, this );
	m_MaterialShadingToggleButton->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnRenderMode ), NULL, this );
	m_TextureShadingToggleButton->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnRenderMode ), NULL, this );
	m_OrbitViewToggleButton->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnCamera ), NULL, this );
	m_FrontViewToggleButton->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnCamera ), NULL, this );
	m_SideViewToggleButton->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnCamera ), NULL, this );
	m_TopViewToggleButton->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnCamera ), NULL, this );
	m_FrameOriginButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnFrameOrigin ), NULL, this );
	m_FrameSelectionButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnFrameSelected ), NULL, this );
	m_HighlightModeToggleButton->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnToggleHighlightMode ), NULL, this );
	m_NextViewButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnNextView ), NULL, this );
	m_PreviousViewButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnPreviousView ), NULL, this );
	m_OrbitViewToggleButton->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnCamera ), NULL, this );
	m_FrontViewToggleButton->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnCamera ), NULL, this );
	m_SideViewToggleButton->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnCamera ), NULL, this );
	m_TopViewToggleButton->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnCamera ), NULL, this );
	m_ShowAxesToggleButton->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnToggleShowAxes ), NULL, this );
	m_ShowGridToggleButton->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( ViewPanel::OnToggleShowGrid ), NULL, this );

	Layout();
}

void ViewPanel::RefreshButtonStates()
{
	m_HighlightModeToggleButton->SetValue( m_ViewCanvas->GetViewport().IsHighlighting() );

	m_OrbitViewToggleButton->SetValue( m_ViewCanvas->GetViewport().GetCameraMode() == Editor::CameraMode::Orbit );
	m_FrontViewToggleButton->SetValue( m_ViewCanvas->GetViewport().GetCameraMode() == Editor::CameraMode::Front );
	m_SideViewToggleButton->SetValue( m_ViewCanvas->GetViewport().GetCameraMode() == Editor::CameraMode::Side );
	m_TopViewToggleButton->SetValue( m_ViewCanvas->GetViewport().GetCameraMode() == Editor::CameraMode::Top );

	m_ShowAxesToggleButton->SetValue( m_ViewCanvas->GetViewport().IsAxesVisible() );
	m_ShowGridToggleButton->SetValue( m_ViewCanvas->GetViewport().IsGridVisible() );
	m_ShowBoundsToggleButton->SetValue( m_ViewCanvas->GetViewport().IsBoundsVisible() );

	m_FrustumCullingToggleButton->SetValue( m_ViewCanvas->GetViewport().GetCamera()->IsViewFrustumCulling() );
	m_BackfaceCullingToggleButton->SetValue( m_ViewCanvas->GetViewport().GetCamera()->IsBackFaceCulling() );

	m_WireframeShadingToggleButton->SetValue( m_ViewCanvas->GetViewport().GetCamera()->GetShadingMode() == Editor::ShadingMode::Wireframe );
	m_MaterialShadingToggleButton->SetValue( m_ViewCanvas->GetViewport().GetCamera()->GetShadingMode() == Editor::ShadingMode::Material );
}

void ViewPanel::OnChar( wxKeyEvent& event )
{
	int keyCode = event.GetKeyCode();

	switch ( keyCode )
	{
	case WXK_SPACE:
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, ViewPanelEvents::NextView );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip( false );
			break;
		}

	case WXK_UP:
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, EventIds::ID_EditWalkUp);
			wxGetApp().GetFrame()->GetEventHandler()->ProcessEvent( evt );
			event.Skip(false);
			break;
		}

	case WXK_DOWN:
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, EventIds::ID_EditWalkDown);
			GetEventHandler()->ProcessEvent( evt );
			event.Skip(false);
			break;
		}

	case WXK_RIGHT:
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, EventIds::ID_EditWalkForward);
			GetEventHandler()->ProcessEvent( evt );
			event.Skip(false);
			break;
		}

	case WXK_LEFT:
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, EventIds::ID_EditWalkBackward);
			GetEventHandler()->ProcessEvent( evt );
			event.Skip(false);
			break;
		}

	case WXK_INSERT:
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, EventIds::ID_ToolsPivot);
			GetEventHandler()->ProcessEvent( evt );
			event.Skip(false);
			break;
		}

	case WXK_DELETE:
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, wxID_DELETE );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip(false);
			break;
		}

	case WXK_ESCAPE:
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip(false);
			break;
		}

		//
		// ASCII has some strange key codes for ctrl-<letter> combos
		//
		//01 |   1         Ctrl-a         SOH 
		//02 |   2         Ctrl-b         STX 
		//03 |   3         Ctrl-c         ETX 
		//04 |   4         Ctrl-d         EOT 
		//05 |   5         Ctrl-e         ENQ 
		//06 |   6         Ctrl-f         ACK 
		//07 |   7         Ctrl-g         BEL 
		//08 |   8         Ctrl-h         BS 
		//09 |   9  Tab    Ctrl-i         HT 
		//0A |  10         Ctrl-j         LF 
		//0B |  11         Ctrl-k         VT 
		//0C |  12         Ctrl-l         FF 
		//0D |  13  Enter  Ctrl-m         CR 
		//0E |  14         Ctrl-n         SO 
		//0F |  15         Ctrl-o         SI 
		//10 |  16         Ctrl-p         DLE 
		//11 |  17         Ctrl-q         DC1 
		//12 |  18         Ctrl-r         DC2 
		//13 |  19         Ctrl-s         DC3 
		//14 |  20         Ctrl-t         DC4 
		//15 |  21         Ctrl-u         NAK 
		//16 |  22         Ctrl-v         SYN 
		//17 |  23         Ctrl-w         ETB 
		//18 |  24         Ctrl-x         CAN 
		//19 |  25         Ctrl-y         EM 
		//1A |  26         Ctrl-z         SUB 
		//1B |  27  Esc    Ctrl-[         ESC 
		//1C |  28         Ctrl-\         FS 
		//1D |  29         Ctrl-]         GS 

	case 1: // ctrl-a
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, wxID_SELECTALL );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip( false );
			break;
		}

	case 22: // ctrl-v
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, wxID_PASTE );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip( false );
			break;
		}

	case 24: // ctrl-x
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, wxID_CUT );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip( false );
			break;
		}

	case wxT( '4' ):
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, ViewPanelEvents::Wireframe );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip( false );
			break;
		}

	case wxT( '5' ):
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, ViewPanelEvents::Material );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip( false );
			break;
		}

	case wxT( '6' ):
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, ViewPanelEvents::Texture );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip( false );
			break;
		}

	case wxT( '7' ):
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, ViewPanelEvents::OrbitCamera );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip( false );
			break;
		}

	case wxT( '8' ):
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, ViewPanelEvents::FrontCamera );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip( false );
			break;
		}

	case wxT( '9' ):
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, ViewPanelEvents::SideCamera );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip( false );
			break;
		}

	case wxT( '0' ):
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, ViewPanelEvents::TopCamera );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip( false );
			break;
		}

	case wxT( 'o' ):
	case wxT( 'O' ):
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, ViewPanelEvents::FrameOrigin );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip( false );
			break;
		}

	case wxT( 'f' ):
	case wxT( 'F' ):
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, ViewPanelEvents::FrameSelected );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip( false );
			break;
		}

	case wxT( 'h' ):
	case wxT( 'H' ):
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, ViewPanelEvents::ToggleHighlightMode );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip( false );
			break;
		}

	case wxT( ']' ):
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, ViewPanelEvents::NextView );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip( false );
			break;
		}

	case wxT( '[' ):
		{
			wxCommandEvent evt ( wxEVT_COMMAND_MENU_SELECTED, ViewPanelEvents::PreviousView );
			GetEventHandler()->ProcessEvent( evt );
			event.Skip( false );
			break;
		}

	default:
		event.Skip();
		event.ResumePropagation( wxEVENT_PROPAGATE_MAX );
		break;
	}
}

void ViewPanel::OnRenderMode( wxCommandEvent& event )
{
	switch ( event.GetId() )
	{
	case ViewPanelEvents::Wireframe:
		{
			m_ViewCanvas->GetViewport().GetCamera()->SetShadingMode( ShadingMode::Wireframe );
			Refresh();
			event.Skip( false );
			break;
		}

	case ViewPanelEvents::Material:
		{
			m_ViewCanvas->GetViewport().GetCamera()->SetShadingMode( ShadingMode::Material );
			Refresh();
			event.Skip( false );
			break;
		}

	case ViewPanelEvents::Texture:
		{
			m_ViewCanvas->GetViewport().GetCamera()->SetShadingMode( ShadingMode::Texture );
			Refresh();
			event.Skip( false );
			break;
		}

	default:
		event.Skip();
		event.ResumePropagation( wxEVENT_PROPAGATE_MAX );
		break;
	}
}

void ViewPanel::OnCamera( wxCommandEvent& event )
{
	int eventId = event.GetId();
	if ( eventId == ViewPanelEvents::OrbitCamera || eventId == m_OrbitViewToggleButton->GetId() )
	{
		m_ViewCanvas->GetViewport().SetCameraMode( CameraMode::Orbit );
		Refresh();
		event.Skip( false );
	}
	else if ( eventId == ViewPanelEvents::FrontCamera || eventId == m_FrontViewToggleButton->GetId() )
	{
		m_ViewCanvas->GetViewport().SetCameraMode( CameraMode::Front );
		Refresh();
		event.Skip( false );
	}
	else if ( eventId == ViewPanelEvents::SideCamera || eventId == m_SideViewToggleButton->GetId() )
	{
		m_ViewCanvas->GetViewport().SetCameraMode( CameraMode::Side );
		Refresh();
		event.Skip( false );
	}
	else if ( eventId == ViewPanelEvents::TopCamera || eventId == m_TopViewToggleButton->GetId() )
	{
		m_ViewCanvas->GetViewport().SetCameraMode( CameraMode::Top );
		Refresh();
		event.Skip( false );
	}
	else
	{
		event.Skip();
		event.ResumePropagation( wxEVENT_PROPAGATE_MAX );
	}

	RefreshButtonStates();
}

void ViewPanel::OnFrameOrigin( wxCommandEvent& event )
{
	m_ViewCanvas->GetViewport().GetCamera()->Reset();
	Refresh();
	event.Skip( false );
}

void ViewPanel::OnFrameSelected( wxCommandEvent& event )
{
	if ( !wxGetApp().GetFrame()->GetSceneManager().HasCurrentScene() )
	{
		event.Skip();
		event.ResumePropagation( wxEVENT_PROPAGATE_MAX );
		return;
	}

	wxGetApp().GetFrame()->GetSceneManager().GetCurrentScene()->FrameSelected();
	Refresh();
	event.Skip( false );
}

void ViewPanel::OnToggleHighlightMode( wxCommandEvent& event )
{
	m_ViewCanvas->GetViewport().SetHighlighting( !m_ViewCanvas->GetViewport().IsHighlighting() );
	m_HighlightModeToggleButton->SetValue( m_ViewCanvas->GetViewport().IsHighlighting() );
	Refresh();
	event.Skip( false );
}

void ViewPanel::OnToggleShowAxes( wxCommandEvent& event )
{
	m_ViewCanvas->GetViewport().SetAxesVisible( !m_ViewCanvas->GetViewport().IsAxesVisible() );
	m_ShowAxesToggleButton->SetValue( m_ViewCanvas->GetViewport().IsAxesVisible() );
	Refresh();
	event.Skip( false );
}

void ViewPanel::OnToggleShowGrid( wxCommandEvent& event )
{
	m_ViewCanvas->GetViewport().SetGridVisible( !m_ViewCanvas->GetViewport().IsGridVisible() );
	m_ShowGridToggleButton->SetValue( m_ViewCanvas->GetViewport().IsGridVisible() );
	Refresh();
	event.Skip( false );
}

void ViewPanel::OnNextView( wxCommandEvent& event )
{
	m_ViewCanvas->GetViewport().NextCameraMode();
	Refresh();
	RefreshButtonStates();
}

void ViewPanel::OnPreviousView( wxCommandEvent& event )
{
	m_ViewCanvas->GetViewport().PreviousCameraMode();
	Refresh();
	RefreshButtonStates();
}
