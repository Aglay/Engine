#pragma once

#include "ProjectViewModel.h"

#include "Application/OrderedSet.h"
#include "Application/DocumentManager.h"

#include "EditorScene/Selection.h"

#include "Editor/EditorGeneratedWrapper.h"
#include "Editor/DragDrop/FileDropTarget.h"

namespace Helium
{
    namespace Editor
    {
        class ProjectPanel : public ProjectPanelGenerated
        {
        public:
            ProjectPanel( wxWindow* parent, DocumentManager* documentManager );
            virtual ~ProjectPanel();

            void OpenProject( const FilePath& project );
            void CloseProject();

            void SetActive( const AssetPath& path, bool active );

        protected:
            void GeneralSettingsChanged( const Reflect::ObjectChangeArgs& args );

            void PopulateOpenProjectListItems();
            void OnRecentProjectButtonClick( wxCommandEvent& event );
            virtual void OnOpenProjectButtonClick( wxCommandEvent& event );
            virtual void OnNewProjectButtonClick( wxCommandEvent& event );

            // UI event handlers
            virtual void OnContextMenu( wxContextMenuEvent& event );
            
            virtual void OnActivateItem( wxDataViewEvent& event );

            virtual void OnUpdateUI( wxUpdateUIEvent& event );

            void OnAddItems( wxCommandEvent& event );
			void OnDeleteItems( wxCommandEvent& event );
			void OnLoadForEdit( wxCommandEvent& event );
			void OnSave( wxCommandEvent& event );

			//virtual void OnAddFile( wxCommandEvent& event ) override;
			//virtual void OnDeleteFile( wxCommandEvent& event ) override;

            void OnOptionsMenuOpen( wxMenuEvent& event );
            void OnOptionsMenuClose( wxMenuEvent& event );
            void OnOptionsMenuSelect( wxCommandEvent& event );

            void OnSelectionChanged( wxDataViewEvent& event );

            void OnDragOver( FileDroppedArgs& args );
            virtual void OnDroppedFiles( const FileDroppedArgs& args );

        protected:
            DocumentManager* m_DocumentManager;
            FilePath m_Project;
            wxObjectDataPtr< ProjectViewModel > m_Model;
            wxMenu* m_OptionsMenu;
            wxMenu m_ContextMenu;

            OrderedSet< FilePath* > m_Selected;

            typedef std::map< wxWindowID, FilePath > M_ProjectMRULookup;
            M_ProjectMRULookup m_ProjectMRULookup;
            
            FileDropTarget* m_DropTarget;

			Editor::Selection m_Selection;
        };
    }
}