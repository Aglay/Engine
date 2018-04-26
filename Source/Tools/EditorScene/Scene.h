#pragma once

#include "Application/DocumentManager.h"
#include "Foundation/Event.h"
#include "Foundation/TUID.h"
#include "Application/UndoQueue.h"

#include "Persist/Archive.h"

#include "Inspect/DataBinding.h"
#include "Inspect/Canvas.h"

#include "EditorScene/API.h"
#include "EditorScene/Selection.h"
#include "EditorScene/PropertiesGenerator.h"

#include "Framework/World.h"
#include "Framework/SceneDefinition.h"

#include "Pick.h"
#include "Tool.h"
#include "SceneNode.h"
#include "Graph.h"
#include "Transform.h"

#include <set>
#include <map>
#include <vector>

namespace Helium
{
	namespace Editor
	{
		class PickVisitor;
		struct SceneChangeArgs;
	}

	namespace Editor
	{
		typedef Helium::Signature< const struct UndoCommandArgs& > UndoCommandSignature;
		typedef Helium::Signature< const struct ResolveSceneArgs& > ResolveSceneSignature;
		typedef Helium::Signature< const struct ReleaseSceneArgs& > ReleaseSceneSignature;
		typedef Helium::Signature< const struct SceneEditingArgs& > SceneEditingSignature;
		typedef Helium::Signature< const struct SceneStatusChangeArgs& > SceneStatusChangeSignature;
		typedef Helium::Signature< const struct SceneContextChangeArgs& > SceneContextChangedSignature;
		typedef Helium::Signature< const struct NodeChangeArgs& > NodeChangeSignature;
		typedef Helium::Signature< const struct LoadArgs& > LoadSignature;
		typedef Helium::Signature< const struct ExecuteArgs& > ExecuteSignature;

		class NameCompare
		{
		public:
			bool operator( )( const std::string& str1, const std::string& str2 ) const
			{
				return CaseInsensitiveCompareString(str1.c_str(), str2.c_str()) < 0;
			}
		};

		typedef std::map< std::string, SceneNode*, NameCompare > M_NameToSceneNodeDumbPtr;

		class HELIUM_EDITOR_SCENE_API Scene : public Reflect::Object
		{
		public:
			/// Nested enum. Signifies which runtime construct this Scene corresponds to.
			class SceneTypes
			{
			public:
				enum SceneType
				{
					World,
					Slice,
					//  Prefab,
				};
			};
			typedef SceneTypes::SceneType SceneType;

			/// Nested enum. Differentiate between replacing the entire contents of a scene and appending new nodes.
			class ImportActions
			{
			public:
				enum ImportAction
				{
					Load,   //!< Scene must be empty, we are bringing in all the nodes.
					Import, //!< Leave whatever nodes are currently in the Scene and bring in new ones.
				};
			};
			typedef ImportActions::ImportAction ImportAction;

			/// Nested enum.
			class ImportFlags
			{
			public:
				enum ImportFlag
				{
					None = 0,

					Merge   = 1 << 1, //!< If a node matches one being imported, replace node in this scene with the new one.
					Select  = 1 << 2, //!< Select the imported nodes so the user can easily identify them.

					Default = None,
				};

				static bool HasFlag( uint32_t flags, ImportFlag singleFlag )
				{
					return ( flags & singleFlag ) == singleFlag;
				}
			};
			typedef ImportFlags::ImportFlag ImportFlag;

			/// Nested enum.
			class ExportFlags
			{
			public:
				enum ExportFlag
				{
					None = 0,

					SelectedNodes         = 1 << 1, //!< Only export nodes that are selected
					MaintainHierarchy     = 1 << 2, //!< Include Parents and children
					MaintainDependencies  = 1 << 3, //!< Include layers, etc...

					Default = MaintainHierarchy | MaintainDependencies,
				};

				static bool HasFlag( uint32_t flags, ExportFlag singleFlag )
				{
					return ( flags & singleFlag ) == singleFlag;
				}
			};
			typedef ExportFlags::ExportFlag ExportFlag;

			/// Argument object for the Export* family of functions.
			struct ExportArgs
			{
				typedef std::vector< AlignedBox > V_AlignedBox;

				uint32_t m_Flags;
				V_AlignedBox m_Bounds;

				ExportArgs( uint32_t flags = ExportFlags::Default )
					: m_Flags ( flags )
				{
				}
			};

		public:
			/// @name Construction/Destruction
			//@{
			Scene( Viewport* viewport, SceneDefinition& scene, SceneType type );
			~Scene();
			//@}

			Helium::TUID GetId() const { return m_Id; }

			/// Is this the current scene in the editor?
			bool IsFocused() { return m_IsFocused; }
			void SetFocused( bool focused ) { m_IsFocused = focused; }

			/// Is this scene able to be edited?
			bool IsEditable();

			/// SceneDefinition that this scene is currently editing.
			const Helium::SceneDefinition& GetSceneDefinition() const { return *m_SceneDefinition; }
			void SetSceneDefinition( Helium::SceneDefinition& definition ) { m_SceneDefinition = &definition; }

			/// Get the current tool in use in this scene.
			const ToolPtr& GetTool();
			void SetTool(const ToolPtr& tool);

			// support for zone color
			const Color& GetColor() const;
			void SetColor( const Color& color );

			SceneType GetType() const { return m_Type; }

			SceneDefinition* GetDefinition() const { return m_Definition; }

			Helium::World* GetWorld() const { return m_World; }
			Helium::Slice* GetSlice() const { return m_Slice; }

			/// @name Selection
			/// These are are PER-SCENE, so they can be utilized by objects in the scene or tools.
			//@{
			/// Some code wants to know if there is anything selected during undo/redo, use this.
			bool HasSelection() const { return !m_Selection.GetItems().Empty(); }
			bool HasHighlighted() const { return !m_Highlighted.Empty(); }

			/// Allows external people to modify selection.
			Selection& GetSelection() { return m_Selection; }
			const OS_SceneNodeDumbPtr& GetHighlighted() const { return m_Highlighted; }

			/// Add/remove access to the selection changing event (callable while the undo queue is active).
			void AddSelectionChangingListener( const SelectionChangingSignature::Delegate& listener ) const { m_Selection.AddChangingListener( listener ); }
			void RemoveSelectionChangingListener( const SelectionChangingSignature::Delegate& listener ) const { m_Selection.RemoveChangingListener( listener ); }

			/// Add/remove access to the selection change event (callable while the undo queue is active).
			void AddSelectionChangedListener( const SelectionChangedSignature::Delegate& listener ) const { m_Selection.AddChangedListener( listener ); }
			void RemoveSelectionChangedListener( const SelectionChangedSignature::Delegate& listener ) const { m_Selection.RemoveChangedListener( listener ); }
			//@}


			/// @name Database
			/// These comprise the most basic data structures that hold references to the objects in the scene.
			//@{
			/// The scene root node (synthetic).
			const TransformPtr& GetRoot() const { return m_Root; }

			/// The dependency graph (for ordered and efficient execution).
			const SceneGraphPtr& GetGraph() const { return m_Graph; }

			/// The nodes in the scene.
			const M_SceneNodeSmartPtr& GetNodes() const { return m_Nodes; }

			SceneNode* Find( const std::string& name ) const; 

			SceneNode* Get( const Helium::TUID& uid ) const
			{
				M_SceneNodeSmartPtr::const_iterator it = m_Nodes.find( uid );

				if ( it != m_Nodes.end() )
				{
					return it->second;
				}

				return NULL;
			}

			template< class T >
			T* Get( const Helium::TUID &uid ) const
			{
				return Reflect::SafeCast< T >( Get( uid ) );
			}
			//@}

			/// @name GUI Elements
			/// Sometimes direct references to GUI are necessary (like the 3D view).
			//@{
			/// The 3D view to use for drawing this scene.
			Viewport* GetViewport() const { return m_View; }
			//@}

			/// @name Document
			//@{
			//void ConnectDocument( Document* document );
			//void DisconnectDocument( const Document* document );
			//@}

			/// @name Loading
			//@{
			/// Open a whole scene, replacing the current one.
			bool Load( Helium::SceneDefinition& path );
			bool Reload();

			// Import data into this scene, possibly merging with existing nodes.
			UndoCommandPtr Import( const Helium::FilePath& path, ImportAction action = ImportActions::Import, uint32_t importFlags = ImportFlags::None, HierarchyNode* parent = NULL, const Reflect::MetaClass* importReflectType = NULL );
			UndoCommandPtr ImportJson( const std::string& json, uint32_t importFlags = ImportFlags::None, HierarchyNode* parent = NULL );
			UndoCommandPtr ImportSceneNodes( std::vector< Reflect::ObjectPtr >& elements, ImportAction action, uint32_t importFlags, const Reflect::MetaClass* importReflectType = NULL );
			//@}

			/// @name Saving
			//@{
			/// For status and error reporting from Reflect::Archive events.
			void ArchiveStatus( const Persist::ArchiveStatus& info );

			/// Saves this scene to its current file location. (Get and change the scene editor file to switch the destination.)
			//bool Serialize();

			// Save nodes to a file or to an xml string buffer.  Do not change the file
			// that this scene is pointing at.  Optionally export the entire scene or
			// just selected nodes.  Optionally maintain hierarchy or dependencies.
			bool Export( const FilePath& path, const ExportArgs& args );
			bool ExportJson( std::string& json, const ExportArgs& args );
			bool Export( std::vector< Reflect::ObjectPtr >& elements, const ExportArgs& args, BatchUndoCommand* changes );
			//@}

			/// @name Naming
			/// Each object in Editor has a unique name, these interfaces ensure that.
			/// We use unique names because a lot of our combo boxes need to refer to truly unique objects.
			//@{
			/// Entry point for other objects to request their name to be changed.
			void Rename( SceneNode* sceneNode, const std::string& newName, std::string oldName = "" );
			//@}

			/// @name Object Management
			/// Add/remove objects during load and save, create and delete, as well as undo and redo
			//@{
			/// Insert a node into the scene
			void AddObject( SceneNodePtr node );
			void RemoveObject( SceneNodePtr node );
			//@}

			/// @name Evaluation, Rendering, and Picking
			/// Update object states, render viewports, and handle viewport intersection and selection.
			//@{
			/// Pump evaluation and refresh view.
			void Execute( bool interactively );

			// allocate/cleanup resources
			void Create();
			void Delete();

			// core render and pick visitation entry points
			void Render( RenderVisitor* render );
			bool Pick( PickVisitor* pick ) const;

			// selection and highlight setup
			void Select( const SelectArgs& args );
			void SetHighlight( const SetHighlightArgs& args );
			void ClearHighlight( const ClearHighlightArgs& args );
			//@}

			/// @name Undo/Redo Support
			//@{
			bool Push( const UndoCommandPtr& command );
			//@}

			/// @name Query Interfaces
			/// Simple queries that find objects, and interfaces to advanced find/selection.
			//@{
			// pick will inject the guid of the object selected next into this data
			void PickLink( const Inspect::PickLinkArgs& args );

			// select will cause a selection to occur for the object referenced by the key
			void SelectLink( const Inspect::SelectLinkArgs& args );

			// populate will populate all the items in a scene that match the specified type
			void PopulateLink( Inspect::PopulateLinkArgs& args );

			// find/search for an object by different criteria
			SceneNode* FindNode( const Helium::TUID& id );
			SceneNode* FindNode( const std::string& name );

			// raise event
			void ChangeStatus( const std::string& status );
			void RefreshSelection();

			// callbacks when important events occur
			void PropertyChanging( const Inspect::ControlChangingArgs& args );
			void PropertyChanged( const Inspect::ControlChangedArgs& args );
			void SelectionChanging( const SelectionChangingArgs& args );
			void SelectionChanged( const SelectionChangeArgs& args );
			void CurrentSceneChanging( const SceneChangeArgs& args );
			void CurrentSceneChanged( const SceneChangeArgs& args );
			//@}

			/// @name Change and Scene Management
			/// Additional routines that perform work mandated by the UI should go here (at the end of the cpp file)
			//@{
			HierarchyNode* GetCommonParent( const V_HierarchyNodeDumbPtr& nodes );
			void GetCommonParents( const V_HierarchyNodeDumbPtr& nodes, V_HierarchyNodeDumbPtr& parents );
			void GetSelectionParents( OS_ObjectDumbPtr& parents );

			void GetFlattenedSelection( OS_ObjectDumbPtr& selection );
			void GetFlattenedHierarchy( HierarchyNode* node, OS_HierarchyNodeDumbPtr& items );

			void GetSelectedTransforms( V_Matrix4& transforms );
			UndoCommandPtr SetSelectedTransforms( const V_Matrix4& transforms );

			UndoCommandPtr SetHiddenSelected( bool hidden );
			UndoCommandPtr SetHiddenUnrelated( bool hidden );
			UndoCommandPtr ShowLastHidden();
			UndoCommandPtr SelectSimilar();
			UndoCommandPtr DeleteSelected();
			UndoCommandPtr ParentSelected();
			UndoCommandPtr UnparentSelected();
			UndoCommandPtr GroupSelected();
			UndoCommandPtr UngroupSelected();
			UndoCommandPtr CenterSelected();
			UndoCommandPtr DuplicateSelected();
			UndoCommandPtr SmartDuplicateSelected();
			UndoCommandPtr SnapSelectedToCamera();
			UndoCommandPtr SnapCameraToSelected();

			void FrameSelected();

			void MeasureDistance();

			UndoCommandPtr PickWalkUp();
			UndoCommandPtr PickWalkDown();
			UndoCommandPtr PickWalkSibling(bool forward);
			//@}

		protected:
			// Object Management helpers
			void AddSceneNode( const SceneNodePtr& node );
			void RemoveSceneNode( const SceneNodePtr& node );

			// Undo/Redo Support helpers
			void UndoingOrRedoing( const UndoQueueChangingArgs& args );
			void UndoQueueCommandPushed( const UndoQueueChangeArgs& args );

		private:
			// Document helpers
			/// Callback for when a document is saved.
			//void OnDocumentSave( const DocumentEventArgs& args );

			// Loading helpers
			void Reset();

			UndoCommandPtr ImportSceneNode( const Reflect::ObjectPtr& element, V_SceneNodeSmartPtr& createdNodes, ImportAction action, uint32_t importFlags, const Reflect::MetaClass* importReflectType = NULL  );

			/// @brief If this node has been remapped from another node, return the source nodes ID
			/// When we copy elements, we give them a new UniqueID. If we need information related
			/// to the original node, we need a way to gather the id of the original node.
			/// @param nodeId the id of the copied node
			/// @return returns the id of the source node if it exists. If not it returns Helium::TUID::Null
			Helium::TUID GetRemappedID( tuid nodeId );

			// Saving helpers
			void ExportSceneNode( SceneNode* node, std::vector< Reflect::ObjectPtr >& elements, std::set<TUID>& exported, const ExportArgs& args, BatchUndoCommand* changes );
			void ExportHierarchyNode( HierarchyNode* node, std::vector< Reflect::ObjectPtr >& elements, std::set<TUID>& exported, const ExportArgs& args, BatchUndoCommand* changes, bool exportChildren = true );

			// Naming helpers
			/// Split the number portion of the name out.
			int Split( std::string& outName );

			/// Fully validating setter for a node's name.
			void SetName( SceneNode* sceneNode, const std::string& newName );

			// Evaluation, Rendering, and Picking helpers
			/// Evaluate dependency graph.
			void Evaluate( bool silent = false );

			// Change and Scene Management helpers
			void ViewPreferencesChanged( const Reflect::ObjectChangeArgs& args );

			/// @name Events
			//@{
		public:
			mutable DocumentObjectChangedSignature::Event e_HasChanged;

			UndoCommandSignature::Delegate d_UndoCommand;
			ResolveSceneSignature::Delegate d_ResolveScene;
			ReleaseSceneSignature::Delegate d_ReleaseScene;
			SceneEditingSignature::Delegate d_Editing;
			SceneStatusChangeSignature::Event e_StatusChanged;
			SceneContextChangedSignature::Event e_SceneContextChanged;
			NodeChangeSignature::Event e_NodeAdded;
			NodeChangeSignature::Event e_NodeRemoved;
			NodeChangeSignature::Event e_NodeRemoving;
			LoadSignature::Event e_LoadStarted;
			LoadSignature::Event e_LoadFinished;
			ExecuteSignature::Event e_Executed;
			//@}

		private:
			SceneType m_Type;
			SceneDefinitionPtr m_Definition;
			WorldPtr m_World;
			SlicePtr m_Slice;

			Helium::SceneDefinitionPtr m_SceneDefinition;
			Helium::TUID m_Id;

			// load
			int32_t m_Progress;
			std::map< TUID, TUID > m_RemappedIDs;
			HierarchyNode* m_ImportRoot;
			bool m_Importing;

			// scene data
			TransformPtr m_Root;

			// gives us ordered evaluation
			SceneGraphPtr m_Graph;

			// container for nodes sorted by uid
			M_SceneNodeSmartPtr m_Nodes;

			// container for nodes sorted by name
			M_NameToSceneNodeDumbPtr m_Names;

			// selection of this scene
			Selection m_Selection;

			// highlighted items of this scene
			OS_SceneNodeDumbPtr m_Highlighted;

			// data for handling picks
			Inspect::DataBindingPtr m_PickData;

			// the 3d view control
			Viewport* m_View;

			// the tool in use by this scene
			ToolPtr m_Tool;

			// offset matrix for smart duplicate
			Matrix4 m_SmartDuplicateMatrix;

			// flag that the smart duplicate matrix is valid
			bool m_ValidSmartDuplicateMatrix;

			// the set of last hidden
			std::set<Helium::TUID> m_LastHidden;

			// set by the zone that this scene belongs to, and used for
			// the 3D view's "color modes"
			Color m_Color;

			bool m_IsFocused;
		};

		typedef Helium::StrongPtr< Scene > ScenePtr;
		typedef std::set< ScenePtr > S_SceneSmartPtr;
		typedef std::map< std::string, ScenePtr > M_SceneSmartPtr;
		typedef std::map< Scene*, int32_t > M_AllocScene;

		/// Command for adding and removing nodes from a scene.
		class SceneNodeExistenceCommand : public ExistenceUndoCommand< SceneNodePtr >
		{
		public:
			SceneNodeExistenceCommand( ExistenceAction action, Scene* scene, const SceneNodePtr& node, bool redo = true )
				: ExistenceUndoCommand( action, node,
				Delegate< SceneNodePtr >( scene, &Scene::AddObject ),
				Delegate< SceneNodePtr >( scene, &Scene::RemoveObject ),
				redo )
			{
			}
		};

		/// Command for importing nodes into the scene
		class SceneImportCommand : public UndoCommand
		{
		public:
			SceneImportCommand( Scene* scene, const Helium::FilePath& path, Scene::ImportAction importAction = Scene::ImportActions::Import, uint32_t importFlags = Scene::ImportFlags::None, HierarchyNode* importRoot = NULL, const Reflect::MetaClass* importReflectType = NULL )
				: m_Scene( scene )
				, m_Path( path )
				, m_ImportAction( importAction )
				, m_ImportFlags( importFlags )
				, m_ImportRoot( importRoot )
				, m_ImportReflectType( importReflectType )
			{ 
			}

			virtual void Undo() override
			{
				if ( m_UndoCommand )
				{
					m_UndoCommand->Undo();
				}
			}

			virtual void Redo() override
			{
				if ( m_Scene )
				{
					if ( m_UndoCommand )
					{
						m_UndoCommand->Redo();
					}
					else
					{
						m_UndoCommand = m_Scene->Import( m_Path, m_ImportAction, m_ImportFlags, m_ImportRoot, m_ImportReflectType );
					}
				}
			}

		private:
			Scene* m_Scene;
			Helium::FilePath m_Path;
			Scene::ImportAction m_ImportAction;
			uint32_t m_ImportFlags;
			HierarchyNode* m_ImportRoot;
			UndoCommandPtr m_UndoCommand;
			const Reflect::MetaClass* m_ImportReflectType;
		};

		/// Command for selecting nodes in the scene
		class SceneSelectCommand : public UndoCommand
		{
		public:

			SceneSelectCommand( Scene* scene, OS_ObjectDumbPtr& selection  ) 
				: m_Scene( scene )
				, m_Selection( selection )
			{ 
				if( m_Scene )
				{
					m_OldSelection = m_Scene->GetSelection().GetItems();
				}
			}

			virtual void Undo() override
			{
				if ( m_Scene )
				{
					m_Scene->GetSelection().SetItems( m_OldSelection );
				}
			}

			virtual void Redo() override
			{
				if ( m_Scene )
				{
					m_Scene->GetSelection().SetItems( m_Selection );
				}
			}

		private:
			Scene* m_Scene;
			OS_ObjectDumbPtr m_Selection;
			OS_ObjectDumbPtr m_OldSelection;
		};

		namespace SceneContexts
		{
			enum SceneContext
			{
				None,
				Normal,
				Loading,
				Saving,
				Picking,
			};
		}
		typedef SceneContexts::SceneContext SceneContext;

		struct ResolveSceneArgs
		{
			Editor::Viewport* m_Viewport;
			Helium::SceneDefinitionPtr m_Definition;
			mutable Scene* m_Scene;

			ResolveSceneArgs( Editor::Viewport* viewport, Helium::SceneDefinition& definition )
				: m_Viewport( viewport )
				, m_Definition( &definition )
				, m_Scene( NULL )
			{
			}
		};

		struct ReleaseSceneArgs
		{
			mutable Scene* m_Scene;

			ReleaseSceneArgs( Scene* scene )
				: m_Scene( scene )
			{
			}
		};

		struct SceneEditingArgs
		{
			Scene* m_Scene;
			mutable bool m_Veto;

			SceneEditingArgs( Scene* scene )
				: m_Scene( scene )
				, m_Veto( false )
			{
			}
		};

		// update the status bar of the frame of this instance of the scene editor
		struct SceneStatusChangeArgs
		{
			const std::string& m_Status;

			SceneStatusChangeArgs( const std::string& status )
				: m_Status (status)
			{
			}
		};

		struct SceneContextChangeArgs
		{
			SceneContext m_OldContext;
			SceneContext m_NewContext;

			SceneContextChangeArgs( SceneContext oldContext, SceneContext newContext )
				: m_OldContext( oldContext )
				, m_NewContext( newContext )
			{
			}
		};

		// arguments and delegates for when a node is changed (in this case, added to or removed from the scene)
		struct NodeChangeArgs
		{
			Editor::SceneNode* m_Node;

			NodeChangeArgs( Editor::SceneNode* node )
				: m_Node( node )
			{
			}
		};

		// event for loading a scene.
		struct LoadArgs
		{
			Editor::Scene* m_Scene;
			bool m_Success; // Only valid for finished loading events

			LoadArgs( Editor::Scene* scene, bool loadedOk = false )
				: m_Scene( scene )
				, m_Success( loadedOk )
			{
			}
		};

		struct ExecuteArgs
		{
			Editor::Scene* m_Scene;
			bool m_Interactively;

			ExecuteArgs( Editor::Scene* scene, bool interactively )
				: m_Scene( scene )
				, m_Interactively( interactively )
			{
			}
		};

		struct UndoCommandArgs
		{
			Editor::Scene* m_Scene;
			UndoCommandPtr m_Command;

			UndoCommandArgs( Editor::Scene* scene, UndoCommandPtr command )
				: m_Scene( scene )
				, m_Command( command )
			{
			}
		};
	}
}
