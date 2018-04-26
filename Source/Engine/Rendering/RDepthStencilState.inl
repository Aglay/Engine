namespace Helium
{
    /// Constructor.
    ///
    /// Initializes the description to default state values.
    RDepthStencilState::Description::Description()
        : depthFunction( RENDERER_COMPARE_FUNCTION_LESS_EQUAL )
        , stencilFailOperation( RENDERER_STENCIL_OPERATION_KEEP )
        , stencilDepthFailOperation( RENDERER_STENCIL_OPERATION_KEEP )
        , stencilDepthPassOperation( RENDERER_STENCIL_OPERATION_KEEP )
        , stencilFunction( RENDERER_COMPARE_FUNCTION_ALWAYS )
        , stencilReadMask( 0xff )
        , stencilWriteMask( 0xff )
        , bDepthTestEnable( true )
        , bDepthWriteEnable( true )
        , bStencilTestEnable( false )
    {
    }
}
