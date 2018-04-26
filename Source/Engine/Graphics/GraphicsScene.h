#pragma once

#include "Graphics/Graphics.h"
//#include "Engine/Asset.h"
#include "Reflect/Object.h"

#include "Foundation/BitArray.h"
#include "Rendering/RRenderResource.h"
#include "GraphicsTypes/GraphicsSceneObject.h"
#include "GraphicsTypes/GraphicsSceneView.h"

#if GRAPHICS_SCENE_BUFFERED_DRAWER
#include "Foundation/ObjectPool.h"
#include "Graphics/BufferedDrawer.h"
#endif // GRAPHICS_SCENE_BUFFERED_DRAWER

namespace Helium
{
    HELIUM_DECLARE_RPTR( RConstantBuffer );

    class HELIUM_GRAPHICS_API SceneObjectTransform : public Helium::Component
    {
        HELIUM_DECLARE_COMPONENT(Helium::SceneObjectTransform, Helium::Component);

        virtual void GraphicsSceneObjectUpdate(GraphicsScene *pScene) { }
    };

    /// Manager for a graphics scene.
    class HELIUM_GRAPHICS_API GraphicsScene : public Reflect::Object
    {
        HELIUM_DECLARE_CLASS( Helium::GraphicsScene, Reflect::Object );

    public:
        /// @name Construction/Destruction
        //@{
        GraphicsScene();
        virtual ~GraphicsScene();
        //@}

        /// @name Updating
        //@{
        virtual void Update( World *pWorld );
        //@}

        /// @name Scene View Management
        //@{
        uint32_t AllocateSceneView();
        void ReleaseSceneView( uint32_t id );
        inline GraphicsSceneView* GetSceneView( uint32_t id );

        void SetActiveSceneView( uint32_t id );
        //@}

        /// @name Scene Asset Allocation
        //@{
        size_t AllocateSceneObject();
        void ReleaseSceneObject( size_t id );
        inline GraphicsSceneObject* GetSceneObject( size_t id );
        //@}

        /// @name Scene Asset Sub-mesh Allocation
        //@{
        size_t AllocateSceneObjectSubMeshData( size_t sceneObjectId );
        void ReleaseSceneObjectSubMeshData( size_t id );
        inline GraphicsSceneObject::SubMeshData* GetSceneObjectSubMeshData( size_t id );
        //@}

        /// @name Lighting
        //@{
        void SetAmbientLight(
            const Color& rTopColor, float32_t topBrightness, const Color& rBottomColor, float32_t bottomBrightness );
        inline const Color& GetAmbientLightTopColor() const;
        inline float32_t GetAmbientLightTopBrightness() const;
        inline const Color& GetAmbientLightBottomColor() const;
        inline float32_t GetAmbientLightBottomBrightness() const;

        void SetDirectionalLight( const Simd::Vector3& rDirection, const Color& rColor, float32_t brightness );
        inline const Simd::Vector3& GetDirectionalLightDirection() const;
        inline const Color& GetDirectionalLightColor() const;
        inline float32_t GetDirectionalLightBrightness() const;
        //@}

#if GRAPHICS_SCENE_BUFFERED_DRAWER
        /// @name Buffered Drawing Support
        //@{
        inline BufferedDrawer& GetSceneBufferedDrawer();
        BufferedDrawer* GetSceneViewBufferedDrawer( uint32_t id );
        //@}
#endif // GRAPHICS_SCENE_BUFFERED_DRAWER

        /// @name Static Reserved Names
        //@{
        static Name GetDefaultSamplerStateName();
        static Name GetShadowSamplerStateName();
        static Name GetShadowMapTextureName();
        //@}

    private:
        /// Front-to-back sub-mesh sort comparison function
        class HELIUM_GRAPHICS_API SubMeshFrontToBackCompare
        {
        public:
            /// @name Construction/Destruction
            //@{
            SubMeshFrontToBackCompare();
            SubMeshFrontToBackCompare(
                const Simd::Vector3& rCameraDirection, const SparseArray< GraphicsSceneObject >& rSceneObjects,
                const SparseArray< GraphicsSceneObject::SubMeshData >& rSubMeshes );
            //@}

            /// @name Overloaded Operators
            //@{
            bool operator()( size_t subMeshIndex0, size_t subMeshIndex1 ) const;
            //@}

        private:
            /// Camera direction.
            Simd::Vector3 m_cameraDirection;
            /// Scene object list.
            const SparseArray< GraphicsSceneObject >* m_pSceneObjects;
            /// Scene object sub-mesh list.
            const SparseArray< GraphicsSceneObject::SubMeshData >* m_pSubMeshes;
        };

        /// Material-based sub-mesh sort comparison function
        class HELIUM_GRAPHICS_API SubMeshMaterialCompare
        {
        public:
            /// @name Construction/Destruction
            //@{
            SubMeshMaterialCompare();
            explicit SubMeshMaterialCompare( const SparseArray< GraphicsSceneObject::SubMeshData >& rSubMeshes );
            //@}

            /// @name Overloaded Operators
            //@{
            bool operator()( size_t subMeshIndex0, size_t subMeshIndex1 ) const;
            //@}

        private:
            /// Scene object sub-mesh list.
            const SparseArray< GraphicsSceneObject::SubMeshData >* m_pSubMeshes;
        };

        /// Scene view list.
        SparseArray< GraphicsSceneView > m_sceneViews;
        /// Scene object list.
        SparseArray< GraphicsSceneObject > m_sceneObjects;
        /// Scene object sub-data list.
        SparseArray< GraphicsSceneObject::SubMeshData > m_sceneObjectSubMeshes;

#if GRAPHICS_SCENE_BUFFERED_DRAWER
        /// Buffered drawing support for the entire scene (presented in all views).
        BufferedDrawer m_sceneBufferedDrawer;
        /// Pool of buffered drawing objects for various scene views.
        ObjectPool< BufferedDrawer > m_viewBufferedDrawerPool;
        /// Buffered drawing objects for each scene view.
        DynamicArray< BufferedDrawer* > m_viewBufferedDrawers;
#endif // GRAPHICS_SCENE_BUFFERED_DRAWER

        /// Visible scene objects for the current view.
        BitArray<> m_visibleSceneObjects;
        /// Scene object sub-data index list (for sorting during rendering).
        DynamicArray< size_t > m_sceneObjectSubMeshIndices;

        /// Ambient light top color.
        Color m_ambientLightTopColor;
        /// Ambient light top brightness.
        float32_t m_ambientLightTopBrightness;
        /// Ambient light bottom color.
        Color m_ambientLightBottomColor;
        /// Ambient light bottom brightness.
        float32_t m_ambientLightBottomBrightness;

        /// Directional light direction.
        Simd::Vector3 m_directionalLightDirection;
        /// Directional light color.
        Color m_directionalLightColor;
        /// Directional light brightness.
        float32_t m_directionalLightBrightness;

        /// ID of the currently active scene view.
        uint32_t m_activeViewId;

        /// Pre-computed shadow depth pass inverse view/projection matrices.
        DynamicArray< Simd::Matrix44 > m_shadowViewInverseViewProjectionMatrices;

        /// Per-view global vertex constant buffers.
        DynamicArray< RConstantBufferPtr > m_viewVertexGlobalDataBuffers[ 2 ];
        /// Per-view base-pass vertex constant buffers.
        DynamicArray< RConstantBufferPtr > m_viewVertexBasePassDataBuffers[ 2 ];
        /// Per-view screen-space vertex constant buffers.
        DynamicArray< RConstantBufferPtr > m_viewVertexScreenDataBuffers[ 2 ];

        /// Per-view base-pass pixel constant buffers.
        DynamicArray< RConstantBufferPtr > m_viewPixelBasePassDataBuffers[ 2 ];

        /// Per-view vertex constant buffers for shadow depth rendering.
        DynamicArray< RConstantBufferPtr > m_shadowViewVertexDataBuffers[ 2 ];

        /// Pool of per-instance vertex constant buffers for non-skinned meshes.
        DynamicArray< RConstantBufferPtr > m_staticInstanceVertexGlobalDataBufferPool[ 2 ];
        /// Pool of per-instance vertex constant buffers for skinned meshes.
        DynamicArray< RConstantBufferPtr > m_skinnedInstanceVertexGlobalDataBufferPool[ 2 ];

        /// Scene object global vertex constant buffers.
        DynamicArray< RConstantBuffer* > m_objectVertexGlobalDataBuffers;
        /// Mapped scene object global vertex constant buffer addresses.
        DynamicArray< float32_t* > m_mappedObjectVertexGlobalDataBuffers;

        /// Sub-mesh global vertex constant buffers.
        DynamicArray< RConstantBuffer* > m_subMeshVertexGlobalDataBuffers;
        /// Mapped sub-mesh global veretex constant buffer addresses.
        DynamicArray< float32_t* > m_mappedSubMeshVertexGlobalDataBuffers;

        /// Current dynamic constant buffer set index.
        size_t m_constantBufferSetIndex;

        /// @name Rendering
        //@{
        void UpdateShadowInverseViewProjectionMatrixSimple( size_t viewIndex );
        void UpdateShadowInverseViewProjectionMatrixLspsm( size_t viewIndex );

        void SwapDynamicConstantBuffers();

        void DrawSceneView( uint_fast32_t viewIndex );

        void DrawShadowDepthPass( uint_fast32_t viewIndex );
        void DrawDepthPrePass( uint_fast32_t viewIndex );
        void DrawBasePass( uint_fast32_t viewIndex );
        //@}

        /// @name Private Static Utility Functions
        //@{
        static Name GetNoneOptionName();

        static Name GetSkinningSysSelectName();
        static Name GetSkinningSmoothOptionName();
        static Name GetSkinningRigidOptionName();
        //@}
    };
}

#include "Graphics/GraphicsScene.inl"
