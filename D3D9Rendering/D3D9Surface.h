//----------------------------------------------------------------------------------------------------------------------
// D3D9Surface.h
//
// Copyright (C) 2010 WhiteMoon Dreams, Inc.
// All Rights Reserved
//----------------------------------------------------------------------------------------------------------------------

#pragma once
#ifndef LUNAR_D3D9_RENDERING_D3D9_SURFACE_H
#define LUNAR_D3D9_RENDERING_D3D9_SURFACE_H

#include "D3D9Rendering/D3D9Rendering.h"
#include "Rendering/RSurface.h"

namespace Lunar
{
    /// Wrapper for a Direct3D 9 surface.
    class D3D9Surface : public RSurface
    {
    public:
        /// @name Construction/Destruction
        //@{
        D3D9Surface( IDirect3DSurface9* pD3DSurface, bool bSrgb );
        //@}

        /// @name Data Access
        //@{
        inline IDirect3DSurface9* GetD3DSurface() const;
        inline bool IsSrgb() const;
        //@}

    private:
        /// Reference to the Direct3D 9 surface.
        IDirect3DSurface9* m_pSurface;
        /// True if gamma correction to sRGB should be applied when writing to this surface.
        bool m_bSrgb;

        /// @name Construction/Destruction
        //@{
        ~D3D9Surface();
        //@}
    };
}

#include "D3D9Rendering/D3D9Surface.inl"

#endif  // LUNAR_D3D9_RENDERING_D3D9_SURFACE_H
