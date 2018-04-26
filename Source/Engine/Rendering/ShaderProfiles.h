#pragma once

#include "Rendering/Rendering.h"

namespace Helium
{
    namespace ShaderProfile
    {
        /// PC shader profiles.
        enum EPc
        {
            PC_FIRST   =  0,
            PC_INVALID = -1,

            // Shader model 2.0b.
            PC_SM2b,
            // Shader model 3.0.
            PC_SM3,
            // Shader model 4.0.
            PC_SM4,

            PC_MAX,
            PC_LAST = PC_MAX - 1
        };
    }
}
