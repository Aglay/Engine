#include "Precompile.h"
#include "CurveControlPoint.h"

#include "EditorScene/Transform.h"
#include "EditorScene/PropertiesGenerator.h"

#include "Reflect/TranslatorDeduction.h"

HELIUM_DEFINE_CLASS( Helium::Editor::CurveControlPoint );

using namespace Helium;
using namespace Helium::Editor;

void CurveControlPoint::PopulateMetaType( Reflect::MetaStruct& comp )
{
	comp.AddField( &CurveControlPoint::m_Position, "m_Position" );
}

CurveControlPoint::CurveControlPoint() 
{
}

CurveControlPoint::~CurveControlPoint()
{
}

const Vector3& CurveControlPoint::GetPosition() const
{
	return m_Position;
}

void CurveControlPoint::SetPosition( const Vector3& value )
{
	m_Position = value;
	Dirty();
}

void CurveControlPoint::ConnectManipulator(ManiuplatorAdapterCollection *collection)
{
	switch( collection->GetMode() )
	{
	case ManipulatorModes::Translate:
		{
			collection->AddManipulatorAdapter( new CurveControlPointTranslateManipulatorAdapter( this ) );
			break;
		}

	default:
		{
			break;
		}
	}
}

bool CurveControlPoint::Pick( PickVisitor* pick )
{
	return pick->PickPoint( m_Position );
}

void CurveControlPoint::Evaluate( GraphDirection direction )
{
	Base::Evaluate(direction);

	switch (direction)
	{
	case GraphDirections::Downstream:
		{
			m_ObjectBounds.minimum = GetPosition() - Vector3 (1.f,1.f,1.f);
			m_ObjectBounds.maximum = GetPosition() + Vector3 (1.f,1.f,1.f);
			break;
		}
	case GraphDirections::Upstream:
		break;
	}
}

Matrix4 CurveControlPointTranslateManipulatorAdapter::GetFrame(ManipulatorSpace space)
{
	// base object manip frame
	Matrix4 m = m_Point->GetTransform()->GetGlobalTransform();

	// if we are pivoting then just use the value
	m = Matrix4 (GetValue()) * m;

	if (space == ManipulatorSpace::Object)
	{
		return m;
	}
	else
	{
		Matrix4 frame = Matrix4::Identity;

		frame.t.x = m.t.x;
		frame.t.y = m.t.y;
		frame.t.z = m.t.z;

		return frame;
	}
}

Matrix4 CurveControlPointTranslateManipulatorAdapter::GetObjectMatrix()
{
	return GetNode()->GetTransform()->GetGlobalTransform();
}

Matrix4 CurveControlPointTranslateManipulatorAdapter::GetParentMatrix()
{
	return GetNode()->GetTransform()->GetGlobalTransform();
}
