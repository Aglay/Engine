#pragma once

#include "Primitive.h"

namespace Helium
{
	namespace Editor
	{
		class PrimitiveLocator : public PrimitiveTemplate< Helium::SimpleVertex >
		{
		public:
			typedef PrimitiveTemplate< Helium::SimpleVertex > Base;

			float m_Length;

		public:
			PrimitiveLocator();

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