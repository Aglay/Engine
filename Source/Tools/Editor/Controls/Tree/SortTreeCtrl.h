#pragma once

#include <wx/treectrl.h>

#include "Platform/System.h"

namespace Helium
{
    namespace Editor
    {
        // MetaEnum of different sorting methods useable by the tree control
        namespace TreeSortMethods
        {
            enum TreeSortMethod
            {
                Normal, // Regular, alphabetical sorting. Ex: Item1, Item10, Item2
                Natural // Natrual string ordering, keeping numeric values in order. Ex: Item1, Item2, Item10
            };
        };
        typedef TreeSortMethods::TreeSortMethod TreeSortMethod;

        /////////////////////////////////////////////////////////////////////////////
        // Wrapper for TreeCtrl that provides options for turning sorting on and
        // off.  If sorting is turned on, one of the TreeSortMethods above can be
        // specified for how the tree should be sorted.  By default, sorting is
        // turned on, and TreeSortMethods::Natural is used.
        // 
        class SortTreeCtrl : public wxTreeCtrl
        {
        protected:
            bool m_AllowSorting;
            TreeSortMethod m_SortMethod;

        public:
            SortTreeCtrl();
            SortTreeCtrl( wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTR_HAS_BUTTONS, const wxValidator& validator = wxDefaultValidator, const wxString& name = wxT( "listCtrl" ) );
            virtual ~SortTreeCtrl();

            bool IsSortingEnabled() const;
            void EnableSorting( bool enable = true );
            void DisableSorting() { EnableSorting( false ); }

            void Sort( const wxTreeItemId& root = NULL, bool recursive = true );

            TreeSortMethod GetSortMethod() const;
            void SetSortMethod( TreeSortMethod method );

            // TreeCtrl overrides
        public:
            virtual int OnCompareItems( const wxTreeItemId& item1, const wxTreeItemId& item2 ) override;
            virtual void SortChildren( const wxTreeItemId& item ) override;

        private:
            // Required so that OnCompareItems will be called
            DECLARE_DYNAMIC_CLASS( SortTreeCtrl )
        };

    } // namespace Editor
}