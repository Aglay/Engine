#pragma once

#include "Math/Axes.h"
#include "Math/Vector3.h"

namespace Helium
{
	namespace Editor
	{
		const extern Axis SideAxis;
		const extern Vector3 SideVector;

		const extern Axis UpAxis;
		const extern Vector3 UpVector;

		const extern Axis OutAxis;
		const extern Vector3 OutVector;

		inline Vector3 SetupVector(float sideValue, float upValue, float outValue)
		{
			return (SideVector * sideValue) + (UpVector * upValue) + (OutVector * outValue);
		}
	}
}