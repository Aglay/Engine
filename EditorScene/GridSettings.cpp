#include "EditorScenePch.h"
#include "GridSettings.h"

#include "Reflect/TranslatorDeduction.h"

HELIUM_DEFINE_ENUM( Helium::Editor::GridUnit );
HELIUM_DEFINE_CLASS( Helium::Editor::GridSettings );

using namespace Helium;
using namespace Helium::Editor;

void GridSettings::PopulateMetaType( Reflect::MetaStruct& comp )
{
	comp.AddField( &GridSettings::m_Units, "Units" );
	comp.AddField( &GridSettings::m_Width, "Width" );
	comp.AddField( &GridSettings::m_Length, "Length" );
	comp.AddField( &GridSettings::m_MajorStep, "Major Step" );
	comp.AddField( &GridSettings::m_MinorStep, "Minor Step" );
	comp.AddField( &GridSettings::m_AxisColor, "Axis Color" );
	comp.AddField( &GridSettings::m_MajorColor, "Major Color" );
	comp.AddField( &GridSettings::m_MinorColor, "Minor Color" );
}

GridSettings::GridSettings( const std::string& version,
						   GridUnit units,
						   uint32_t width,
						   uint32_t length,
						   float32_t majorStep,
						   float32_t minorStep,
						   Color axisColor,
						   Color majorColor,
						   Color minorColor )
						   : m_Units( units )
						   , m_PreviousUnits( units )
						   , m_Width( width )
						   , m_Length( length )
						   , m_MajorStep( majorStep )
						   , m_MinorStep( minorStep )
						   , m_AxisColor( axisColor )
						   , m_MajorColor( majorColor )
						   , m_MinorColor( minorColor )
{
	e_Changed.Add( Reflect::ObjectChangeSignature::Delegate( this, &GridSettings::OnChanged ) );
}

GridSettings::~GridSettings()
{
	e_Changed.Remove( Reflect::ObjectChangeSignature::Delegate( this, &GridSettings::OnChanged ) );
}

void GridSettings::PostDeserialize()
{
	m_PreviousUnits = m_Units;
}

uint32_t GridSettings::GetWidth()
{
	return m_Width;
}

uint32_t GridSettings::GetLength()
{
	return m_Length;
}

float32_t GridSettings::GetMajorStep()
{
	return ConvertUnits( m_MajorStep, m_Units, GridUnit::Meters );
}

float32_t GridSettings::GetMinorStep()
{
	return ConvertUnits( m_MinorStep, m_Units, GridUnit::Meters );
}

const Color& GridSettings::GetAxisColor()
{
	return m_AxisColor;
}

const Color& GridSettings::GetMajorColor()
{
	return m_MajorColor;
}

const Color& GridSettings::GetMinorColor()
{
	return m_MinorColor;
}

void GridSettings::OnChanged( const Reflect::ObjectChangeArgs& args )
{
	if ( m_Units == m_PreviousUnits )
	{
		return;
	}

	m_MajorStep = ConvertUnits( m_MajorStep, m_PreviousUnits, m_Units );
	m_MinorStep = ConvertUnits( m_MinorStep, m_PreviousUnits, m_Units );
	m_PreviousUnits = m_Units;

	RaiseChanged();
}

float32_t GridSettings::GetConversionFactor( GridUnit units )
{
	switch ( units )
	{
	case GridUnit::Centimeters:
		return 0.01f;
	default:
		return 1.f;
	}
}

float32_t GridSettings::ConvertUnits( float32_t sourceValue, GridUnit sourceUnits, GridUnit destinationUnits )
{
	float32_t sourceConversion = GetConversionFactor( sourceUnits );
	float32_t destinationConversion = GetConversionFactor( destinationUnits );
	return sourceValue * sourceConversion / destinationConversion;
}
