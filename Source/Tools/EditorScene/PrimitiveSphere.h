#pragma once

#include "PrimitiveRadius.h"

namespace Helium
{
	namespace Editor
	{
		class PrimitiveSphere : public PrimitiveRadius
		{
		private:
			typedef PrimitiveRadius Base;

			mutable int32_t m_WireVertCount;
			mutable int32_t m_PolyVertCount;

		public:
			int32_t m_LengthSteps;

		public:
			PrimitiveSphere();

			int32_t GetWireVertCount() const;
			int32_t GetPolyVertCount() const;

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