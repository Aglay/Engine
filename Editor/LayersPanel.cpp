#include "Precompile.h"

#include "LayersPanel.h"
#include "ArtProvider.h"
#include "EditorIDs.h"

#include "Editor/Controls/EditorButton.h"
#include "Editor/Controls/DynamicBitmap.h"
#include "EditorScene/DependencyCommand.h"

using namespace Helium;
using namespace Helium::Editor;

LayersPanel::NameChangeInfo::NameChangeInfo()
	: m_Layer( NULL )
{
}

LayersPanel::NameChangeInfo::~NameChangeInfo()
{
}

///////////////////////////////////////////////////////////////////////////////
// Clears out internal member data to original state.
// 
void LayersPanel::NameChangeInfo::Clear()
{
	m_Layer = NULL;
	m_OldName.clear();
}

LayersPanel::LayersPanel( SceneManager* manager, wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
	: LayersPanelGenerated( parent, id, pos, size, style )
	, m_SceneManager( manager )
	, m_Grid( new Grid( this, EventIds::ID_LayerGrid, true ) )
	, m_Scene( NULL )
{
	SetHelpText( "This is the Layers Panel, you can control how layers are set up in your project here." );

	// TODO: Remove this block of code if/when wxFormBuilder supports wxArtProvider
	{
		Freeze();

		m_NewLayerBitmap->SetArtID( ArtIDs::Editor::CreateNewLayer );
		m_NewLayerFromSelectionBitmap->SetArtID( ArtIDs::Editor::CreateNewLayerFromSelection );
		m_DeleteLayersBitmap->SetArtID( ArtIDs::Editor::DeleteSelectedLayers );

		m_AddToLayerBitmap->SetArtID( ArtIDs::Editor::AddSelectionToLayers );
		m_RemoveFromLayerBitmap->SetArtID( ArtIDs::Editor::RemoveSelectionFromLayers );

		m_SelectMembersBitmap->SetArtID( ArtIDs::Editor::SelectLayerMembers );

		m_LayerManagementPanel->Layout();

		Layout();
		Thaw();
	}

	m_NewLayerButton->SetHelpText( "Creates a new layer in the scene." );
	m_NewLayerFromSelectionButton->SetHelpText( "Creates a new layer in the scene and adds the selection to it." );
	m_DeleteLayersButton->SetHelpText( "Deletes the selected layer from the scene." );
	m_AddToLayerButton->SetHelpText( "Adds the currently selected items to the layer." );
	m_RemoveFromLayerButton->SetHelpText( "Removes the currently selected items from the layer." );
	m_SelectMembersButton->SetHelpText( "Selects all items that are part of the selected layer(s)." );

	m_LayerManagementPanel->SetHelpText( "This is the layer toolbar." );
	m_Grid->GetPanel()->SetHelpText( "This is the layer grid, you can select a layer to manipulate here." );

	m_NewLayerButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LayersPanel::OnNewLayer ), NULL, this );
	m_NewLayerFromSelectionButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LayersPanel::OnNewLayerFromSelection ), NULL, this );
	m_DeleteLayersButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LayersPanel::OnDeleteLayer ), NULL, this );
	m_AddToLayerButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LayersPanel::OnAddSelectionToLayer ), NULL, this );
	m_RemoveFromLayerButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LayersPanel::OnRemoveSelectionFromLayer ), NULL, this );
	m_SelectMembersButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LayersPanel::OnSelectLayerMembers ), NULL, this );

	// Make sure the toolbar buttons start out disabled
	UpdateToolBarButtons();

	GetSizer()->Add( m_Grid->GetPanel(), 1, wxEXPAND );

	// Listeners that are not dependent on the current scene
	if ( m_SceneManager )
	{
		m_SceneManager->e_CurrentSceneChanging.AddMethod( this, &LayersPanel::CurrentSceneChanging );
		m_SceneManager->e_CurrentSceneChanged.AddMethod( this, &LayersPanel::CurrentSceneChanged );
	}
	m_Grid->AddRowVisibilityChangedListener( GridRowChangeSignature::Delegate( this, &LayersPanel::LayerVisibleChanged ) );
	m_Grid->AddRowSelectabilityChangedListener( GridRowChangeSignature::Delegate( this, &LayersPanel::LayerSelectableChanged ) );
	m_Grid->AddRowRenamedListener( GridRowRenamedSignature::Delegate( this, &LayersPanel::RowRenamed ) );
}

LayersPanel::~LayersPanel()
{
	m_NewLayerButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LayersPanel::OnNewLayer ), NULL, this );
	m_NewLayerFromSelectionButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LayersPanel::OnNewLayerFromSelection ), NULL, this );
	m_DeleteLayersButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LayersPanel::OnDeleteLayer ), NULL, this );
	m_AddToLayerButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LayersPanel::OnAddSelectionToLayer ), NULL, this );
	m_RemoveFromLayerButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LayersPanel::OnRemoveSelectionFromLayer ), NULL, this );
	m_SelectMembersButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LayersPanel::OnSelectLayerMembers ), NULL, this );

	// Remove listeners that are not dependent on the current scene
	if ( m_SceneManager )
	{
		m_SceneManager->e_CurrentSceneChanging.RemoveMethod( this, &LayersPanel::CurrentSceneChanging );
		m_SceneManager->e_CurrentSceneChanged.RemoveMethod( this, &LayersPanel::CurrentSceneChanged );
	}
	m_Grid->RemoveRowVisibilityChangedListener( GridRowChangeSignature::Delegate  ( this, &LayersPanel::LayerVisibleChanged ) );
	m_Grid->RemoveRowSelectabilityChangedListener( GridRowChangeSignature::Delegate  ( this, &LayersPanel::LayerSelectableChanged ) );
	m_Grid->RemoveRowRenamedListener( GridRowRenamedSignature::Delegate ( this, &LayersPanel::RowRenamed ) );

	DisconnectSceneListeners();

	delete m_Grid;
}

///////////////////////////////////////////////////////////////////////////////
// Adds a layer to the list managed by this control  The layer will show up
// in the grid, and its selectability and visibility will be able to be 
// changed.
// 
bool LayersPanel::AddLayer( Layer* layer )
{
	std::pair< M_LayerDumbPtr::const_iterator, bool > inserted = m_Layers.insert( M_LayerDumbPtr::value_type( layer->GetName(), layer ) );
	HELIUM_ASSERT( inserted.second );

	// Listen for name changes to this layer
	layer->AddNameChangingListener( SceneNodeChangeSignature::Delegate ( this, &LayersPanel::NameChanging ) );
	layer->AddNameChangedListener( SceneNodeChangeSignature::Delegate ( this, &LayersPanel::NameChanged ) );

	bool  result = m_Grid->AddRow( layer->GetName(), layer->IsVisible(), layer->IsSelectable() );

	// Select the newly added row
	if(result)
	{
		m_Grid->SelectRow(m_Grid->GetRowNumber(layer->GetName()), false);
		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
// Removes the specified layer from the list managed by this control.
// 
bool LayersPanel::RemoveLayer( Layer* layer )
{
	bool foundLayer = m_Layers.erase( layer->GetName() ) > 0;
	HELIUM_ASSERT( foundLayer );

	// We are no longer tracking this layer, so don't listen for name changes any more
	layer->RemoveNameChangingListener( SceneNodeChangeSignature::Delegate ( this, &LayersPanel::NameChanging ) );
	layer->RemoveNameChangedListener( SceneNodeChangeSignature::Delegate ( this, &LayersPanel::NameChanged ) );
	return m_Grid->RemoveRow( layer->GetName() );

	return false;
}

///////////////////////////////////////////////////////////////////////////////
// Removes layers with one or zero elements
// 
void LayersPanel::OnCleanUpLayers(wxCommandEvent& event)
{
	CleanUpLayers();
}

///////////////////////////////////////////////////////////////////////////////
// Increments the internal batch count.  Every call to BeginBatch() should be
// accompanied by a call to EndBatch().  When the batch count reaches zero,
// the grid will redraw.  Use this function to halt UI updates to the grid
// when making a lot of changes all at once.
// 
void LayersPanel::BeginBatch()
{
	m_Grid->BeginBatch();
}

///////////////////////////////////////////////////////////////////////////////
// Decrements teh internal batch count.  When the batch count reaches zero, the
// grid will redraw.  See BeginBatch() for more information.
// 
void LayersPanel::EndBatch()
{
	m_Grid->EndBatch();
}

///////////////////////////////////////////////////////////////////////////////
// Connects this object to all the events that we want to listen for.
// 
void LayersPanel::ConnectSceneListeners()
{
	if ( m_Scene )
	{
		// Add listeners for when layers are added/removed from a the scene
		m_Scene->e_NodeAdded.Add( NodeChangeSignature::Delegate ( this, &LayersPanel::NodeAdded ) );
		m_Scene->e_NodeRemoved.Add( NodeChangeSignature::Delegate ( this, &LayersPanel::NodeRemoved ) );

		// Listen for changes to the scene's selection
		m_Scene->AddSelectionChangedListener( SelectionChangedSignature::Delegate ( this, &LayersPanel::SelectionChanged ) );
	}
}

///////////////////////////////////////////////////////////////////////////////
// Disconnects this object from all the listeners that we added ourselves to.
// 
void LayersPanel::DisconnectSceneListeners()
{
	if ( m_Scene )
	{
		// Remove layer creation listeners on the scene
		m_Scene->e_NodeAdded.Remove( NodeChangeSignature::Delegate ( this, &LayersPanel::NodeAdded ) );
		m_Scene->e_NodeRemoved.Remove( NodeChangeSignature::Delegate ( this, &LayersPanel::NodeRemoved ) );

		// Remove selection change listener on scene
		m_Scene->RemoveSelectionChangedListener( SelectionChangedSignature::Delegate ( this, &LayersPanel::SelectionChanged ) );
	}
}

///////////////////////////////////////////////////////////////////////////////
// Removes all the layers currently managed by this control.
// 
void LayersPanel::RemoveAllLayers()
{
	// Remove name change listener on each layer
	M_LayerDumbPtr::const_iterator layerItr = m_Layers.begin();
	M_LayerDumbPtr::const_iterator layerEnd = m_Layers.end();
	for ( ; layerItr != layerEnd; ++layerItr )
	{
		Layer* layer = layerItr->second;
		layer->RemoveNameChangingListener( SceneNodeChangeSignature::Delegate ( this, &LayersPanel::NameChanging ) );
		layer->RemoveNameChangedListener( SceneNodeChangeSignature::Delegate ( this, &LayersPanel::NameChanged ) );
	}

	m_Layers.clear();
	m_Grid->RemoveAllRows();
}

///////////////////////////////////////////////////////////////////////////////
// Enables/disables the toolbar buttons.
// 
void LayersPanel::UpdateToolBarButtons()
{
	// Enable/disable everything in the toolbar based on whether or not we have
	// a valid scene.
	if ( m_Scene )
	{
		m_LayerManagementPanel->Enable();
	}
	else
	{
		m_LayerManagementPanel->Disable();
	}

	m_LayerManagementPanel->Refresh();
}

///////////////////////////////////////////////////////////////////////////////
// Adds undoable commands to m_Scene's undo queue which will do one of two thing:
// 
// Either...
// 1. Adds the currently selected scene items to the currently highlighted
// layers.  Set addToLayer to true for this option.
// or...
// 2. Removes the currently selected scene items from the currently highlighted
// layers.  Set addToLayer to false for this option.
//
// If there are no selected items, or no selected layers, nothing happens.  It 
// is also safe to remove items from a layer even if they are not part of the
// layer (nothing happens), or add items to a layer that already belong to
// that layer (again, nothing happens).
// 
void LayersPanel::LayerSelectedItems( bool addToLayer )
{
	HELIUM_ASSERT( m_Scene );

	// Decide whether we are adding the selected scene items to the highlighted
	// layers, or removing the items from the layers.
	const DependencyCommand::DependencyAction action = addToLayer ? DependencyCommand::Connect : DependencyCommand::Disconnect;

	// If there are selected nodes in the scene, and selected rows in this control...
	const OS_ObjectDumbPtr& selectedNodes = m_Scene->GetSelection().GetItems();
	std::set< uint32_t > selectedRows = m_Grid->GetSelectedRows();
	if ( selectedNodes.Size() > 0 && selectedRows.size() > 0 )
	{
		//Log::Debug( "LayerSelectedItems\n" );
		BatchUndoCommandPtr batch = new BatchUndoCommand ();

		OS_ObjectDumbPtr::Iterator nodeItr = selectedNodes.Begin();
		OS_ObjectDumbPtr::Iterator nodeEnd = selectedNodes.End();
		std::set< uint32_t >::const_iterator rowItr;
		const std::set< uint32_t >::const_iterator rowEnd = selectedRows.end();
		const M_LayerDumbPtr::const_iterator layerEnd = m_Layers.end();

		// For each node in the scene's selection list...
		for ( ; nodeItr != nodeEnd; ++nodeItr )
		{
			// ensure that we are not trying to add one layer to another layer
			{
				Layer* layerTest = Reflect::SafeCast< Layer >( *nodeItr );
				if( layerTest )
				{
					continue;
				}
			}

			//Check the current selection
			if( *nodeItr == NULL )
			{
				//Invalid or incompatible
				continue;
			}

			SceneNode* node = Reflect::SafeCast< SceneNode >( *nodeItr );
			if ( node )
			{
				// For each row that is highlighted...
				for ( rowItr = selectedRows.begin(); rowItr != rowEnd; ++rowItr )
				{
					// Find the layer that goes with the highlighted row
					M_LayerDumbPtr::iterator layerItr = m_Layers.find( m_Grid->GetRowName( *rowItr ) );
					if ( layerItr != layerEnd )
					{
						// Check to see if the node is already in the current layer...
						Layer* layer = layerItr->second;
						S_SceneNodeSmartPtr::const_iterator foundDescendant = layer->GetDescendants().find( node );

						// If the node is already in this layer, and we are suppose to be adding the node to the layer,
						// just skip the command (doCommand = false).  If the node is not in this layer, and we are
						// suppose to be removing the node from the layer, skip the command as well.  Otherwise, go 
						// ahead and carry out the command (doCommand = true).
						const bool doCommand = addToLayer ? ( foundDescendant == layer->GetDescendants().end() ) : ( foundDescendant != layer->GetDescendants().end() );
						if ( doCommand )
						{
							// Finally make an undoable command to add/remove the node to/from the layer
							batch->Push( new DependencyCommand( action, layer, node ) );
							//Log::Debug( "\t\t%s node %s %s layer %s [row=%d]\n", addToLayer ? "Added" : "Removed", node->GetName().c_str(), addToLayer ? "to" : "from", layer->GetName().c_str(), *rowItr );
						}
						else
						{
							//Log::Debug( "\t\tNode %s was already a member of layer %s [row=%d]\n", node->GetName().c_str(), layer->GetName().c_str(), *rowItr );
						}
					}
					else
					{
						// Something is wrong.  The rows that are selected in the grid do not correspond to
						// items in our list of layers (m_Layers).  Somehow those lists got out of sync.
						Log::Error( "Unable to add selection to layer [row=%d] because it doesn't exist\n", *rowItr );
						HELIUM_BREAK();
					}
				}
			}
		}

		//Log::Debug( "\n" );
		if( !batch->IsEmpty() )
		{
			m_Scene->Push( batch );
			m_Scene->Execute( false );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// This is a handy function for use while debugging.  It prints the currently
// selected layers to the output window.
// 
void LayersPanel::DebugDumpSelection()
{
#ifdef _DEBUG
	Log::Debug( "Dumping grid selection.\n" );
	std::set< uint32_t > selection = m_Grid->GetSelectedRows();
	const size_t numSelected = selection.size();
	if ( numSelected == 0 )
	{
		Log::Debug( "\tNo items are selected.\n" );
	}
	else
	{
		Log::Debug( "\t%d item%s selected.\n", numSelected, ( numSelected == 1 ) ? "" : "s" );
		std::set< uint32_t >::const_iterator rowItr = selection.begin();
		std::set< uint32_t >::const_iterator rowEnd = selection.end();
		for ( ; rowItr != rowEnd; ++rowItr )
		{
			const std::string& name = m_Grid->GetRowName( *rowItr );
			Log::Debug( "\t\t%s\n", name.c_str() );
		}
	}
	Log::Debug( "\n" );
#endif
}

///////////////////////////////////////////////////////////////////////////////
// Called when the "New Layer" button is clicked on the toolbar.  Creates an
// un-doable command that adds a layer to the scene (thereby adding it to this
// control).
// 
void LayersPanel::OnNewLayer( wxCommandEvent& event )
{
	if ( m_Scene )
	{
		LayerPtr layer = new Layer();
		layer->SetOwner( m_Scene );
		layer->Initialize();
		m_Scene->Push( new SceneNodeExistenceCommand( ExistenceActions::Add, m_Scene, layer ) );
		m_Scene->Execute( false ); 
	}
}

///////////////////////////////////////////////////////////////////////////////
// Called when the "New Layer From Selection" button is clicked on the toolbar.
// 
void LayersPanel::OnNewLayerFromSelection( wxCommandEvent& dummyEvt )
{
	if ( m_Scene )
	{    
		if(IsSelectionValid() == false)
		{
			return;
		}

		BatchUndoCommandPtr batch = new BatchUndoCommand ();
		LayerPtr layer = new Layer();
		layer->SetOwner( m_Scene );
		layer->Initialize();

		// Generate a name for this layer
		GenerateLayerName(layer);

		batch->Push( new SceneNodeExistenceCommand( ExistenceActions::Add, m_Scene, layer ) );

		// Step 2: add all the selected items to the layer
		const OS_ObjectDumbPtr& selection = m_Scene->GetSelection().GetItems();
		OS_ObjectDumbPtr::Iterator itr = selection.Begin();
		OS_ObjectDumbPtr::Iterator end = selection.End();
		for ( ; itr != end; ++itr )
		{
			//If the element is a supported type
			if( *itr )
			{
				SceneNode* node = Reflect::SafeCast< SceneNode >( *itr );
				if ( node )
				{
					batch->Push( new DependencyCommand( DependencyCommand::Connect, layer, node ) );
				}
			}
		}

		m_Scene->Push( batch );
		m_Scene->Execute( false );
	}
}

///////////////////////////////////////////////////////////////////////////////
// Called when the "Delete Layer" button is clicked on the toolbar.  If any
// layers are selected in this control, they are deleted from the scene (and
// thereby deleted from this control as well).  This adds an undoable operation
// to the scene's undo queue.
// 
void LayersPanel::OnDeleteLayer( wxCommandEvent& event_ )
{
	DeleteSelectedLayers();
}

///////////////////////////////////////////////////////////////////////////////
//
void LayersPanel::DeleteSelectedLayers()
{
	// If anything selected in the grid
	if ( m_Scene && m_Grid->IsAnythingSelected() )
	{
		LayerSelectedItems( false );

		// Begin undo batch
		BatchUndoCommandPtr batch = new BatchUndoCommand ();

		// Get an ordered list of the selected rows, and traverse the list in reverse order.
		// This makes sure that removing an item doesn't change the row number of another
		// item that will be removed later in the loop.  If we don't do this, we run the
		// risk of invalidating the selection array as we iterate over it.
		const std::set< uint32_t >& selection = m_Grid->GetSelectedRows();
		std::set< uint32_t >::const_reverse_iterator rowItr = selection.rbegin();
		std::set< uint32_t >::const_reverse_iterator rowEnd = selection.rend();
		for ( ; rowItr != rowEnd; ++rowItr )
		{
			M_LayerDumbPtr::iterator layerItr = m_Layers.find( m_Grid->GetRowName( *rowItr ) );
			HELIUM_ASSERT( layerItr != m_Layers.end() );
			// NOTE: m_Layers is changing as we iterate over the selection (items are being 
			// removed via callbacks), so don't hold on to any iterators that point into the list.  
			// Recalculate m_Layers.end() each time through the loop.
			if ( layerItr != m_Layers.end() )
			{
				Layer* layer = layerItr->second;

				// If the layer that we are about to delete is in the scene's selection list,
				// we had better just clear out the selection list (otherwise the attribute
				// editor will still be showing a layer that is no longer in the scene).  This
				// has to be done before actually deleting the layer.
				if ( m_Scene->GetSelection().Contains( layer ) )
				{
					batch->Push( m_Scene->GetSelection().Clear() );
				}

				// If the layer has any members, we should remove them before removing the layer,
				// otherwise if those members are deleted, they will be pointing to an invalid
				// layer that will eventually be Disconnected.
				S_SceneNodeSmartPtr descendents = layer->GetDescendants();
				for ( S_SceneNodeSmartPtr::iterator itr = descendents.begin(), end = descendents.end(); itr != end; ++itr )
				{
					batch->Push( new DependencyCommand( DependencyCommand::Disconnect, layer, *itr ) );
				}

				// Push the command to delete the layer
				batch->Push( new SceneNodeExistenceCommand( ExistenceActions::Remove, m_Scene, layer ) );
			}
		}

		// End undo batch
		m_Scene->Push( batch );

		m_Scene->Execute( false );
	}
}

///////////////////////////////////////////////////////////////////////////////
// Called when the "Add Selection To Layer" button is clicked on the toolbar.
// Adds all of the currently selected scene nodes to each of the currently
// highlighted layers.  This operation adds an undoable command to the
// scene's undo queue.
// 
void LayersPanel::OnAddSelectionToLayer( wxCommandEvent& event )
{
	if ( m_Scene )
	{
		LayerSelectedItems( true );
	}
}

///////////////////////////////////////////////////////////////////////////////
// Called when the "Remove Selection From Layer" button is clicked on the toolbar.
// Removes all the currently selected scene nodes from the currently hightlighted
// layers (if there is any of either).  This operation adds an undoable command
// to the scene's undo queue.
// 
void LayersPanel::OnRemoveSelectionFromLayer( wxCommandEvent& event )
{
	if ( m_Scene )
	{
		LayerSelectedItems( false );
	}
}

///////////////////////////////////////////////////////////////////////////////
// Called when the "Select Layer Members" button is clicked on the toolbar.
// 
void LayersPanel::OnSelectLayerMembers( wxCommandEvent& event )
{
	if ( m_Scene )
	{
		OS_SceneNodeDumbPtr newSelection;
		M_LayerDumbPtr::const_iterator layerItr = m_Layers.begin();
		M_LayerDumbPtr::const_iterator layerEnd = m_Layers.end();
		for ( ; layerItr != layerEnd; ++layerItr )
		{
			Layer* layer = layerItr->second;
			if ( m_Grid->IsSelected( layer->GetName() ) )
			{
				S_SceneNodeSmartPtr::const_iterator dependItr = layer->GetDescendants().begin();
				S_SceneNodeSmartPtr::const_iterator dependEnd = layer->GetDescendants().end();
				for ( ; dependItr != dependEnd; ++dependItr )
				{
					newSelection.Append( *dependItr );
				}
			}
		}

		m_Scene->Push( m_Scene->GetSelection().SetItems( newSelection ) );
		m_Scene->Execute( false );
	}
}

///////////////////////////////////////////////////////////////////////////////
// Called when the "Select Layer" button is clicked on the toolbar.  Goes 
// through all the layers that are currently selected in this control and
// adds them to the scene's selection list (implicity this is an undoable
// operation).
// 
void LayersPanel::OnSelectLayer( wxCommandEvent& event )
{
	if ( m_Scene )
	{
		OS_SceneNodeDumbPtr newSelection;

		const std::set< uint32_t >& selection = m_Grid->GetSelectedRows();
		std::set< uint32_t >::const_iterator rowItr = selection.begin();
		std::set< uint32_t >::const_iterator rowEnd = selection.end();
		for ( ; rowItr != rowEnd; ++rowItr )
		{
			M_LayerDumbPtr::iterator layerItr = m_Layers.find( m_Grid->GetRowName( *rowItr ) );
			HELIUM_ASSERT( layerItr != m_Layers.end() );
			Layer* layer = layerItr->second;
			newSelection.Append( layer );
		}

		m_Scene->Push( m_Scene->GetSelection().SetItems( newSelection ) );
		m_Scene->Execute( false );
	}
}

///////////////////////////////////////////////////////////////////////////////
// Callback for when the scene's selection changes (as opposed to the selection
// within the layer grid).  Checks the selection to see if any of our layers
// are contained within.  If any of the layers managed by this grid are within
// the selection, the grid is updated to make sure that the appropriate rows 
// are selected.  This mainly facilitates undo/redo of selection so that this
// control shows the same layers as selected as other controls in the scene.
// 
void LayersPanel::SelectionChanged( const SelectionChangeArgs& args )
{
	if ( args.m_Selection.Size() > 0 )
	{
		uint32_t numLayersInSelection = 0;
		OS_ObjectDumbPtr::Iterator itr = args.m_Selection.Begin();
		OS_ObjectDumbPtr::Iterator end = args.m_Selection.End();
		for ( ; itr != end; ++itr )
		{
			Layer* lunaLayer = Reflect::SafeCast< Layer >( *itr );
			if ( lunaLayer )
			{
				HELIUM_ASSERT( m_Layers.find( lunaLayer->GetName() ) != m_Layers.end() );

				++numLayersInSelection;
				// If this is the first layer that we found in the selection list...
				if ( numLayersInSelection == 1 )
				{
					// Clear the grid's selection as soon as we find a layer contained
					// in the selection list.  The grid's selection will be rebuilt as
					// we continue to iterate over the selection list.
					m_Grid->DeselectAllRows();
				}

				int32_t row = m_Grid->GetRowNumber( lunaLayer->GetName() );
				HELIUM_ASSERT( row >= 0 );
				if ( row >= 0 )
				{
					m_Grid->SelectRow( row, true );
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Callback just before a layer's name is changed.  Stores the layer and the
// current name of the layer for processing in NameChanged.
// 
void LayersPanel::NameChanging( const SceneNodeChangeArgs& args )
{
	M_LayerDumbPtr::const_iterator layerItr = m_Layers.find( args.m_Node->GetName() );
	if ( layerItr != m_Layers.end() )
	{
		Layer* layer = layerItr->second;
		const std::string& name = layerItr->first;
		if ( args.m_Node != layer || layer->GetName() != name )
		{
			Log::Error( "Layer in list (named %s), does not match layer named %s.\n", name.c_str(), args.m_Node->GetName().c_str() );
			HELIUM_BREAK();
		}
		m_NameChangeInfo.m_Layer = layer;
		m_NameChangeInfo.m_OldName = name;
	}
	else
	{
		Log::Error( "Layer named %s is not in the grid.\n", args.m_Node->GetName().c_str() );
		HELIUM_BREAK();
	}
}

///////////////////////////////////////////////////////////////////////////////
// Callback for when a layer's name changes.  Uses the old name of the layer
// that we stashed to earlier to update our layer list and the row label in 
// the grid.
// 
void LayersPanel::NameChanged( const SceneNodeChangeArgs& args )
{
	const std::string& oldName = m_NameChangeInfo.m_OldName;
	M_LayerDumbPtr::iterator layerItr = m_Layers.find( oldName );
	if ( layerItr != m_Layers.end() )
	{
		Layer* layer = layerItr->second;
		const std::string& newName = args.m_Node->GetName();
		m_Layers.erase( layerItr );
		m_Layers.insert( M_LayerDumbPtr::value_type( newName, layer ) );

		bool nameUpdated = m_Grid->SetRowName( oldName, newName );
		HELIUM_ASSERT( nameUpdated );
	}
	else
	{
		Log::Error( "Layer named %s is not in the grid.\n", m_NameChangeInfo.m_OldName.c_str() );
		HELIUM_BREAK();
	}

	m_NameChangeInfo.Clear();
}

///////////////////////////////////////////////////////////////////////////////
// Callback for when a row's visibility checkbox is changed in the grid.  
// Changes the visibility member of the layer to match the row that was changed.
// 
void LayersPanel::LayerVisibleChanged( const GridRowChangeArgs& args )
{
	const std::string& name = m_Grid->GetRowName( args.m_RowNumber );
	M_LayerDumbPtr::const_iterator layerItr = m_Layers.find( name );
	if ( layerItr != m_Layers.end() )
	{
		Layer* layer = layerItr->second;
		layer->SetVisible( m_Grid->IsRowVisibleChecked( args.m_RowNumber ) );
		layer->GetOwner()->Execute( false );
	}
	else
	{
		Log::Error( "LayerVisibleChanged - layer named %s not found\n", name.c_str() );
		HELIUM_BREAK();
	}
}

///////////////////////////////////////////////////////////////////////////////
// Callback for when a row's selectability checkbox is changed in the grid.
// Changes the selectability member of the layer to match the row that was
// changed.
// 
void LayersPanel::LayerSelectableChanged( const GridRowChangeArgs& args )
{
	const std::string& name = m_Grid->GetRowName( args.m_RowNumber );
	M_LayerDumbPtr::const_iterator layerItr = m_Layers.find( name );
	if ( layerItr != m_Layers.end() )
	{
		Layer* layer = layerItr->second;
		bool selectable = m_Grid->IsRowSelectableChecked( args.m_RowNumber );

		layer->SetSelectable( selectable );

		if (!selectable)
		{
			OS_ObjectDumbPtr newSelection;

			OS_ObjectDumbPtr selection = layer->GetOwner()->GetSelection().GetItems();
			OS_ObjectDumbPtr::Iterator itr = selection.Begin();
			OS_ObjectDumbPtr::Iterator end = selection.End();
			for ( ; itr != end; ++itr )
			{
				SceneNode* node = Reflect::SafeCast<SceneNode>( *itr );

				if (!node || !layer->ContainsMember( node ))
				{
					newSelection.Append(*itr);
				}
			}

			if (newSelection.Size() != selection.Size())
			{
				layer->GetOwner()->GetSelection().SetItems( newSelection );
			}
		}

		layer->GetOwner()->Execute( false );
	}
	else
	{
		Log::Error( "LayerSelectableChanged - layer named %s not found\n", name.c_str() );
		HELIUM_BREAK();
	}
}

///////////////////////////////////////////////////////////////////////////////
// Callback for when a row is renamed by the user through the layer grid UI.
// Renames the corresponding layer node in the scene.
// 
void LayersPanel::RowRenamed( const GridRowRenamedArgs& args )
{
	M_LayerDumbPtr::iterator found = m_Layers.find( args.m_OldName );
	if ( found != m_Layers.end() )
	{
		Layer* layer = found->second;
		layer->GetOwner()->Push( new PropertyUndoCommand< std::string >( new Helium::MemberProperty< Layer, std::string >( layer, &Layer::GetName, &Layer::SetGivenName ), args.m_NewName ) );
	}
}

///////////////////////////////////////////////////////////////////////////////
// Callback for when the current scene is about to change.  Clears out all the
// layers and disconnect all the scene listeners.
// 
void LayersPanel::CurrentSceneChanging( const SceneChangeArgs& args )
{
	if ( args.m_Scene == m_Scene )
	{
		DisconnectSceneListeners();
		RemoveAllLayers();
		m_Scene = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Callback for when the current scene has been changed.  Connects the listeners
// to the scene and updates the UI elements.  The control is not automatically
// populated with the current scene's layers, so that falls to the owner of 
// this control.  However, any additionally added layers will automatically
// show up in the UI.
// 
void LayersPanel::CurrentSceneChanged( const SceneChangeArgs& args )
{
	if ( args.m_Scene != m_Scene )
	{
		m_Scene = args.m_Scene;
		UpdateToolBarButtons();
		ConnectSceneListeners();
	}
}

///////////////////////////////////////////////////////////////////////////////
// Callback for when a layer has been added to the scene.  Adds a layer to
// the UI.
// 
void LayersPanel::NodeAdded( const NodeChangeArgs& args )
{
	Layer* layer = Reflect::SafeCast< Layer >( args.m_Node );
	if ( layer )
	{
		AddLayer( layer );
	}
}

///////////////////////////////////////////////////////////////////////////////
// Callback for when a layer has been removed from the scene.  Removes the
// corresponding layer from the UI.
// 
void LayersPanel::NodeRemoved( const NodeChangeArgs& args )
{
	Layer* layer = Reflect::SafeCast< Layer >( args.m_Node );
	if ( layer )
	{
		RemoveLayer( layer );
	}
}
