#pragma once

#include "PanelButton.h"

namespace Helium
{
    namespace Editor
    {
        class EditorButton : public PanelButton
        {        
        public:
            EditorButton();
            EditorButton( wxWindow *parent,
                wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxBORDER_THEME,
                const wxString& name = wxT( "EditorButton" ) );
            virtual ~EditorButton();

        protected:
            void OnUpdateUI( wxUpdateUIEvent& event );

            bool m_ShowText;
            bool m_ShowIcons;

            int m_IconSize;

        public:
            DECLARE_DYNAMIC_CLASS( EditorButton )
        };
    }
}