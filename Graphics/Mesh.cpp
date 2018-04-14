#include "GraphicsPch.h"
#include "Graphics/Mesh.h"

#include "Engine/AsyncLoader.h"
#include "MathSimd/Matrix44.h"
#include "Engine/CacheManager.h"
#include "Rendering/RIndexBuffer.h"
#include "Rendering/Renderer.h"
#include "Rendering/RVertexBuffer.h"
#include "Reflect/TranslatorDeduction.h"

#if HELIUM_USE_GRANNY_ANIMATION
#include "GrannyMeshInterface.cpp.inl"
#endif

HELIUM_IMPLEMENT_ASSET( Helium::Mesh, Graphics, AssetType::FLAG_NO_TEMPLATE );
HELIUM_DEFINE_CLASS( Helium::Mesh::PersistentResourceData );

using namespace Helium;

/// Constructor.
Mesh::Mesh()
: m_vertexBufferLoadId( Invalid< size_t >() )
, m_indexBufferLoadId( Invalid< size_t >() )
{
}

/// Destructor.
Mesh::~Mesh()
{
    HELIUM_ASSERT( !m_spVertexBuffer );
    HELIUM_ASSERT( !m_spIndexBuffer );
    HELIUM_ASSERT( IsInvalid( m_vertexBufferLoadId ) );
    HELIUM_ASSERT( IsInvalid( m_indexBufferLoadId ) );
}

/// @copydoc Asset::PreDestroy()
void Mesh::RefCountPreDestroy()
{
    HELIUM_ASSERT( IsInvalid( m_vertexBufferLoadId ) );
    HELIUM_ASSERT( IsInvalid( m_indexBufferLoadId ) );

    m_spVertexBuffer.Release();
    m_spIndexBuffer.Release();

    Base::RefCountPreDestroy();
}

void Mesh::PopulateMetaType(Reflect::MetaStruct& comp)
{
    comp.AddField(&Mesh::m_materials, "m_materials");
}

/// @copydoc Asset::NeedsPrecacheResourceData()
bool Mesh::NeedsPrecacheResourceData() const
{
    return true;
}

/// @copydoc Asset::BeginPrecacheResourceData()
bool Mesh::BeginPrecacheResourceData()
{
    HELIUM_ASSERT( IsInvalid( m_vertexBufferLoadId ) );
    HELIUM_ASSERT( IsInvalid( m_indexBufferLoadId ) );

    Renderer* pRenderer = Renderer::GetInstance();
    if( !pRenderer )
    {
        return true;
    }

    if( m_persistentResourceData.m_vertexCount != 0 )
    {
        size_t vertexDataSize = GetSubDataSize( 0 );
        if( IsInvalid( vertexDataSize ) )
        {
            HELIUM_TRACE(
                TraceLevels::Error,
                "Mesh::BeginPrecacheResourceData(): Failed to locate cached vertex buffer data for mesh \"%s\".\n",
                *GetPath().ToString() );
        }
        else
        {
            m_spVertexBuffer = pRenderer->CreateVertexBuffer( vertexDataSize, RENDERER_BUFFER_USAGE_STATIC );
            if( !m_spVertexBuffer )
            {
                HELIUM_TRACE(
                    TraceLevels::Error,
                    "Mesh::BeginPrecacheResourceData(): Failed to create a vertex buffer of %" PRIuSZ " bytes for mesh \"%s\".\n",
                    vertexDataSize,
                    *GetPath().ToString() );
            }
            else
            {
                void* pData = m_spVertexBuffer->Map();
                if( !pData )
                {
                    HELIUM_TRACE(
                        TraceLevels::Error,
                        "Mesh::BeginPrecacheResourceData(): Failed to map vertex buffer for loading for mesh \"%s\".\n",
                        *GetPath().ToString() );

                    m_spVertexBuffer.Release();
                }
                else
                {
                    m_vertexBufferLoadId = BeginLoadSubData( pData, 0 );
                    if( IsInvalid( m_vertexBufferLoadId ) )
                    {
                        HELIUM_TRACE(
                            TraceLevels::Error,
                            "Mesh::BeginPrecacheResourceData(): Failed to queue async load request for vertex buffer data for mesh \"%s\".\n",
                            *GetPath().ToString() );

                        m_spVertexBuffer->Unmap();
                        m_spVertexBuffer.Release();
                    }
                }
            }
        }
    }

    if( m_persistentResourceData.m_triangleCount != 0 )
    {
        size_t indexDataSize = GetSubDataSize( 1 );
        if( IsInvalid( indexDataSize ) )
        {
            HELIUM_TRACE(
                TraceLevels::Error,
                "Mesh::BeginPrecacheResourceData(): Failed to locate cached index buffer data for mesh \"%s\".\n",
                *GetPath().ToString() );
        }
        else
        {
            m_spIndexBuffer = pRenderer->CreateIndexBuffer(
                indexDataSize,
                RENDERER_BUFFER_USAGE_STATIC,
                RENDERER_INDEX_FORMAT_UINT16 );
            if( !m_spIndexBuffer )
            {
                HELIUM_TRACE(
                    TraceLevels::Error,
                    "Mesh::BeginPrecacheResourceData(): Failed to create an index buffer of %" PRIuSZ " bytes for mesh \"%s\".\n",
                    indexDataSize,
                    *GetPath().ToString() );
            }
            else
            {
                void* pData = m_spIndexBuffer->Map();
                if( !pData )
                {
                    HELIUM_TRACE(
                        TraceLevels::Error,
                        "Mesh::BeginPrecacheResourceData(): Failed to map index buffer for loading for mesh \"%s\".\n",
                        *GetPath().ToString() );

                    m_spIndexBuffer.Release();
                }
                else
                {
                    m_indexBufferLoadId = BeginLoadSubData( pData, 1 );
                    if( IsInvalid( m_indexBufferLoadId ) )
                    {
                        HELIUM_TRACE(
                            TraceLevels::Error,
                            "Mesh::BeginPrecacheResourceData(): Failed to queue async load request for index buffer data for mesh \"%s\".\n",
                            *GetPath().ToString() );

                        m_spIndexBuffer->Unmap();
                        m_spIndexBuffer.Release();
                    }
                }
            }
        }
    }

    return true;
}

/// @copydoc Asset::TryFinishPrecacheResourceData()
bool Mesh::TryFinishPrecacheResourceData()
{
    if( IsValid( m_vertexBufferLoadId ) )
    {
        if( !TryFinishLoadSubData( m_vertexBufferLoadId ) )
        {
            return false;
        }

        SetInvalid( m_vertexBufferLoadId );

        HELIUM_ASSERT( m_spVertexBuffer );
        m_spVertexBuffer->Unmap();
    }

    if( IsValid( m_indexBufferLoadId ) )
    {
        if( !TryFinishLoadSubData( m_indexBufferLoadId ) )
        {
            return false;
        }

        SetInvalid( m_indexBufferLoadId );

        HELIUM_ASSERT( m_spIndexBuffer );
        m_spIndexBuffer->Unmap();
    }

    return true;
}

Mesh::PersistentResourceData::PersistentResourceData()
: m_vertexCount( 0 )
, m_triangleCount( 0 )
#if !HELIUM_USE_GRANNY_ANIMATION
, m_boneCount( 0 )
#endif
{

}

void Mesh::PersistentResourceData::PopulateMetaType( Reflect::MetaStruct& comp )
{
    comp.AddField( &PersistentResourceData::m_sectionVertexCounts,      "m_sectionVertexCounts" );
    comp.AddField( &PersistentResourceData::m_sectionTriangleCounts,    "m_sectionTriangleCounts" );
    comp.AddField( &PersistentResourceData::m_skinningPaletteMap,       "m_skinningPaletteMap" );
    comp.AddField( &PersistentResourceData::m_vertexCount,              "m_vertexCount" );
    comp.AddField( &PersistentResourceData::m_triangleCount,            "m_triangleCount" );
    comp.AddField( &PersistentResourceData::m_bounds,                   "m_bounds" );
#if !HELIUM_USE_GRANNY_ANIMATION
    comp.AddField( &PersistentResourceData::m_boneCount,                "m_boneCount" );
    comp.AddField( &PersistentResourceData::m_pBoneNames,               "m_pBoneNames" );
    comp.AddField( &PersistentResourceData::m_pParentBoneIndices,       "m_pParentBoneIndices" );
    comp.AddField( &PersistentResourceData::m_pReferencePose,           "m_pReferencePose" );
#endif
}

bool Helium::Mesh::LoadPersistentResourceObject( Reflect::ObjectPtr &_object )
{    
    m_spVertexBuffer.Release();
    m_spIndexBuffer.Release();

    HELIUM_ASSERT(_object.ReferencesObject());
    if (!_object.ReferencesObject())
    {
        return false;
    }

    _object->CopyTo(&m_persistentResourceData);

    return true;
}

/// @copydoc Resource::GetCacheName()
Name Mesh::GetCacheName() const
{
    static Name cacheName( "Mesh" );

    return cacheName;
}


/// Get the GPU skinning palette map for a specific mesh section.
///
/// The skinning palette map provides the indices within the bone palette passed to a shader for each bone defined
/// in the main mesh.  Invalid index values are used to signify bones that do not directly influence any bones in
/// the mesh section, and as such do not need to be provided to the GPU.
///
/// @param[in] sectionIndex  Mesh section index.
///
/// @return  Pointer to the GPU skinning palette map for the section associated with the specified index.
///
/// @see GetSectionVertexCount(), GetSectionTriangleCount(), GetSectionCount()
const uint8_t* Mesh::GetSectionSkinningPaletteMap( size_t sectionIndex ) const
{
    HELIUM_ASSERT( sectionIndex < m_persistentResourceData.m_sectionTriangleCounts.GetSize() );

#if HELIUM_USE_GRANNY_ANIMATION
    size_t boneCount = m_grannyData.GetBoneCount();
#else
    size_t boneCount = m_persistentResourceData.m_boneCount;
#endif

    return m_persistentResourceData.m_skinningPaletteMap.GetData() + sectionIndex * boneCount;
}
