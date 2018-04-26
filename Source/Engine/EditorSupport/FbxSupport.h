#pragma once

#include "EditorSupport/EditorSupport.h"

#if HELIUM_TOOLS

#include "MathSimd/Matrix44.h"
#include "MathSimd/Quat.h"
#include "GraphicsTypes/VertexTypes.h"

#if HELIUM_CC_CL
#pragma warning( push )
#endif

#if HELIUM_CC_CLANG
#pragma clang push
#pragma clang diagnostic ignored "-Wnull-dereference"
#endif

#include <fbxsdk.h>

#if HELIUM_CC_CLANG
#pragma clang pop
#endif

#if HELIUM_CC_CL
#pragma warning( pop )
#endif

// Non-zero to enable wrapping of the FBX memory allocator with our own allocator (currently disabled due to
// long-standing bugs with the FBX SDK attempting to free allocations returned by CRT functions, i.e. strdup()).
#define HELIUM_ENABLE_FBX_MEMORY_ALLOCATOR 0

namespace Helium
{
#if HELIUM_ENABLE_FBX_MEMORY_ALLOCATOR
    /// Custom FBX memory allocator.
    class FbxMemoryAllocator : public FbxMemoryAllocator
    {
    public:
        /// @name Construction/Destruction
        //@{
        FbxMemoryAllocator();
        //@}

    protected:
        /// @name Memory Routines
        //@{
        static void* Malloc( size_t size );
        static void* Calloc( size_t count, size_t size );
        static void* Realloc( void* pMemory, size_t size );
        static void Free( void* pMemory );
        static size_t Msize( void* pMemory );
        static void* MallocDebug( size_t size, int, const char*, int );
        static void* CallocDebug( size_t count, size_t size, int, const char*, int );
        static void* ReallocDebug( void* pMemory, size_t size, int, const char*, int );
        static void FreeDebug( void* pMemory, int );
        static size_t MsizeDebug( void* pMemory, int );
        //@}
    };
#endif  // HELIUM_ENABLE_FBX_MEMORY_ALLOCATOR

    /// FBX SDK support.
    class FbxSupport : NonCopyable
    {
    public:
        /// Information about a given bone in the skeleton when building the mesh skeleton data.
        struct BoneData
        {
            /// Bone reference pose transform (relative to parent bone).
            Simd::Matrix44 referenceTransform;
            /// Inverse mesh-space transform for the bone in its reference pose.
            Simd::Matrix44 inverseWorldTransform;
            /// Bone name.
            Name name;
            /// Parent bone index (invalid index if this is a root bone).
            uint8_t parentIndex;
        };

        /// Per-vertex skinning information.
        struct BlendData
        {
            /// Blend weights.
            float32_t weights[ 4 ];
            /// Blend indices.
            uint8_t indices[ 4 ];
        };

        /// Animation key frame data.
        struct Key
        {
            /// Translation data.
            Simd::Vector3 translation;
            /// Rotation data.
            Simd::Quat rotation;
            /// Scaling data.
            Simd::Vector3 scale;
        };

        /// Animation track information.
        struct AnimTrackData
        {
            /// Track name.
            Name name;
            /// Bone transform key frames.
            DynamicArray< Key > keys;
        };

        /// @name Access Reference Counting
        //@{
        void Release();
        //@}

        /// @name Resource Loading
        //@{
        bool LoadMesh(
            const String& rSourceFilePath, DynamicArray< StaticMeshVertex< 1 > >& rVertices, DynamicArray< uint16_t >& rIndices,
            DynamicArray< uint16_t >& rSectionVertexCounts, DynamicArray< uint32_t >& rSectionTriangleCounts,
            DynamicArray< BoneData >& rBones, DynamicArray< BlendData >& rVertexBlendData,
            DynamicArray< uint8_t >& rSkinningPaletteMap, bool bStripNamespaces = true );
        bool LoadAnimation(
            const String& rSourceFilePath, uint8_t oversampling, DynamicArray< AnimTrackData >& rTracks,
            uint_fast32_t& rSamplesPerSecond, bool bStripNamespaces = true );
        //@}

        /// @name Static Access
        //@{
        static FbxSupport& StaticAcquire();
        //@}

    private:
        /// Information about a given bone in the skeleton relevant only while building the skinning data during mesh
        /// loading.
        struct WorkingBoneData
        {
            /// Scene node for the skeleton bone.
            const FbxNode* pNode;
            /// True if the node transform is in space relative to its parent bone, false if it is in global space and
            /// needs to be converted.
            bool bParentRelative;
        };

        /// Information about a given animation track relevant only during animation loading.
        struct WorkingTrackData
        {
            /// Model-space bone transform data for a single key frame.
            FbxMatrix modelSpaceTransform;
            /// Scene node for the skeleton bone.
            FbxNode* pNode;
            /// Parent bone index (invalid index if this is a root bone).
            uint8_t parentIndex;
        };

        /// FBX SDK manager instance.
        FbxManager* m_pSdkManager;
        /// IO settings instance.
        FbxIOSettings* m_pIoSettings;
        /// Import handler.
        FbxImporter* m_pImporter;

#if HELIUM_ENABLE_FBX_MEMORY_ALLOCATOR
        /// Memory allocation handler.
        FbxMemoryAllocator m_memoryAllocator;
#endif  // HELIUM_ENABLE_FBX_MEMORY_ALLOCATOR

        /// Reference count.
        volatile int32_t m_referenceCount;

        /// Singleton instance.
        static FbxSupport* sm_pInstance;

        /// @name Construction/Destruction
        //@{
        FbxSupport();
        ~FbxSupport();
        //@}

        /// @name Private Utility Functions
        //@{
        void LazyInitialize();

        void BuildSkinningInformation(
            FbxScene* pScene, FbxMesh* pMesh, FbxNode* pSkeletonRootNode,
            const DynamicArray< int >& rControlPointIndices, const DynamicArray< uint16_t >& rSectionVertexCounts,
            DynamicArray< BoneData >& rBones, DynamicArray< BlendData >& rVertexBlendData,
            DynamicArray< uint8_t >& rSkinningPaletteMap, bool bStripNamespaces );

        void RecursiveAddMeshSkeletonData(
            const FbxNode* pCurrentBoneNode, uint8_t parentBoneIndex, DynamicArray< BoneData >& rBones,
            DynamicArray< WorkingBoneData >& rWorkingBones, bool bStripNamespaces );

        void RecursiveAddAnimationSkeletonData(
            FbxNode* pCurrentBoneNode, uint8_t parentTrackIndex, DynamicArray< AnimTrackData >& rTracks,
            DynamicArray< WorkingTrackData >& rWorkingTracks, bool bStripNamespaces );

        bool BuildMeshFromScene(
            FbxScene* pScene, DynamicArray< StaticMeshVertex< 1 > >& rVertices, DynamicArray< uint16_t >& rIndices,
            DynamicArray< uint16_t >& rSectionVertexCounts, DynamicArray< uint32_t >& rSectionTriangleCounts,
            DynamicArray< BoneData >& rBones, DynamicArray< BlendData >& rVertexBlendData,
            DynamicArray< uint8_t >& rSkinningPaletteMap, bool bStripNamespaces );
        bool BuildAnimationFromScene(
            FbxScene* pScene, uint_fast32_t oversampling, DynamicArray< AnimTrackData >& rTracks,
            uint_fast32_t& rSamplesPerSecond, bool bStripNamespaces );
        //@}

        /// @name Static Private Utility Functions
        //@{
        static const char* StripNamespace( const char* pString );
        //@}
    };
}

#endif  // HELIUM_TOOLS
