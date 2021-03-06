#pragma once

#include "Platform/System.h"

#if HELIUM_SHARED
# ifdef HELIUM_EDITOR_SCENE_EXPORTS
#  define HELIUM_EDITOR_SCENE_API HELIUM_API_EXPORT
# else
#  define HELIUM_EDITOR_SCENE_API HELIUM_API_IMPORT
# endif
#else
#define HELIUM_EDITOR_SCENE_API
#endif

#define EDITOR_SCENE_PROFILE 0

#if HELIUM_PROFILE_INSTRUMENT_ALL || EDITOR_SCENE_PROFILE
# define HELIUM_EDITOR_SCENE_SCOPE_TIMER( ... ) HELIUM_PROFILE_SCOPE_TIMER( __VA_ARGS__ )
#else
# define HELIUM_EDITOR_SCENE_SCOPE_TIMER( ... )
#endif

#define EDITOR_SCENE_PROFILE_EVALUATE 0

#if HELIUM_PROFILE_INSTRUMENT_ALL || EDITOR_SCENE_PROFILE_EVALUATE
# define HELIUM_EDITOR_SCENE_EVALUATE_SCOPE_TIMER( ... ) HELIUM_PROFILE_SCOPE_TIMER( __VA_ARGS__ )
#else
# define HELIUM_EDITOR_SCENE_EVALUATE_SCOPE_TIMER( ... )
#endif

#define EDITOR_SCENE_PROFILE_RENDER 0

#if HELIUM_PROFILE_INSTRUMENT_ALL || EDITOR_SCENE_PROFILE_RENDER
# define HELIUM_EDITOR_SCENE_RENDER_SCOPE_TIMER( ... ) HELIUM_PROFILE_SCOPE_TIMER( __VA_ARGS__ )
#else
# define HELIUM_EDITOR_SCENE_RENDER_SCOPE_TIMER( ... )
#endif
