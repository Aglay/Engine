#pragma once

#include <map>

#include "Application/OrderedSet.h"
#include "Application/UndoQueue.h"

#include "Foundation/TUID.h"

#include "Reflect/Object.h"

#include "EditorScene/API.h"

namespace Helium
{
	namespace Editor
	{
		class Scene;
		typedef Helium::StrongPtr< Scene > ScenePtr;

		class SceneNode;
		typedef Helium::StrongPtr< SceneNode > SceneNodePtr;

		class SceneManifest;
		class Graph;

		typedef std::vector< SceneNode* > V_SceneNodeDumbPtr;
		typedef std::vector< SceneNodePtr > V_SceneNodeSmartPtr;

		typedef std::set< SceneNode* > S_SceneNodeDumbPtr;
		typedef std::set< SceneNodePtr > S_SceneNodeSmartPtr;

		typedef OrderedSet< SceneNode* > OS_SceneNodeDumbPtr;
		typedef OrderedSet< SceneNodePtr > OS_SceneNodeSmartPtr;

		typedef OrderedSet< Reflect::Object* > OS_ObjectDumbPtr;
		typedef OrderedSet< Reflect::Object > OS_ObjectSmartPtr;

		typedef std::map< Helium::TUID, Editor::SceneNode* > M_SceneNodeDumbPtr;
		typedef std::map< Helium::TUID, SceneNodePtr > M_SceneNodeSmartPtr;

		namespace GraphDirections
		{
			enum GraphDirection
			{
				Downstream,
				Upstream,
			};
			static const uint32_t Count = 2;
		}

		typedef GraphDirections::GraphDirection GraphDirection;

		namespace NodeStates
		{
			enum NodeState
			{
				Evaluating,
				Clean,
				Dirty,
			};
			static const uint32_t Count = 3;
		}

		typedef NodeStates::NodeState NodeState;

		struct SceneNodeChangeArgs
		{
			SceneNodeChangeArgs( Editor::SceneNode* node ) : m_Node( node ) {}

			Editor::SceneNode* m_Node;
		};
		typedef Helium::Signature< const SceneNodeChangeArgs& > SceneNodeChangeSignature;

		//
		// Scene Node
		//  Editor application object that can have a dependencies on other objects.
		//   o Has an evaluate function that does work that is dependent on other objects being clean.
		//   o Dependencies of this object are traversed by Graph to compute the order of evaluation.
		//

		class HELIUM_EDITOR_SCENE_API SceneNode : public Reflect::Object
		{
		public:
			HELIUM_DECLARE_ABSTRACT( SceneNode, Reflect::Object );
			static void PopulateMetaType( Reflect::MetaStruct& comp );

			SceneNode();
			~SceneNode();

			NodeState GetNodeState(GraphDirection direction) const
			{
				return m_NodeStates[direction];
			}
			void SetNodeState(GraphDirection direction, NodeState value)
			{
				m_NodeStates[direction] = value;
			}

			Graph* GetGraph()
			{
				return m_Graph;
			}
			const Graph* GetGraph() const
			{
				return m_Graph;
			}
			void SetGraph(Graph* value)
			{
				m_Graph = value;
			}

			Editor::Scene* GetOwner() const
			{
				return m_Owner;
			}
			void SetOwner( Scene* owner )
			{
				HELIUM_ASSERT( m_Owner == NULL || m_Owner == owner );
				m_Owner = owner;
			}

			virtual bool IsTransient() const
			{
				return m_IsTransient;
			}
			virtual void SetTransient( bool isTransient )
			{
				m_IsTransient = isTransient;
			}

			S_SceneNodeDumbPtr& GetAncestors()
			{
				return m_Ancestors;
			}
			const S_SceneNodeDumbPtr& GetAncestors() const
			{
				return m_Ancestors;
			}

			S_SceneNodeSmartPtr& GetDescendants()
			{
				return m_Descendants;
			}
			const S_SceneNodeSmartPtr& GetDescendants() const
			{
				return m_Descendants;
			}

			// 
			// Visibility indicates that the item shows up in the 3D view (assuming
			// that additional options are enabled, such as the pointer or bounds).
			// Should be overridden in derived classes to accurately determine this.
			// Derived classes should raise the m_VisibilityChanged event if they 
			// make a change to the node's visibility so that it it accurately
			// Reflected throughout the UI.
			// 
			virtual bool IsVisible() const
			{
				return true;
			}

			//
			// ID and Name, every node has a unique name
			//

		public:
			const Helium::TUID& GetID() const;
			void SetID(const Helium::TUID& id);

			virtual std::string GenerateName() const;
			virtual const std::string& GetName() const;
			virtual void SetName(const std::string& newName);

			virtual bool UseGivenName() const;
			bool UseAutoName() const
			{
				return !UseGivenName();
			}

			virtual void SetUseGivenName(bool use);
			void SetUseAutoName(bool use)
			{
				SetUseGivenName(!use);
			}

			virtual void SetGivenName(const std::string& newName);
			virtual void Rename(const std::string& newName);

			//
			// VisitedID tracks which eval traversal we were last visited on
			//

			uint32_t GetVisitedID() const
			{
				return m_VisitedID;
			}

			void SetVisitedID(uint32_t id)
			{
				m_VisitedID = id;
			}

			//
			// Node management
			//

			// Reset object state, used to decomposing dependency graph
			virtual void Reset();

			// Callback after new scene data is loaded, and initial evaluation is complete
			virtual void Initialize();

			// Check for initialization state
			bool IsInitialized()
			{
				return m_IsInitialized;
			}

			//
			// These protected functions provide a way to extend the logic of an atomic
			//  operation on a dependency
			//

		protected:
			virtual void ConnectDescendant( Editor::SceneNode* descendant );
			virtual void DisconnectDescendant( Editor::SceneNode* descendant );

			virtual void ConnectAncestor( Editor::SceneNode* ancestor );
			virtual void DisconnectAncestor( Editor::SceneNode* ancestor );

			//
			// These public functions ensure that the dependency is created bidirectionally and that
			//  the dependent object is added to the graph and classified
			//

		public:
			void CreateDependency(Editor::SceneNode* ancestor);
			void RemoveDependency(Editor::SceneNode* ancestor);

			//
			// Graph seggregation and merging
			//
			// Insert reconnects trimmed decendant connections from all non-pruned dependent graph nodes
			//  Note this uses the preserved ancestor connections for each pruned node
			//
			// Prune trims the decendant connection from all dependent graph nodes
			//  Note this preserves the ancestor connection for each pruned node so we can do insert easily
			//

		public:
			virtual void Insert( Graph* graph, V_SceneNodeDumbPtr& insertedNodes );
			virtual void Prune( V_SceneNodeDumbPtr& prunedNodes );

			//
			// Evaluate
			//

		protected:
			// entry point from the graph
			virtual void DoEvaluate(GraphDirection direction); friend Graph;

		public:
			// Makes us Evaluate() on next graph evaluation
			virtual uint32_t Dirty();

			// overridable method for derived classes
			virtual void Evaluate(GraphDirection direction);

			//
			// Manifest
			//

		public:
			// used for manifest generation during save
			virtual void PopulateManifest( SceneManifest* manifest ) const;

			//
			// Resources
			//

		public:
			// call to create state resources
			virtual void Create();

			// call to delete state resources
			virtual void Delete();

			// Make object dirty and iterate world
			virtual void Execute(bool interactively);

			//
			// Undo/Redo Snapshot
			//

			// Retrieve serialzed data for this object into the parameter
			void GetState( Reflect::ObjectPtr& state ) const;

			// Restore serialized data from the element for this object
			void SetState( const Reflect::ObjectPtr& state );

			// Get undo command for this object's state (uses GetState/SetState above)
			virtual UndoCommandPtr SnapShot( Reflect::Object* newState = NULL );

			//
			// Selected state
			//

			// Is this object currently selectable?
			//  Sometimes objects can on a per-instance or per-type basis decided to NOT be selectable
			//  This prototype exposes the capability to override the selection of an object
			virtual bool IsSelectable() const;

			// Get/Set selected state
			virtual bool IsSelected() const;
			virtual void SetSelected(bool);

			//
			// Events
			//

		private:
			SceneNodeChangeSignature::Event m_NameChanging;
		public:
			void AddNameChangingListener( const SceneNodeChangeSignature::Delegate& listener )
			{
				m_NameChanging.Add( listener );
			}
			void RemoveNameChangingListener( const SceneNodeChangeSignature::Delegate& listener )
			{
				m_NameChanging.Remove( listener );
			}

		private:
			SceneNodeChangeSignature::Event m_NameChanged;
		public:
			void AddNameChangedListener( const SceneNodeChangeSignature::Delegate& listener )
			{
				m_NameChanged.Add( listener );
			}
			void RemoveNameChangedListener( const SceneNodeChangeSignature::Delegate& listener )
			{
				m_NameChanged.Remove( listener );
			}

		protected:
			SceneNodeChangeSignature::Event m_VisibilityChanged;
		public:
			void AddVisibilityChangedListener( const SceneNodeChangeSignature::Delegate& listener )
			{
				m_VisibilityChanged.Add( listener );
			}
			void RemoveVisibilityChangedListener( const SceneNodeChangeSignature::Delegate& listener )
			{
				m_VisibilityChanged.Remove( listener );
			}

		protected:
			// Reflected
			TUID                    m_ID;                                   // The ID of the node
			std::string             m_DefaultName;                          // generated name of the node
			std::string             m_GivenName;                            // user created name (can be empty)
			bool                    m_UseGivenName;                         // should the name change when the object does?

			// Non-reflected
			bool                    m_IsInitialized;                        // has Initialize() been called?
			bool                    m_IsSelected;                           // is this in the selection list?
			bool                    m_IsTransient;                          // is this a temp object?

			Scene*                  m_Owner;                                // the scene that owns us
			Graph*                  m_Graph;                                // the graph that evaluates us
			S_SceneNodeDumbPtr      m_Ancestors;                            // nodes that are evaluated before this Node
			S_SceneNodeSmartPtr     m_Descendants;                          // nodes that are evaluated after this Node
			NodeState               m_NodeStates[ GraphDirections::Count ]; // our current state
			uint32_t                m_VisitedID;                            // data cached for evaluation
		};
	}
}