#pragma once

#include "API.h"

#include "Common/Memory/SmartPtr.h"
#include "Reflect/Element.h"
#include "Common/File/Path.h"

namespace Luna
{
    class Project;
    typedef Nocturnal::SmartPtr< Project> ProjectPtr;

    class LUNA_EDITOR_API Project : public Reflect::Element
    {
    private:
        REFLECT_DECLARE_CLASS( Project, Reflect::Element );
        static void EnumerateClass( Reflect::Compositor< Project >& comp );

    public:
        Project();

    private:
        Nocturnal::S_Path m_Paths;

    public:

        const Nocturnal::S_Path& GetPaths() const;
    };
}