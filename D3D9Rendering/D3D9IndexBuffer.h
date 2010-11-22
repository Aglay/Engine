//----------------------------------------------------------------------------------------------------------------------
// D3D9IndexBuffer.h
//
// Copyright (C) 2010 WhiteMoon Dreams, Inc.
// All Rights Reserved
//----------------------------------------------------------------------------------------------------------------------

#pragma once
#ifndef LUNAR_D3D9_RENDERING_D3D9_INDEX_BUFFER_H
#define LUNAR_D3D9_RENDERING_D3D9_INDEX_BUFFER_H

#include "D3D9Rendering/D3D9Rendering.h"
#include "Rendering/RIndexBuffer.h"

namespace Lunar
{
    /// Direct3D 9 index buffer implementation.
    class D3D9IndexBuffer : public RIndexBuffer
    {
    public:
        /// @name Construction/Destruction
        //@{
        D3D9IndexBuffer( IDirect3DIndexBuffer9* pD3DBuffer );
        //@}

        /// @name Data Access
        //@{
        void* Map( ERendererBufferMapHint hint );
        void Unmap();

        inline IDirect3DIndexBuffer9* GetD3DBuffer() const;
        //@}

    private:
        /// Vertex buffer instance.
        IDirect3DIndexBuffer9* m_pBuffer;

        /// @name Construction/Destruction
        //@{
        ~D3D9IndexBuffer();
        //@}
    };
}

#include "D3D9Rendering/D3D9IndexBuffer.inl"

#endif  // LUNAR_D3D9_RENDERING_D3D9_INDEX_BUFFER_H
