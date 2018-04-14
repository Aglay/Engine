#include "EditorScenePch.h"
#include "LocatorCreateTool.h"

#include "EditorScene/Scene.h"
#include "EditorScene/Locator.h"
#include "EditorScene/Pick.h"

HELIUM_DEFINE_ABSTRACT( Helium::Editor::LocatorCreateTool );

using namespace Helium;
using namespace Helium::Editor;

LocatorShape LocatorCreateTool::s_Shape = LocatorShape::Cross;

LocatorCreateTool::LocatorCreateTool(Scene* scene, PropertiesGenerator* generator)
	: CreateTool (scene, generator)
{

}

LocatorCreateTool::~LocatorCreateTool()
{

}

TransformPtr LocatorCreateTool::CreateNode()
{
#ifdef SCENE_DEBUG_RUNTIME_DATA_SELECTION

	LocatorPtr v = new Locator( s_Shape );

	v->RectifyRuntimeData();

	LLocatorPtr locator = new Locator( m_Scene, v );

	m_Scene->AddObject( locator );

	{
		OS_SceneNodeDumbPtr selection;
		selection.push_back( locator );
		m_Scene->GetSelection().SetItems( selection );

		m_Scene->GetSelection().Clear();
	}

	m_Scene->RemoveObject( locator );

	return locator;

#else

	TransformPtr node = new Locator ();
	node->SetOwner( m_Scene );
	node->Initialize();
	return node;

#endif
}

void LocatorCreateTool::CreateProperties()
{
	m_Generator->PushContainer( "Locator" );
	{
		m_Generator->PushContainer();
		{
			m_Generator->AddLabel( "Shape" );

			Inspect::Choice* choice = m_Generator->AddChoice<int>( new Helium::MemberProperty<LocatorCreateTool, int>(this, &LocatorCreateTool::GetLocatorShape, &LocatorCreateTool::SetLocatorShape) );
			choice->a_IsDropDown.Set( true );
			std::vector< Inspect::ChoiceItem > items;

			{
				std::ostringstream str;
				str << LocatorShape::Cross;
				items.push_back( Inspect::ChoiceItem( "Cross", str.str() ) );
			}

			{
				std::ostringstream str;
				str << LocatorShape::Cube;
				items.push_back( Inspect::ChoiceItem( "Cube", str.str() ) );
			}

			choice->a_Items.Set( items );
		}
		m_Generator->Pop();
	}
	m_Generator->Pop();

	Base::CreateProperties();
}

int LocatorCreateTool::GetLocatorShape() const
{
	return (int)s_Shape;
}

void LocatorCreateTool::SetLocatorShape(int value)
{
	s_Shape = static_cast< LocatorShape::Enum >( value );

	Place(Matrix4::Identity);
}
