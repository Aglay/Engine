#pragma once

#include "Primitive.h"

namespace Helium
{
	namespace Editor
	{
		class PrimitiveCapsule : public PrimitiveTemplate< Helium::SimpleVertex >
		{
		private:
			typedef PrimitiveTemplate< Helium::SimpleVertex > Base;

			mutable int32_t m_WireVertCount;
			mutable int32_t m_PolyVertCount;
			mutable int32_t m_CapVertCount;
			mutable int32_t m_ShaftVertCount;

		public:
			float32_t m_Radius;
			int32_t m_RadiusSteps;

			float32_t m_Length;
			int32_t m_LengthSteps;

		public:
			PrimitiveCapsule();

			int GetWireVertCount() const;
			int GetPolyVertCount() const;

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