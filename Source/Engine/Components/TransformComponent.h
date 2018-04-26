#pragma once

#include "Components/Components.h"
#include "MathSimd/Vector3.h"
#include "MathSimd/Quat.h"
#include "MathSimd/Matrix44.h"
#include "Framework/ComponentDefinition.h"
#include "Framework/TaskScheduler.h"

namespace Helium
{
	class TransformComponentDefinition;

	class HELIUM_COMPONENTS_API TransformComponent : public Component
	{
		HELIUM_DECLARE_COMPONENT( Helium::TransformComponent, Helium::Component );
		static void PopulateMetaType( Reflect::MetaStruct& comp );

		void Initialize( const TransformComponentDefinition &definition );
				
		inline const Simd::Vector3& GetPosition() const { return m_Position; }
		virtual void SetPosition( const Simd::Vector3& rPosition ) { m_Position = rPosition; m_bDirty = true; }

		inline const Simd::Quat& GetRotation() const { return m_Rotation; }
		virtual void SetRotation( const Simd::Quat& rRotation ) { m_Rotation = rRotation; m_bDirty = true; }

		inline float32_t GetScale() const { return m_Scale; }
		virtual void SetScale( float32_t scale ) { m_Scale = scale; }

		bool IsDirty() const { return m_bDirty; }
		void ClearDirtyFlag() { m_bDirty = false; }

		Simd::Vector3 m_Position;
		Simd::Quat m_Rotation;
		float32_t m_Scale;
		bool m_bDirty;
	};
	typedef Helium::ComponentPtr<TransformComponent> TransformComponentPtr;
		
	class HELIUM_COMPONENTS_API TransformComponentDefinition : public Helium::ComponentDefinitionHelper<TransformComponent, TransformComponentDefinition>
	{
		TransformComponentDefinition();

		HELIUM_DECLARE_CLASS( Helium::TransformComponentDefinition, Helium::ComponentDefinition );
		static void PopulateMetaType( Reflect::MetaStruct& comp );
		
		inline const Simd::Vector3& GetPosition() const { return m_Position; }
		virtual void SetPosition( const Simd::Vector3& rPosition ) { m_Position = rPosition; }

		inline const Simd::Quat& GetRotation() const { return m_Rotation; }
		virtual void SetRotation( const Simd::Quat& rRotation ) { m_Rotation = rRotation; }

		inline float32_t GetScale() const { return m_Scale; }
		virtual void SetScale( float32_t scale ) { m_Scale = scale; }

		Simd::Vector3 m_Position;
		Simd::Quat m_Rotation;
		float32_t m_Scale;
	};
	typedef StrongPtr<TransformComponentDefinition> TransformComponentDefinitionPtr;

	struct HELIUM_COMPONENTS_API ClearTransformComponentDirtyFlagsTask : public TaskDefinition
	{
		HELIUM_DECLARE_TASK(ClearTransformComponentDirtyFlagsTask);
		virtual void DefineContract(TaskContract &rContract);
	};
}
