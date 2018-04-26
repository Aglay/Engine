#pragma once

#include <set>

#include "Platform/Types.h"
#include "Platform/Assert.h"
#include "Foundation/SmartPtr.h"

#include "EditorScene/API.h"
#include "Render.h"

// this enables stream source and fvf transition printing
//#define SCENE_DEBUG_RESOURCES

namespace Helium
{
	HELIUM_DECLARE_RPTR( RIndexBuffer );
	HELIUM_DECLARE_RPTR( RVertexBuffer );

	namespace Editor
	{ 
		//
		// MetaEnum of compatible info types
		//

		namespace ResourceTypes
		{
			enum ResourceType
			{
				Index,
				Vertex,
			};
		}

		typedef ResourceTypes::ResourceType ResourceType;

		//
		// Populate arguments, for calling back into objects
		//

		struct PopulateArgs
		{
			PopulateArgs(ResourceType t, uint32_t o, uint8_t* b)
				: m_Type (t)
				, m_Buffer (b)
				, m_Offset (o)
			{

			}

			ResourceType m_Type;
			uint32_t m_Offset;
			uint8_t* m_Buffer;
		};
		typedef Helium::Signature< PopulateArgs*> PopulateSignature;

		//
		// Resource object
		//

		class Resource : public Reflect::Object
		{
		protected:
			// type
			ResourceType m_Type;

			// populator
			PopulateSignature::Delegate m_Populator;

			// state
			bool m_IsDirty;

		private:
			// state
			bool m_IsCreated;

			// flags
			bool m_IsDynamic;

			// data
			uint32_t m_BaseIndex;
			uint32_t m_ElementCount;

		public:
			Resource(ResourceType type)
				: m_Type ( type )
				, m_IsDirty ( true )
				, m_IsCreated ( false )
				, m_IsDynamic ( false )
				, m_BaseIndex ( 0 )
				, m_ElementCount ( 0 )
			{
			}

			virtual ~Resource()
			{
			}

			ResourceType GetResourceType()
			{
				return m_Type;
			}

			bool IsDynamic()
			{
				return m_IsDynamic;
			}

			void SetDynamic(bool dynamic)
			{
				m_IsDynamic = dynamic;
				m_IsDirty = true;
			}

			uint32_t GetBaseIndex() const
			{
				return m_BaseIndex;
			}

			uint32_t GetElementCount() const
			{
				return m_ElementCount;
			}
			void SetElementCount(uint32_t size)
			{
				m_ElementCount = size;
				m_IsDirty = true;
			}

			virtual uint32_t GetElementSize() const = 0;

			void SetPopulator( const PopulateSignature::Delegate& populator )
			{
				m_Populator = populator;
				m_IsDirty = true;
			}

			// create the actual resource data
			virtual void Create();

			// delete the actual resource data
			virtual void Delete();

			// reallocate and repopulate the data from the populator
			virtual void Update();

			// populate the allocated resource data from the populator delegate
			virtual void Populate();

		protected:
			// lock resource memory
			virtual uint8_t* Lock() = 0;

			// unlock resource memory
			virtual void Unlock() = 0;

			// do runtime allocation of resource
			virtual bool Allocate() = 0;

			// release runtime allocation
			virtual void Release() = 0;
		};

		class IndexResource : public Resource
		{
		public:
			IndexElementType m_ElementType;
			Helium::RIndexBufferPtr m_Buffer; 

			IndexResource();
			virtual ~IndexResource();

			IndexElementType GetElementType() const
			{
				return m_ElementType;
			}

			void SetElementType( IndexElementType type )
			{
				m_ElementType = type;
				m_IsDirty = true;
			}

			Helium::RIndexBuffer* GetBuffer() const
			{
				return m_Buffer; 
			}

			virtual uint32_t GetElementSize() const override;

			virtual uint8_t* Lock() override; 
			virtual void Unlock() override; 

		protected:
			virtual bool Allocate() override; 
			virtual void Release() override; 
		};

		class VertexResource : public Resource
		{
		public:
			VertexElementType m_ElementType;
			Helium::RVertexBufferPtr m_Buffer;

			VertexResource();
			virtual ~VertexResource();

			VertexElementType GetElementType() const
			{
				return m_ElementType;
			}

			void SetElementType( VertexElementType type )
			{
				m_ElementType = type;
				m_IsDirty = true;
			}

			Helium::RVertexBuffer* GetBuffer() const
			{
				return m_Buffer;
			}

			virtual uint32_t GetElementSize() const override;

			virtual uint8_t* Lock() override; 
			virtual void Unlock() override; 

		protected:
			virtual bool Allocate() override;
			virtual void Release() override;

			uint8_t* m_LockedVerts; 

		};
	}
}