#include "Precompile.h"
#include "TransformManipulator.h"
#include "EditorScene/Transform.h"

#include "EditorScene/Viewport.h"
#include "EditorScene/Camera.h"
#include "EditorScene/Colors.h"

HELIUM_DEFINE_ABSTRACT( Helium::Editor::TransformManipulator );

using namespace Helium;
using namespace Helium::Editor;

TransformManipulator::TransformManipulator(const ManipulatorMode mode, Editor::Scene* scene, PropertiesGenerator* generator)
	: Tool(scene, generator)
	, m_Mode (mode)
	, m_SelectedAxes (MultipleAxes::None)
	, m_Left (false)
	, m_Middle (false)
	, m_Right (false)
	, m_StartX (0)
	, m_StartY (0)
	, m_Manipulating (false)
	, m_Manipulated (false)
	, m_AxisMaterial (Editor::Colors::BLACK)
	, m_SelectedAxisMaterial (Editor::Colors::YELLOW)
{
	m_Scene->AddSelectionChangedListener( SelectionChangedSignature::Delegate (this, &TransformManipulator::SelectionChanged) );

	SelectionChanged(m_Scene->GetSelection().GetItems());
}

TransformManipulator::~TransformManipulator()
{
	m_Scene->RemoveSelectionChangedListener( SelectionChangedSignature::Delegate (this, &TransformManipulator::SelectionChanged) );
}

void TransformManipulator::Cleanup()
{
	SetResult();
}

void TransformManipulator::SelectionChanged(const SelectionChangeArgs& args)
{
	m_ManipulatorAdapters.clear();

	OS_ObjectDumbPtr::Iterator itr = args.m_Selection.Begin();
	OS_ObjectDumbPtr::Iterator end = args.m_Selection.End();
	for ( ; itr != end; ++itr )
	{
		Editor::HierarchyNode* h = Reflect::SafeCast< Editor::HierarchyNode >( *itr );
		if ( h )
		{
			h->ConnectManipulator(this);
		}
	}
}

Vector3 TransformManipulator::GetAxesNormal(AxesFlags axes)
{
	Vector3 result;

	if (axes == MultipleAxes::All)
	{
		m_View->GetCamera()->GetDirection(result);

		return result;
	}

	if ((axes & MultipleAxes::X) != MultipleAxes::None)
	{
		if ((axes & MultipleAxes::Y) != MultipleAxes::None)
			return Vector3::BasisZ;

		if ((axes & MultipleAxes::Z) != MultipleAxes::None)
			return Vector3::BasisY;

		return Vector3::BasisX;
	}

	if ((axes & MultipleAxes::Y) != MultipleAxes::None)
	{
		if ((axes & MultipleAxes::X) != MultipleAxes::None)
			return Vector3::BasisZ;

		if ((axes & MultipleAxes::Z) != MultipleAxes::None)
			return Vector3::BasisX;

		return Vector3::BasisY;
	}

	if ((axes & MultipleAxes::Z) != MultipleAxes::None)
	{
		if ((axes & MultipleAxes::X) != MultipleAxes::None)
			return Vector3::BasisY;

		if ((axes & MultipleAxes::Y) != MultipleAxes::None)
			return Vector3::BasisX;

		return Vector3::BasisZ;
	}

	return result;
}

bool TransformManipulator::MouseDown( const MouseButtonInputEvent& e )
{
	if (e.LeftDown())
		m_Left = true;

	if (e.MiddleDown())
		m_Middle = true;

	if (e.RightDown())
		m_Right = true;

	m_StartX = e.GetPosition().x;
	m_StartY = e.GetPosition().y;

	m_Manipulated = false;
	m_Manipulating = true;

	return true;
}

void TransformManipulator::MouseUp( const MouseButtonInputEvent& e )
{
	m_Manipulating = false;

	if (m_Manipulated)
	{
		SetResult();
	}

	if (e.LeftUp())
		m_Left = false;

	if (e.MiddleUp())
		m_Middle = false;

	if (e.RightUp())
		m_Right = false;

	m_AllowSelection = true;
}

void TransformManipulator::MouseMove( const MouseMoveInputEvent& e )
{
	if ( e.Dragging() )
	{
		// clear the current highlight
		m_Scene->ClearHighlight( ClearHighlightArgs (true) );

		// disallow selection when dragging
		m_AllowSelection = false;
	}
	else
	{
		// reset cached button state
		m_Left = m_Middle = m_Right = false;

		// allow selection when not dragging
		m_AllowSelection = true;
	}
}
