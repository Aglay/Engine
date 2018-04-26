#pragma once

#include "Application/FileDialog.h"

namespace Helium
{
    namespace Editor
    {
        class FileDialogDisplayer
        {
        public:
            FileDialogDisplayer( wxWindow* parent = NULL )
                : m_Parent( parent )
            {

            }

            void SetParent( wxWindow* parent )
            {
                m_Parent = parent;
            }

            void DisplayFileDialog( const FileDialogArgs& args );

        private:
            wxWindow* m_Parent;
        };
    }
}