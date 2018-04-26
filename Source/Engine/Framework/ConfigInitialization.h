#pragma once

#include "Framework/Framework.h"

namespace Helium
{
    /// Base class for initializing application configuration settings.
    class HELIUM_FRAMEWORK_API ConfigInitialization
    {
    public:
        /// @name Config Initialization
        //@{
        virtual void Startup() = 0;
        //@}
    };
}
