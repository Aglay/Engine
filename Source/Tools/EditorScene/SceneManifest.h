#pragma once

#include "Foundation/FilePath.h"
#include "Reflect/TranslatorDeduction.h"
#include "Math/Vector3.h"

#include "EditorScene/API.h"

namespace Helium
{
	namespace Editor
	{
		class HELIUM_EDITOR_SCENE_API SceneManifest : public Reflect::Object
		{
		public:
			Vector3 m_BoundingBoxMin;
			Vector3 m_BoundingBoxMax;
			std::set< Helium::FilePath > m_Assets;

			HELIUM_DECLARE_CLASS(SceneManifest, Reflect::Object);
			static void PopulateMetaType( Reflect::MetaStruct& comp );
		};

		typedef Helium::StrongPtr<SceneManifest> SceneManifestPtr;
		typedef std::vector<SceneManifestPtr> V_SceneManifest;
	}
}