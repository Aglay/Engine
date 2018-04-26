
#pragma once

#include "Reflect/MetaStruct.h"

#include "Framework/Components.h"
#include "Framework/ComponentDefinition.h"

namespace Helium
{
	class ComponentSet;
	class ParameterSet;
	class ComponentDefinition;

	namespace Components
	{
		// Construct a new set of components and apply them to the target. This isn't a member
		// function because I plan to support giving an array of definitions to combine, and possibly an
		// out parameter that includes extra name/component lookups, etc.
		void HELIUM_FRAMEWORK_API DeployComponents(IHasComponents &rHasComponents, const Helium::ComponentSet &components, const ParameterSet *parameters = NULL);
		void HELIUM_FRAMEWORK_API DeployComponents(IHasComponents &rHasComponents, const DynamicArray<ComponentDefinitionPtr> &components);
	}

	// Holds a set of definitions and allows them to construct and wire up together. Parameters can be provided, and the components themselves
	// can be components
	class HELIUM_FRAMEWORK_API ComponentSet : public Reflect::Struct
	{
	public:
		HELIUM_DECLARE_BASE_STRUCT(Helium::ComponentSet);
		static void PopulateMetaType( Reflect::MetaStruct& comp );

		// Add a component definition to list of definitions to construct
		void AddComponentDefinition( Helium::Name name, Helium::ComponentDefinition *pComponentDefinition );

		// Define a parameter that can be set via parameter set or a named component
		void ExposeParameter( Helium::Name paramName, Helium::Name componentName, Helium::Name fieldName );
		
		friend void Helium::Components::DeployComponents( 
			Components::IHasComponents &rHasComponents, 
			const Helium::ComponentSet &components, 
			const ParameterSet *parameters);

	private:

		struct NameDefinitionPair : Reflect::Struct
		{
			HELIUM_DECLARE_BASE_STRUCT( Helium::ComponentSet::NameDefinitionPair );
			static void PopulateMetaType( Reflect::MetaStruct& comp );
			
			inline bool operator==( const NameDefinitionPair& _rhs ) const;
			inline bool operator!=( const NameDefinitionPair& _rhs ) const;

			Name m_Name;
			Helium::StrongPtr<ComponentDefinition> m_Definition;
		};

		struct Parameter : Reflect::Struct
		{
			HELIUM_DECLARE_BASE_STRUCT( Helium::ComponentSet::Parameter );
			static void PopulateMetaType( Reflect::MetaStruct& comp );
			
			inline bool operator==( const Parameter& _rhs ) const;
			inline bool operator!=( const Parameter& _rhs ) const;

			Name m_ComponentName;
			Name m_ComponentFieldName;

			Name m_ParameterName;
		};

		DynamicArray<NameDefinitionPair> m_Components;
		DynamicArray<Parameter> m_Parameters;
	};
}
