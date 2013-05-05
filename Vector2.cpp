#include "MathSimdPch.h"

#include "MathSimd/Vector2.h"
#include "Reflect/TranslatorDeduction.h"

REFLECT_DEFINE_BASE_STRUCTURE( Helium::Simd::Vector2 );

void Helium::Simd::Vector2::PopulateStructure( Reflect::Structure& comp )
{
#pragma TODO("Support static arrays in reflect")
    comp.AddField( &Vector2::m_x,       TXT( "m_x" ) );
    comp.AddField( &Vector2::m_y,       TXT( "m_y" ) );
}