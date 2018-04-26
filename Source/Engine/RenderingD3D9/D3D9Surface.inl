namespace Helium
{
    /// Get the Direct3D surface.
    ///
    /// @return  Direct3D surface instance.
    IDirect3DSurface9* D3D9Surface::GetD3DSurface() const
    {
        return m_pSurface;
    }

    /// Get whether gamma correction to sRGB color space should be applied when writing to this surface.
    ///
    /// @return  True if gamma correction to sRGB should be applied when writing to this surface, false if not.
    bool D3D9Surface::IsSrgb() const
    {
        return m_bSrgb;
    }
}
