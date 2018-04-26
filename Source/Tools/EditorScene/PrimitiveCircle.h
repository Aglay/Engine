#pragma once

#include "PrimitiveRadius.h"

namespace Helium
{
	namespace Editor
	{
		class PrimitiveCircle : public PrimitiveRadius
		{
		public:
			typedef PrimitiveRadius Base;

			bool m_HackyRotateFlag;

			PrimitiveCircle();

			virtual void Update() override;
			virtual void Draw(
				BufferedDrawer* drawInterface,
				Helium::Color materialColor = Colors::WHITE,
				const Simd::Matrix44& transform = Simd::Matrix44::IDENTITY,
				const bool* solid = NULL,
				const bool* transparent = NULL ) const override;
			virtual void DrawFill(
				BufferedDrawer* drawInterface,
				Helium::Color materialColor = Colors::WHITE,
				const Simd::Matrix44& transform = Simd::Matrix44::IDENTITY ) const;
			virtual void DrawHiddenBack( const Editor::Camera* camera, const Matrix4& m ) const;
			virtual bool Pick( PickVisitor* pick, const bool* solid = NULL ) const override;
		};
	}
}