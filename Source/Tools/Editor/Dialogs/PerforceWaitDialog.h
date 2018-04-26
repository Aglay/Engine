#pragma once

#include "Foundation/Profile.h"
#include "Editor/Perforce/P4Provider.h"
#include "Editor/PerforceGenerated.h"

#include <wx/dialog.h>
#include <wx/timer.h>

namespace Helium
{
    namespace Editor
    {
        typedef bool (*CancelWaitFunc)();

        class PerforceWaitDialog : public wxDialog
        {
        public:
            static void EnableWaitDialog( bool enable );

            PerforceWaitDialog();

            int ShowModal( Perforce::WaitInterface* waitInterface );
            void Throb();

        private:
            PerforceWaitPanelGenerated* m_Panel;
        };

        class PerforceWaitTimer : public wxTimer
        {
        public:
            PerforceWaitTimer( class PerforceWaitDialog* dialog, Perforce::WaitInterface* waitInterface );

        protected:
            void Notify();

        private:
            PerforceWaitDialog*       m_WaitDialog;
            Perforce::WaitInterface*  m_WaitInterface;
            SimpleTimer               m_WaitTimer;
        };
    }
}