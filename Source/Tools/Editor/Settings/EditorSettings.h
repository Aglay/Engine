#pragma once

#include "EditorScene/SettingsManager.h"

#include "Editor/API.h"
#include "Editor/MRU/MRU.h"

namespace Helium
{
    namespace Editor
    {
        class IconSize
        {
        public:
            enum Enum
            {
                Small,
                Medium,
                Large,
            };

            HELIUM_DECLARE_ENUM( IconSize );

            static void PopulateMetaType( Reflect::MetaEnum& info )
            {
                info.AddElement( Small, "Small" );
                info.AddElement( Medium, "Medium" );
                info.AddElement( Large, "Large" );
            }
        };

        class EditorSettings : public Settings
        {
        public:
            EditorSettings();

            std::vector< std::string >& GetMRUProjects();
            void SetMRUProjects( MRU< std::string >* mru );

            bool GetReopenLastProjectOnStartup() const;
            void SetReopenLastProjectOnStartup( bool value );

            bool GetShowFileExtensionsInProjectView() const;
            void SetShowFileExtensionsInProjectView( bool value );

            bool GetEnableAssetTracker() const;
            void SetEnableAssetTracker( bool value );

            HELIUM_DECLARE_CLASS( EditorSettings, Settings );
            static void PopulateMetaType( Reflect::MetaStruct& comp );

        public:
            std::vector< std::string > m_MRUProjects;
            
            bool m_ReopenLastProjectOnStartup;
            bool m_ShowFileExtensionsInProjectView;
            bool m_EnableAssetTracker;
            bool m_ShowTextOnButtons;
            bool m_ShowIconsOnButtons;
            IconSize m_IconSizeOnButtons;
        };

        typedef Helium::StrongPtr< EditorSettings > GeneralSettingsPtr;

    }
}