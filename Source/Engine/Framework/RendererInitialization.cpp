#include "Precompile.h"
#include "Framework/RendererInitialization.h"

using namespace Helium;

/// Destructor.
RendererInitialization::~RendererInitialization()
{
}

/// @fn bool RendererInitialization::Initialize()
/// Create an initialize a new Renderer instance.
///
/// Note that a renderer is optional.  If no renderer is intended to be created, this will still return true
/// (success) even though the renderer instance is null.
///
/// @return  True if initialization was successful or no renderer was created intentionally, false if renderer
///          creation failed.
