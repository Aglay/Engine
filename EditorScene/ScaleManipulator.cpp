#include "EditorScenePch.h"
#include "ScaleManipulator.h"

#include "EditorScene/Pick.h"
#include "EditorScene/Viewport.h"
#include "EditorScene/Camera.h"
#include "EditorScene/Colors.h"

#include "PrimitiveAxes.h"
#include "PrimitiveCube.h"

#include "EditorScene/Scene.h"
#include "SceneSettings.h"

HELIUM_DEFINE_ABSTRACT( Helium::Editor::ScaleManipulator );

using namespace Helium;
using namespace Helium::Editor;

ScaleManipulator::ScaleManipulator( SettingsManager* settingsManager, const ManipulatorMode mode, Editor::Scene* scene, PropertiesGenerator* generator)
	: Editor::TransformManipulator (mode, scene, generator)
	, m_SettingsManager( settingsManager )
	, m_Size( 0.3f )
	, m_GridSnap( false )
	, m_Distance( 1.0f )
{
	SceneSettings* settings = m_SettingsManager->GetSettings< SceneSettings >();
	m_Size = settings->ScaleManipulatorSize();
	m_GridSnap = settings->ScaleManipulatorGridSnap();
	m_Distance = settings->ScaleManipulatorDistance();

	m_Axes = new Editor::PrimitiveAxes ();
	m_Axes->Update();

	m_Cube = new Editor::PrimitiveCube ();
	m_Cube->SetSolid(true);
	m_Cube->Update();

	m_XCube = new Editor::PrimitiveCube ();
	m_XCube->SetSolid(true);
	m_XCube->Update();

	m_YCube = new Editor::PrimitiveCube ();
	m_YCube->SetSolid(true);
	m_YCube->Update();

	m_ZCube = new Editor::PrimitiveCube ();
	m_ZCube->SetSolid(true);
	m_ZCube->Update();

	ResetSize();
}

ScaleManipulator::~ScaleManipulator()
{
	delete m_Axes;
	delete m_Cube;

	delete m_XCube;
	delete m_YCube;
	delete m_ZCube;
}

void ScaleManipulator::ResetSize()
{
	m_Axes->m_Length = 1.0f;
	m_Cube->SetRadius(0.05f);

	m_XCube->SetRadius(0.04f);
	m_XPosition = Vector3::BasisX;
	m_YCube->SetRadius(0.04f);
	m_YPosition = Vector3::BasisY;
	m_ZCube->SetRadius(0.04f);
	m_ZPosition = Vector3::BasisZ;
}

void ScaleManipulator::ScaleTo(float factor)
{
	ResetSize();

	m_Axes->m_Length *= factor;
	m_Axes->Update();

	m_Cube->ScaleRadius( factor );
	m_Cube->Update();

	m_XCube->ScaleRadius( factor );
	m_XCube->Update();
	m_XPosition *= factor;

	m_YCube->ScaleRadius( factor );
	m_YCube->Update();
	m_YPosition *= factor;

	m_ZCube->ScaleRadius( factor );
	m_ZCube->Update();
	m_ZPosition *= factor;
}

void ScaleManipulator::Evaluate()
{
	ScaleManipulatorAdapter* primary = PrimaryObject<ScaleManipulatorAdapter>();

	if (primary)
	{
		// get the transform for our object
		Matrix4 frame = primary->GetFrame(ManipulatorSpace::Object);

		// compute the scaling factor
		float factor = m_View->GetCamera()->ScalingTo(Vector3 (frame.t.x, frame.t.y, frame.t.z));

		// scale this
		ScaleTo(factor * m_Size);
	}
}

void ScaleManipulator::SetResult()
{
	if (m_Manipulated)
	{
		m_Manipulated = false;

		if (!m_ManipulationStart.empty())
		{
			ScaleManipulatorAdapter* primary = PrimaryObject<ScaleManipulatorAdapter>();

			if (primary != NULL)
			{
				if (!primary->GetNode()->GetOwner()->IsEditable())
				{
					std::vector< ScaleManipulatorAdapter* > set = CompleteSet<ScaleManipulatorAdapter>();
					for ( std::vector< ScaleManipulatorAdapter* >::const_iterator itr = set.begin(), end = set.end(); itr != end; ++itr )
					{
						Vector3 val = m_ManipulationStart.find(*itr)->second.m_StartValue;
						(*itr)->SetValue(Scale (val));
					}
				}
				else
				{
					BatchUndoCommandPtr batch = new BatchUndoCommand ();

					std::vector< ScaleManipulatorAdapter* > set = CompleteSet<ScaleManipulatorAdapter>();
					for ( std::vector< ScaleManipulatorAdapter* >::const_iterator itr = set.begin(), end = set.end(); itr != end; ++itr )
					{
						// get current (resultant) value
						Scale result ((*itr)->GetValue());

						// set start value without undo support so its set for handling undo state
						(*itr)->SetValue(Scale (m_ManipulationStart.find(*itr)->second.m_StartValue));

						// set result with undo support
						batch->Push ( (*itr)->SetValue(Scale (result)) );
					}

					m_Scene->Push( batch );
				}

				// apply modification
				primary->GetNode()->GetOwner()->Execute(false);
			}
		}
	}
}

void ScaleManipulator::Draw( BufferedDrawer* pDrawer )
{
	ScaleManipulatorAdapter* primary = PrimaryObject<ScaleManipulatorAdapter>();

	if (primary == NULL)
	{
		return;
	}

	ManipulationStart start;

	M_ManipulationStart::iterator found = m_ManipulationStart.find( primary );

	if (found != m_ManipulationStart.end())
	{
		start = found->second;
	}
	else
	{
		start.m_StartValue.x = 1.0f;
		start.m_StartValue.y = 1.0f;
		start.m_StartValue.z = 1.0f;
	}

	Scale offset;

	if (m_Manipulating)
	{
		offset.x = primary->GetValue().x / start.m_StartValue.x;
		offset.y = primary->GetValue().y / start.m_StartValue.y;
		offset.z = primary->GetValue().z / start.m_StartValue.z;

		if ((primary->GetValue().x / fabs(primary->GetValue().x)) != (start.m_StartValue.x / fabs(start.m_StartValue.x)))
		{
			offset.x *= -1.0f;
		}

		if ((primary->GetValue().y / fabs(primary->GetValue().y)) != (start.m_StartValue.y / fabs(start.m_StartValue.y)))
		{
			offset.y *= -1.0f;
		}

		if ((primary->GetValue().z / fabs(primary->GetValue().z)) != (start.m_StartValue.z / fabs(start.m_StartValue.z)))
		{
			offset.z *= -1.0f;
		}
	}

	// get the transform for our object
	Matrix4 frame = Matrix4 (offset) * primary->GetFrame(ManipulatorSpace::Object).Normalized();

	Scale inverse (1.0f / offset.x, 1.0f / offset.y, 1.0f / offset.z);

	AxesFlags parallelAxis = m_View->GetCamera()->ParallelAxis(frame, HELIUM_CRITICAL_DOT_PRODUCT);

#ifdef VIEWPORT_REFACTOR
	m_View->GetDevice()->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&frame);
	m_Axes->DrawAxes(args, (AxesFlags)(~parallelAxis & MultipleAxes::All));
#endif

	if (m_SelectedAxes == MultipleAxes::All)
	{
		m_AxisMaterial = Editor::Colors::YELLOW;
	}
	else
	{
		m_AxisMaterial = Editor::Colors::SKYBLUE;
	}

#ifdef VIEWPORT_REFACTOR
	m_View->GetDevice()->SetMaterial(&m_AxisMaterial);
	m_View->GetDevice()->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&(Matrix4 (inverse) * frame));
	m_Cube->Draw(args);
#endif

#ifdef VIEWPORT_REFACTOR
	if (parallelAxis != MultipleAxes::X)
	{
		SetAxisMaterial(MultipleAxes::X);
		m_View->GetDevice()->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&(Matrix4 (inverse) * Matrix4 (m_XPosition) * frame));
		m_XCube->Draw(args);
	}

	if (parallelAxis != MultipleAxes::Y)
	{
		SetAxisMaterial(MultipleAxes::Y);
		m_View->GetDevice()->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&(Matrix4 (inverse) * Matrix4 (m_YPosition) * frame));
		m_YCube->Draw(args);
	}

	if (parallelAxis != MultipleAxes::Z)
	{
		SetAxisMaterial(MultipleAxes::Z);
		m_View->GetDevice()->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&(Matrix4 (inverse) * Matrix4 (m_ZPosition) * frame));
		m_ZCube->Draw(args);
	}
#endif
}

bool ScaleManipulator::Pick( PickVisitor* pick )
{
	ScaleManipulatorAdapter* primary = PrimaryObject<ScaleManipulatorAdapter>();

	if (primary == NULL || pick->GetPickType() != PickTypes::Line)
	{
		return false;
	}

	// get the transform for our object
	Matrix4 frame = primary->GetFrame(ManipulatorSpace::Object).Normalized();

	// setup the pick object
	LinePickVisitor* linePick = dynamic_cast<LinePickVisitor*>(pick);
	linePick->SetCurrentObject (this, frame);
	linePick->ClearHits();

	AxesFlags parallelAxis = m_View->GetCamera()->ParallelAxis(frame, HELIUM_CRITICAL_DOT_PRODUCT);

	if (m_Cube->Pick(linePick))
	{
		m_SelectedAxes = MultipleAxes::All;
	}
	else
	{
		m_SelectedAxes = m_Axes->PickAxis (frame, linePick->GetWorldSpaceLine(), m_XCube->GetBounds().maximum.Length());

		//
		// Prohibit picking a parallel axis
		//

		if (m_SelectedAxes != MultipleAxes::None)
		{
			if (parallelAxis != MultipleAxes::None)
			{
				switch (m_SelectedAxes)
				{
				case MultipleAxes::X:
					{
						if (parallelAxis == MultipleAxes::X)
						{
							m_SelectedAxes = MultipleAxes::None;
						}
						break;
					}

				case MultipleAxes::Y:
					{
						if (parallelAxis == MultipleAxes::Y)
						{
							m_SelectedAxes = MultipleAxes::None;
						}
						break;
					}

				case MultipleAxes::Z:
					{
						if (parallelAxis == MultipleAxes::Z)
						{
							m_SelectedAxes = MultipleAxes::None;
						}
						break;
					}

				default:
					break;
				}
			}
		}

		if (m_SelectedAxes == MultipleAxes::None)
		{
			linePick->SetCurrentObject (this, Matrix4 (m_XPosition) * frame);
			if (parallelAxis != MultipleAxes::X && m_XCube->Pick(linePick))
			{
				m_SelectedAxes = MultipleAxes::X;
			}
			else
			{
				linePick->SetCurrentObject (this, Matrix4 (m_YPosition) * frame);
				if (parallelAxis != MultipleAxes::Y && m_YCube->Pick(linePick))
				{
					m_SelectedAxes = MultipleAxes::Y;
				}
				else
				{
					linePick->SetCurrentObject (this, Matrix4 (m_ZPosition) * frame);
					if (parallelAxis != MultipleAxes::Z && m_ZCube->Pick(linePick))
					{
						m_SelectedAxes = MultipleAxes::Z;
					}
				}
			}
		}
	}

	// TODO: How to poll for ctrl button state? -geoff
	if (m_SelectedAxes != MultipleAxes::All && m_SelectedAxes != MultipleAxes::None && false /*wxIsCtrlDown()*/)
	{
		m_SelectedAxes = (AxesFlags)(~m_SelectedAxes & MultipleAxes::All);
	}

	if (m_SelectedAxes != MultipleAxes::None)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool ScaleManipulator::MouseDown( const MouseButtonInputEvent& e )
{
	AxesFlags previous = m_SelectedAxes;

	LinePickVisitor pick (m_View->GetCamera(), e.GetPosition().x, e.GetPosition().y);

	if (!Pick(&pick))
	{
		if (e.MiddleIsDown())
		{
			m_SelectedAxes = previous;
		}
		else
		{
			return false;
		}
	}

	if (!Base::MouseDown(e))
	{
		return false;
	}

	m_ManipulationStart.clear();

	std::vector< ScaleManipulatorAdapter* > set = CompleteSet<ScaleManipulatorAdapter>();
	for ( std::vector< ScaleManipulatorAdapter* >::const_iterator itr = set.begin(), end = set.end(); itr != end; ++itr )
	{
		ManipulationStart start;
		start.m_StartValue = Vector3 ((*itr)->GetValue().x, (*itr)->GetValue().y, (*itr)->GetValue().z);
		start.m_StartFrame = (*itr)->GetFrame(ManipulatorSpace::Object).Normalized();
		start.m_InverseStartFrame = start.m_StartFrame.Inverted();
		m_ManipulationStart.insert( M_ManipulationStart::value_type (*itr, start) );
	}

	return true;
}

void ScaleManipulator::MouseMove( const MouseMoveInputEvent& e )
{
	Base::MouseMove(e);

	ScaleManipulatorAdapter* primary = PrimaryObject<ScaleManipulatorAdapter>();

	if (primary == NULL || m_ManipulationStart.empty() || (!m_Left && !m_Middle && !m_Right))
	{
		return;
	}

	const ManipulationStart& primaryStart = m_ManipulationStart.find( primary )->second;

	bool uniform = false;
	Vector3 reference;

	Vector3 startPoint;
	primaryStart.m_StartFrame.TransformVertex(startPoint);


	//
	// Compute our reference vector in global space, from the object
	//  This is an axis normal (direction) for single axis manipulation, or a plane normal for multi-axis manipulation
	//

	// start out with global manipulator axes
	reference = GetAxesNormal(m_SelectedAxes);

	// use local axes to manipulate
	primaryStart.m_StartFrame.Transform(reference, 0.f);

	if (m_SelectedAxes == MultipleAxes::All)
	{
		uniform = true;
	}
	else if (reference == Vector3::Zero)
	{
		return;
	}

	int sX = m_StartX, sY = m_StartY, eX = e.GetPosition().x, eY = e.GetPosition().y;

	if (uniform)
	{
		// make +x scale up
		sX = -sX;
		eX = -eX;
	}

	// Pick ray from our starting location
	Line startRay;
	m_View->GetCamera()->ViewportToLine( (float32_t)sX, (float32_t)sY, startRay);

	// Pick ray from our current location
	Line endRay;
	m_View->GetCamera()->ViewportToLine( (float32_t)eX, (float32_t)eY, endRay);

	// start and end points of the drag in world space, on the line or on the plane
	Vector3 p1, p2;

	if (!uniform)
	{
		//
		// Linear insersections of the rays with the selected reference line
		//

		if (!startRay.IntersectsLine(startPoint, startPoint + reference, &p1))
		{
			return;
		}

		if (!endRay.IntersectsLine(startPoint, startPoint + reference, &p2))
		{
			return;
		}
	}
	else
	{
		//
		// Planar intersections of the rays with the selected reference plane
		//

		if (!startRay.IntersectsPlane(Plane (startPoint, reference), &p1))
		{
			return;
		}

		if (!endRay.IntersectsPlane(Plane (startPoint, reference), &p2))
		{
			return;
		}
	}

	// bring into transform space
	primary->GetNode()->GetTransform()->GetInverseGlobalTransform().TransformVertex(p1);
	primary->GetNode()->GetTransform()->GetInverseGlobalTransform().TransformVertex(p2);

	// account for pivot point
	p1 = p1 - primary->GetPivot();
	p2 = p2 - primary->GetPivot();

	// scaling factor is the scaling from our starting point p1 to our ending point p2
	float scaling = p2.Dot(p1) / p1.Dot(p1);

	// start with identity
	Scale result;

	// multiply out scaling in along selected axes
	switch (m_SelectedAxes)
	{
	case MultipleAxes::X:
		{
			result.x = scaling;
			break;
		}

	case MultipleAxes::Y:
		{
			result.y = scaling;
			break;
		}

	case MultipleAxes::Z:
		{
			result.z = scaling;
			break;
		}

	case MultipleAxes::XY:
		{
			result.x = scaling;
			result.y = scaling;
			break;
		}

	case MultipleAxes::YZ:
		{
			result.y = scaling;
			result.z = scaling;
			break;
		}

	case MultipleAxes::ZX:
		{
			result.z = scaling;
			result.x = scaling;
			break;
		}

	case MultipleAxes::All:
		{
			result.x = ((scaling - 1.0f) * 5.0f) + 1.0f;
			result.y = ((scaling - 1.0f) * 5.0f) + 1.0f;
			result.z = ((scaling - 1.0f) * 5.0f) + 1.0f;
			break;
		}

	default:
		break;
	}

	//
	// Set Value
	//

	std::vector< ScaleManipulatorAdapter* > set = CompleteSet<ScaleManipulatorAdapter>();
	for ( std::vector< ScaleManipulatorAdapter* >::const_iterator itr = set.begin(), end = set.end(); itr != end; ++itr )
	{
		const ManipulationStart& start = m_ManipulationStart.find( *itr )->second;

		Scale scale (start.m_StartValue.x * result.x, start.m_StartValue.y * result.y, start.m_StartValue.z * result.z);

		if ( m_GridSnap )
		{
			if ( m_SelectedAxes == MultipleAxes::X )
			{
				float32_t delta = result.x - start.m_StartValue.x;
				delta /= m_Distance;
				delta = Round( delta );
				delta *= m_Distance;

				scale.x = start.m_StartValue.x + delta;
				if ( fabs( scale.x ) < 0.0000001f )
				{
					return;
				}
			}
			else if ( m_SelectedAxes == MultipleAxes::Y )
			{
				float32_t delta = scale.y - start.m_StartValue.y;
				delta /= m_Distance;
				delta = Round( delta );
				delta *= m_Distance;

				scale.y = start.m_StartValue.y + delta;
				if ( fabs( scale.y ) < 0.0000001f )
				{
					return;
				}
			}
			else if ( m_SelectedAxes == MultipleAxes::Z )
			{
				float32_t delta = scale.z - start.m_StartValue.z;
				delta /= m_Distance;
				delta = Round( delta );
				delta *= m_Distance;

				scale.z = start.m_StartValue.z + delta;
				if ( fabs( scale.z ) < 0.0000001f )
				{
					return;
				}
			}
		}

		(*itr)->SetValue(scale);
	}

	// apply modification
	primary->GetNode()->GetOwner()->Execute(true);

	// flag as changed
	m_Manipulated = true;
}

void ScaleManipulator::CreateProperties()
{
	Base::CreateProperties();

	m_Generator->PushContainer( "Scale" );
	{
		m_Generator->PushContainer();
		{
			m_Generator->AddLabel( "Grid Snap" );
			m_Generator->AddCheckBox<bool>( new Helium::MemberProperty<Editor::ScaleManipulator, bool> (this, &ScaleManipulator::GetGridSnap, &ScaleManipulator::SetGridSnap) );
		}
		m_Generator->Pop();

		m_Generator->PushContainer();
		{
			m_Generator->AddLabel( "Grid Distance" );
			m_Generator->AddValue<float>( new Helium::MemberProperty<Editor::ScaleManipulator, float> (this, &ScaleManipulator::GetDistance, &ScaleManipulator::SetDistance) );
		}
		m_Generator->Pop();
	}
	m_Generator->Pop();
}

float32_t ScaleManipulator::GetSize() const
{
	return m_Size;
}

void ScaleManipulator::SetSize( float32_t size )
{
	m_Size = size;

	ManipulatorAdapter* primary = PrimaryObject<ManipulatorAdapter>();

	if (primary != NULL)
	{
		primary->GetNode()->GetOwner()->Execute(false);
	}

	SceneSettings* settings = m_SettingsManager->GetSettings< SceneSettings >();
	settings->RaiseChanged( settings->GetMetaClass()->FindField( &ScaleManipulator::m_Size ) );
}

bool ScaleManipulator::GetGridSnap() const
{
	return m_GridSnap;
}

void ScaleManipulator::SetGridSnap( bool gridSnap )
{
	m_GridSnap = gridSnap;

	SceneSettings* settings = m_SettingsManager->GetSettings< SceneSettings >();
	settings->RaiseChanged( settings->GetMetaClass()->FindField( &ScaleManipulator::m_GridSnap ) );
}

float ScaleManipulator::GetDistance() const
{
	return m_Distance;
}

void ScaleManipulator::SetDistance( float distance )
{
	m_Distance = distance;

	SceneSettings* settings = m_SettingsManager->GetSettings< SceneSettings >();
	settings->RaiseChanged( settings->GetMetaClass()->FindField( &ScaleManipulator::m_Distance ) );
}
