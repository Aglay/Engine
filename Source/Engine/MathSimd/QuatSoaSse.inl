#if HELIUM_SIMD_SSE

/// Splat each component of the given quaternion across each SIMD vector for each component in this quaternion set.
///
/// @param[in] rQuat  Quaternion from which to set this quaternion.
void Helium::Simd::QuatSoa::Splat( const Quat& rQuat )
{
    Register quatVec = rQuat.GetSimdVector();
    m_x = _mm_shuffle_ps( quatVec, quatVec, _MM_SHUFFLE( 0, 0, 0, 0 ) );
    m_y = _mm_shuffle_ps( quatVec, quatVec, _MM_SHUFFLE( 1, 1, 1, 1 ) );
    m_z = _mm_shuffle_ps( quatVec, quatVec, _MM_SHUFFLE( 2, 2, 2, 2 ) );
    m_w = _mm_shuffle_ps( quatVec, quatVec, _MM_SHUFFLE( 3, 3, 3, 3 ) );
}

#endif  // HELIUM_SIMD_SSE
