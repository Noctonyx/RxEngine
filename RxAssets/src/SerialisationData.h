#pragma once

#include <vector>
#include <array>
#include "tser.hpp"
//#include <bitsery/bitsery.h>
//#include <bitsery/traits/vector.h>
//#include <bitsery/traits/string.h>
#include "DirectXMath.h"
//#include "glm/vec2.hpp"
//#include "glm/vec3.hpp"
//#include "vulkan/vulkan.h"

#if 0
namespace bitsery
{
    template<typename S>
    void serialize(S & s, DirectX::XMFLOAT3 & vec)
    {
        s.value4b(vec.x);
        s.value4b(vec.y);
        s.value4b(vec.z);
    }

    template<typename S>
    void serialize(S & s, DirectX::XMFLOAT2 & vec)
    {
        s.value4b(vec.x);
        s.value4b(vec.y);
    }
}
#endif

namespace RxAssets
{
#if 0
    struct AssetIndex
    {
        //uint32_t indexCount;
        struct IndexEntry
        {
            std::string asset;
            uint32_t size;
            uint32_t offset;
        };
        std::vector<IndexEntry> index;

        template<typename S>
        void serialize(S & s)
        {
            s.container(
                index, std::numeric_limits<uint32_t>::max(),
                [](S & s, IndexEntry & v)
                {
                    s.text<1>(v.asset, 200);
                    //s.value4b(v.hash);
                    s.value4b(v.size);
                    s.value4b(v.offset);
                });
        }
    };
#endif
    struct MeshPrimitive
    {
        DEFINE_SERIALIZABLE(MeshPrimitive, firstIndex, indexCount, materialIndex, materialName)
            uint32_t firstIndex = 0;
        uint32_t indexCount = 0;
        int32_t materialIndex = 0;
        std::string materialName{};

        //MeshPrimitive() = default;
    };

    struct MeshSaveVertex
    {
        DEFINE_SERIALIZABLE(MeshSaveVertex, x, y, z, nx, ny, nz, uvx, uvy)
            float x, y, z;
        float nx, ny, nz;
        float uvx, uvy;
        //            DirectX::XMFLOAT3 vertex;
        ///float pad1;
        //DirectX::XMFLOAT3 normal;
        //float pad2;
        //DirectX::XMFLOAT2 uvs;
        //float pad3;
        //float pad4;
    };

    struct MeshSaveData
    {
        DEFINE_SERIALIZABLE(MeshSaveData, vertices, indices, minpx, minpy, minpz, maxpx, maxpy, maxpz, materials, primitives )
        //uint32_t vertexCount;
        std::vector<MeshSaveVertex> vertices;
        std::vector<uint32_t> indices;
        float minpx, minpy, minpz;
        float maxpx, maxpy, maxpz;

        std::vector<std::string> materials{};
        std::vector<MeshPrimitive> primitives{};

        //MeshData() = default;
        //MeshData(MeshData && other) = default;
        //MeshData(const MeshData & other) = default;
#if 0
        template <typename S>
        void serialize(S & s)
        {
            //s.value2b(version);
            //s.value4b(section.vertexCount);
            s.object(minp);
            s.object(maxp);
            s.container(
                vertices, std::numeric_limits<uint32_t>::max(),
                [](S & s, Vertex & v)
                {
                    s.object(v.vertex);
                    s.object(v.normal);
                    s.object(v.uvs);
                });
            s.container(materials, 10, [](S & s, std::string & st)
            {
                s.text < 1 > (st, 150);
            });
            //s.value4b(section.indexCount);
            s.container < 4 > (indices, std::numeric_limits<uint32_t>::max());
            s.container(
                primitives, 10, [](S & s, MeshPrimitive & p)
                {
                    s.value4b(p.firstIndex);
                    s.value4b(p.indexCount);
                    s.value4b(p.materialIndex);
                    s.text < 1 > (p.materialName, 100);
                });
        }
#endif
    };

    struct MaterialData
    {
        std::string colorTextureAssetName;
        std::string emissionTextureAssetName;
        std::string normalTextureAssetName;
        std::string vertexShader;
        std::string depthVertexShader;
        std::string fragmentShader;
        std::string depthFragmentShader;
        std::string transparency;

        float metallicValue;
        float roughnessValue;
        std::string name;
#if 0
        template<typename S>
        void serialize(S & s)
        {
            s.text<1>(colorTextureAssetName, 100);
            s.text<1>(emissionTextureAssetName, 100);
            s.text<1>(vertexShader, 100);
            s.text<1>(fragmentShader, 100);
            s.text<1>(depthVertexShader, 100);
            s.text<1>(depthFragmentShader, 100);
            s.value4b(metallicValue);
            s.value4b(roughnessValue);
        }
#endif
    };

    enum ImageType : uint8_t
    {
        eBitmap = 0,
        eBC7 = 1
    };

    struct ImageData
    {
        struct MipLevel
        {
            std::vector<uint8_t> bytes;
            uint16_t width;
            uint16_t height;

#if 0
            template<typename S>
            void serialize(S & s)
            {
                s.container<1>(bytes, std::numeric_limits<uint32_t>::max());
                s.value2b(width);
                s.value2b(height);
            }
#endif
        };

        //uint8_t formatType;
        uint16_t width;
        uint16_t height;
        ImageType imType;
        std::string name;
        uint8_t mips;
        std::vector<MipLevel> mipLevels;

#if 0
        template<typename S>
        void serialize(S & s)
        {
//            s.value1b(formatType);
            s.value2b(width);
            s.value2b(height);
            s.value1b(imType);
            s.text<1>(name, 100);
            s.value1b(mips);
            s.container(mipLevels, 20);
        }
#endif
    };

    struct ShaderData
    {
        std::vector<uint32_t> bytes;
#if 0
        template<typename S>
        void serialize(S & s)
        {
            s.container<4>(bytes, std::numeric_limits<uint32_t>::max());
        }
#endif
    };

    struct SamplerData
    {
        uint8_t minFilter;
        uint8_t magFilter;

        uint8_t mipMapMode;

        uint8_t addressU;
        uint8_t addressV;
        uint8_t addressW;

        float mipLodBias;

        bool anisotropy;
        float maxAnisotropy;

        float minLod;
        float maxLod;

        uint8_t borderColor;
#if 0
        template<typename S>
        void serialize(S & s)
        {
            s.value1b(minFilter);
            s.value1b(magFilter);
            s.value1b(mipMapMode);
            s.value1b(addressU);
            s.value1b(addressV);
            s.value1b(addressW);
            s.value4b(mipLodBias);
            s.value1b(anisotropy);
            s.value4b(maxAnisotropy);
            s.value4b(minLod);
            s.value4b(maxLod);
            s.value1b(borderColor);
        }
#endif
    };

    struct TextureData
    {
        std::string imageName;
        SamplerData sampler;
#if 0
        template<typename S>
        void serialize(S & s)
        {
            s.text<1>(imageName, 100);
            s.object(sampler);
        }
#endif
    };

    struct MaterialData2
    {
        std::string materialBaseName;
        std::array<float, 4> val1;
        std::array<float, 4> val2;
        std::array<float, 4> val3;
        std::array<float, 4> val4;
    };

    struct MaterialBaseData
    {
        std::string opaquePipelineName;
        std::string shadowPipelineName;
        std::string transparentPipelineName;
        std::string texture1;
        std::string texture2;
        std::string texture3;
        std::string texture4;
        std::array<float, 4> val1;
        std::array<float, 4> val2;
        std::array<float, 4> val3;
        std::array<float, 4> val4;
    };

    enum class MaterialPipelineFillMode : uint32_t
    {
        eFill = 0,
        //VK_POLYGON_MODE_FILL,
        eLine = 1,
        //VK_POLYGON_MODE_LINE,
        ePoint = 2 //VK_POLYGON_MODE_POINT
    };

    enum class MaterialPipelineCullMode : uint32_t
    {
        eNone = 0,
        //VK_CULL_MODE_NONE,
        eFront = 1,
        //VK_CULL_MODE_FRONT_BIT,
        eBack = 2,
        //VK_CULL_MODE_BACK_BIT,
        eFrontAndBack = 3 //VK_CULL_MODE_FRONT_AND_BACK
    };

    enum class MaterialPipelineFrontFace : uint32_t
    {
        eCounterClockwise = 0,
        //VK_FRONT_FACE_COUNTER_CLOCKWISE,
        eClockwise = 1 //VK_FRONT_FACE_CLOCKWISE
    };

    enum class MaterialPipelineDepthCompareOp : uint32_t
    {
        eNever = 0,
        //VK_COMPARE_OP_NEVER,
        eLess = 1,
        //VK_COMPARE_OP_LESS,
        eEqual = 2,
        //VK_COMPARE_OP_EQUAL,
        eLessOrEqual = 3,
        //VK_COMPARE_OP_LESS_OR_EQUAL,
        eGreater = 4,
        //VK_COMPARE_OP_GREATER,
        eNotEqual = 5,
        //VK_COMPARE_OP_NOT_EQUAL,
        eGreaterOrEqual = 6,
        //VK_COMPARE_OP_GREATER_OR_EQUAL,
        eAlways = 7 //VK_COMPARE_OP_ALWAYS
    };

    enum class MaterialPipelineBlendFactor : uint32_t
    {
        eZero = 0,
        // VK_BLEND_FACTOR_ZERO,
        eOne = 1,
        //VK_BLEND_FACTOR_ONE,
        eSrcColor = 2,
        //VK_BLEND_FACTOR_SRC_COLOR,
        eOneMinusSrcColor = 3,
        //VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
        eDstColor = 4,
        //VK_BLEND_FACTOR_DST_COLOR,
        eOneMinusDstColor = 5,
        //VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
        eSrcAlpha = 6,
        //VK_BLEND_FACTOR_SRC_ALPHA,
        eOneMinusSrcAlpha = 7,
        //VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        eDstAlpha = 8,
        //VK_BLEND_FACTOR_DST_ALPHA,
        eOneMinusDstAlpha = 9,
        //VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
        eConstantColor = 10,
        //VK_BLEND_FACTOR_CONSTANT_COLOR,
        eOneMinusConstantColor = 11,
        //VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
        eConstantAlpha = 12,
        //VK_BLEND_FACTOR_CONSTANT_ALPHA,
        eOneMinusConstantAlpha = 13,
        //VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
        eSrcAlphaSaturate = 14,
        //VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
        eSrc1Color = 15,
        //VK_BLEND_FACTOR_SRC1_COLOR,
        eOneMinusSrc1Color = 16,
        //VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
        eSrc1Alpha = 17,
        //VK_BLEND_FACTOR_SRC1_ALPHA,
        eOneMinusSrc1Alpha = 18 //VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA
    };

    enum class MaterialPipelineBlendOp : uint32_t
    {
        eAdd = 0,
        //VK_BLEND_OP_ADD,
        eSubtract = 1,
        //VK_BLEND_OP_SUBTRACT,
        eReverseSubtract = 2,
        //VK_BLEND_OP_REVERSE_SUBTRACT,
        eMin = 3,
        //VK_BLEND_OP_MIN,
        eMax = 4 //VK_BLEND_OP_MAX,
    };

    struct MaterialPipelineAttachmentBlend
    {
        bool enable;
        MaterialPipelineBlendFactor sourceFactor;
        MaterialPipelineBlendFactor destFactor;
        MaterialPipelineBlendOp colorBlendOp;
        MaterialPipelineBlendFactor sourceAlphaFactor;
        MaterialPipelineBlendFactor destAlphaFactor;
        MaterialPipelineBlendOp alphaBlendOp;
    };

    enum class MaterialPipelineInputType : uint32_t
    {
        eFloat,
        eByte
    };

    struct MaterialPipelineInput
    {
        MaterialPipelineInputType inputType;
        uint32_t count;
        uint32_t offset;
    };

    enum class PipelineRenderStage : uint32_t
    {
        Opaque,
        Shadow,
        Transparent,
        UI
    };

    struct MaterialPipelineData
    {
        std::string vertexShader;
        std::string fragmentShader;

        float lineWidth;
        MaterialPipelineFillMode fillMode;
        bool depthClamp;
        MaterialPipelineCullMode cullMode;
        MaterialPipelineFrontFace frontFace;

        bool depthTestEnable;
        bool depthWriteEnable;
        MaterialPipelineDepthCompareOp depthCompareOp;

        bool stencilTest;
        float minDepth;
        float maxDepth;

        std::vector<MaterialPipelineAttachmentBlend> blends;
        std::vector<MaterialPipelineInput> inputs;

        PipelineRenderStage stage;

        std::string name;
    };

    struct EntityLOD
    {
        std::string modelName;
        float screenSpace;
    };

    struct EntityData
    {
        std::string materialname;

        uint8_t lodCount;
        std::array<EntityLOD, 4> LODS;
    };
}
