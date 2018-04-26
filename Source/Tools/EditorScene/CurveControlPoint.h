#pragma once

#include "HierarchyNode.h"

#include "EditorScene/Manipulator.h"

namespace Helium
{
	namespace Editor
	{
		class CurveControlPoint : public HierarchyNode
		{
		public:
			HELIUM_DECLARE_CLASS( CurveControlPoint, HierarchyNode );
			static void PopulateMetaType( Reflect::MetaStruct& comp );

		public:
			CurveControlPoint();
			~CurveControlPoint();

			const Vector3& GetPosition() const;
			void SetPosition( const Vector3& value );

			virtual void ConnectManipulator( ManiuplatorAdapterCollection* collection ) override;
			virtual bool Pick( PickVisitor* pick ) override;
			virtual void Evaluate( GraphDirection direction ) override;

		private:
			Vector3 m_Position;
		};

		typedef StrongPtr< CurveControlPoint > CurveControlPointPtr;

		class CurveControlPointTranslateManipulatorAdapter : public TranslateManipulatorAdapter
		{
		protected:
			CurveControlPoint* m_Point;

		public:
			CurveControlPointTranslateManipulatorAdapter( CurveControlPoint* point )
				: m_Point( point )
			{
				HELIUM_ASSERT( m_Point );
			}

			virtual HierarchyNode* GetNode() override
			{
				return m_Point;
			}

			virtual Matrix4 GetFrame(ManipulatorSpace space) override;
			virtual Matrix4 GetObjectMatrix() override;
			virtual Matrix4 GetParentMatrix() override;

			virtual Vector3 GetPivot() override
			{
				return m_Point->GetPosition();
			}

			virtual Vector3 GetValue() override
			{
				return m_Point->GetPosition();
			}

			virtual UndoCommandPtr SetValue( const Vector3& v ) override
			{
				return new PropertyUndoCommand<Vector3> ( new Helium::MemberProperty<CurveControlPoint, Vector3> (m_Point, &CurveControlPoint::GetPosition, &CurveControlPoint::SetPosition), v);
			}
		};

		typedef Helium::StrongPtr<CurveControlPoint> PointPtr;
		typedef std::vector<PointPtr> V_Point;
	}
}