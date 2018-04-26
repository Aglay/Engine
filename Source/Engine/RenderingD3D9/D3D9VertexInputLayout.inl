namespace Helium
{
    /// Get the Direct3D vertex declaration for this vertex input layout.
    ///
    /// @return  Direct3D vertex declaration.
    IDirect3DVertexDeclaration9* D3D9VertexInputLayout::GetD3DDeclaration() const
    {
        return m_pDeclaration;
    }
}
