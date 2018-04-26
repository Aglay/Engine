#pragma once

#include "Primitive.h"

namespace Helium
{
	namespace Editor
	{
		class PrimitiveCube : public PrimitiveTemplate< Helium::SimpleVertex >
		{
		public:
			typedef PrimitiveTemplate< Helium::SimpleVertex > Base;

			PrimitiveCube();

			void SetRadius( float radius )
			{
				m_Bounds.minimum = Vector3 (-radius, -radius, -radius);
				m_Bounds.maximum = Vector3 (radius, radius, radius);
			}

			void ScaleRadius( float scale )
			{
				m_Bounds.minimum *= scale;
				m_Bounds.maximum *= scale;
			}

			void SetBounds( const AlignedBox& box )
			{
				m_Bounds = box;
			}

			void SetBounds( const Vector3& min, const Vector3& max )
			{
				m_Bounds.minimum = min;
				m_Bounds.maximum = max;
			}

			virtual void Update() override;
			virtual void Draw(
				BufferedDrawer* drawInterface,
				Helium::Color materialColor = Colors::WHITE,
				const Simd::Matrix44& transform = Simd::Matrix44::IDENTITY,
				const bool* solid = NULL,
				const bool* transparent = NULL ) const override;
			virtual bool Pick( PickVisitor* pick, const bool* solid = NULL ) const override;
		};
	}
}