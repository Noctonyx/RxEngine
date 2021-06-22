////////////////////////////////////////////////////////////////////////////////
// MIT License
//
// Copyright (c) 2021.  Shane Hyde (shane@noctonyx.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

//
// Created by shane on 17/02/2021.
//

#include <fstream>
#include "vulkan/vulkan.h"
//#include "json.hpp"
#include "Loader.h"
#include "Vfs.h"

#include "stb_image.h"

#define DDSKTX_IMPLEMENT

#include "dds-ktx.h"
//#include "bitsery/adapter/buffer.h"
#include "AssetException.h"

namespace RxAssets
{
  //  typedef bitsery::InputBufferAdapter<std::vector<uint8_t>> inputBufferAdapter;

    void Loader::loadTexture(TextureData & textureData, const std::filesystem::path & path)
    {
        auto vfs = Vfs::getInstance();

        auto load_path = path;

        if (load_path.extension() == ".tex") {
            load_path.replace_extension(".tex.json");
        }

        std::optional<size_t> size = vfs->getFilesize(load_path);

        if (!size.has_value()) {
            throw AssetException("Error loading texture asset:", path);
        }

        std::vector<std::byte> data(size.value());
        vfs->getFileContents(load_path, data.data());

        nlohmann::json j = nlohmann::json::parse(data.begin(), data.end());
        if (!j["image"].is_string()) {
            throw AssetException("No image specified:", path);
        }
        textureData.imageName = j["image"].get<std::string>();

        if (!j["sampler"].is_object()) {
            throw AssetException("No sampler details specified:", path);
        }

        getSamplerDetails(j, textureData.sampler);
    }

    void Loader::loadImage(ImageData & imageData, const std::filesystem::path & path)
    {
        if (!path.has_extension()) {
            throw AssetException("no extension on image file:", path);
        }

        std::string extension = path.extension().generic_string();
        if (extension == ".PNG" || extension == ".png" || extension == ".TGA" || extension == ".tga") {
            importPng(imageData, path);
        }

        if (extension == ".DDS" || extension == ".dds") {
            importDds(imageData, path);
        }
    }

    void Loader::importPng(ImageData & imageData, const std::filesystem::path & path)
    {
        int x, y, n;

        auto vfs = Vfs::getInstance();
        auto size = vfs->getFilesize(path);
        if (!size.has_value()) {
            throw AssetException("Error loading texture asset:", path);
        }

        std::vector<std::byte> data(size.value());
        vfs->getFileContents(path, data.data());

        int s =static_cast<int>(size.value());

        auto * ptr = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc *>(data.data()),
            s, &x, &y, &n, 0);

        imageData.width = static_cast<uint16_t>(x);
        imageData.height = static_cast<uint16_t>(y);

        //imageData.formatType = n;
        imageData.imType = RxAssets::eBitmap;
        imageData.mipLevels.resize(1);
        imageData.mipLevels[0].width = static_cast<uint16_t>(x);
        imageData.mipLevels[0].height = static_cast<uint16_t>(y);
        imageData.mipLevels[0].bytes.resize(x * y * n);
        imageData.name = path.generic_string();
        imageData.mips = 1;

        memcpy(imageData.mipLevels[0].bytes.data(), ptr, imageData.mipLevels[0].bytes.size());
    }

    void Loader::importDds(ImageData & imageData, const std::filesystem::path & path)
    {
        auto vfs = Vfs::getInstance();
        auto size = vfs->getFilesize(path);
        if (!size.has_value()) {
            throw AssetException("Error loading texture asset:", path);
        }

        std::vector<std::byte> data(size.value());
        vfs->getFileContents(path, data.data());

        ddsktx_texture_info texture_info = {}; 
        //  GLuint tex = 0;

        if (ddsktx_parse(&texture_info, data.data(), static_cast<int>(data.size()), nullptr)) {
            assert(texture_info.depth == 1);
            assert(texture_info.num_layers == 1);

            imageData.width = static_cast<uint16_t>(texture_info.width);
            imageData.height = static_cast<uint16_t>(texture_info.height);
            if (texture_info.format == DDSKTX_FORMAT_BC7) {
                imageData.imType = RxAssets::eBC7;
            }
            imageData.mips = static_cast<uint8_t>(texture_info.num_mips);
            imageData.mipLevels.resize(texture_info.num_mips);

            for (int mip = 0; mip < texture_info.num_mips; mip++) {
                ddsktx_sub_data sub_data;
                //int size;
                ddsktx_get_sub(&texture_info, &sub_data, data.data(), static_cast<int>(data.size()), 0, 0, mip);
                if (sub_data.width < 4) {
                    imageData.mipLevels.resize(mip - 1);
                    break;
                }
                imageData.mipLevels[mip].bytes.resize(sub_data.size_bytes);
                imageData.mipLevels[mip].width = static_cast<uint16_t>(sub_data.width);
                imageData.mipLevels[mip].height = static_cast<uint16_t>(sub_data.height);
                memcpy(imageData.mipLevels[mip].bytes.data(), sub_data.buff, sub_data.size_bytes);
            }
        }
    }

    void Loader::loadShader(ShaderData & shaderData, const std::filesystem::path & path)
    {
        auto vfs = Vfs::getInstance();
        auto size = vfs->getFilesize(path);
        if (!size.has_value()) {
            throw AssetException("Error loading texture asset:", path);
        }

        shaderData.bytes.resize(size.value() / 4);
        std::vector<std::byte> data(size.value());
        vfs->getFileContents(path, data.data());

        memcpy(shaderData.bytes.data(), data.data(), size.value());
    }

    void Loader::loadMesh(MeshSaveData & meshData, const std::filesystem::path & path)
    {
        auto vfs = Vfs::getInstance();
        auto size = vfs->getFilesize(path);
        if (!size.has_value()) {
            throw AssetException("Error loading texture asset:", path);
        }

        std::vector<std::byte> data(size.value());
        vfs->getFileContents(path, data.data());

        tser::BinaryArchive archive(0);

        std::string_view sv(reinterpret_cast<char *>(data.data()), data.size());
        archive.initialize(sv);

        archive.load<MeshSaveData>(meshData);
        //meshData = mesh;
        //auto state = bitsery::quickDeserialization<inputBufferAdapter>(
          //  {data.begin(), data.size()}, meshData);
        //(void)state;
    }

    void Loader::getSamplerDetails(nlohmann::json & json, RxAssets::SamplerData & sd)
    {
        sd.minFilter = sd.magFilter = 0;
        if (json["minFilter"].is_string()) {
            if (json["minFilter"].get<std::string>() == "nearest") {
                sd.minFilter = VK_FILTER_NEAREST;
            } else if (json["minFilter"].get<std::string>() == "linear") {
                sd.minFilter = VK_FILTER_LINEAR;
            } else {
                throw std::runtime_error(R"(Invalid value for minFilter - valid values: "nearest", "linear")");
            }
        }
        if (json["magFilter"].is_string()) {
            if (json["magFilter"].get<std::string>() == "nearest") {
                sd.magFilter = VK_FILTER_NEAREST;
            } else if (json["magFilter"].get<std::string>() == "linear") {
                sd.magFilter = VK_FILTER_LINEAR;
            } else {
                throw std::runtime_error(R"(Invalid value for magFilter - valid values: "nearest", "linear")");
            }
        }

        sd.mipMapMode = 0;
        if (json["mipMapMode"].is_string()) {
            if (json["mipMapMode"].get<std::string>() == "nearest") {
                sd.mipMapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            } else if (json["mipMapMode"].get<std::string>() == "linear") {
                sd.mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            } else {
                throw std::runtime_error(R"(Invalid value for mip MapMode - valid values: "nearest", "linear" )");
            }
        }

        sd.addressU = 0;
        if (json["addressU"].is_string()) {
            if (json["addressU"].get<std::string>() == "repeat") {
                sd.addressU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            } else if (json["addressU"].get<std::string>() == "mirrored-repeat") {
                sd.addressU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            } else if (json["addressU"].get<std::string>() == "clamp-edge") {
                sd.addressU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            } else if (json["addressU"].get<std::string>() == "clamp-border") {
                sd.addressU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            } else {
                throw std::runtime_error(
                    R"(Invalid value for addressU - valid values: "repeat", "mirrored-repeat", "clamp-edge", "clamp-border")");
            }
        }
        sd.addressV = 0;
        if (json["addressV"].is_string()) {
            if (json["addressV"].get<std::string>() == "repeat") {
                sd.addressV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            } else if (json["addressV"].get<std::string>() == "mirrored-repeat") {
                sd.addressV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            } else if (json["addressV"].get<std::string>() == "clamp-edge") {
                sd.addressV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            } else if (json["addressV"].get<std::string>() == "clamp-border") {
                sd.addressV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            } else {
                throw std::runtime_error(
                    R"(Invalid value for addressV - valid values: "repeat", "mirrored-repeat", "clamp-edge", "clamp-border")");
            }
        }
        sd.addressW = 0;
        if (json["addressW"].is_string()) {
            if (json["addressW"].get<std::string>() == "repeat") {
                sd.addressW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            } else if (json["addressW"].get<std::string>() == "mirrored-repeat") {
                sd.addressW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            } else if (json["addressW"].get<std::string>() == "clamp-edge") {
                sd.addressW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            } else if (json["addressW"].get<std::string>() == "clamp-border") {
                sd.addressW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            } else {
                throw std::runtime_error(
                    R"(Invalid value for addressW - valid values: "repeat", "mirrored-repeat", "clamp-edge", "clamp-border")");
            }
        }

        sd.mipLodBias = 0.f;
        if (json["mipLodBias"].is_number()) {
            sd.mipLodBias = json["mipLodBias"].get<float>();
        }

        sd.anisotropy = false;
        if (json["anisotropy"].is_boolean()) {
            sd.anisotropy = json["anisotropy"].get<bool>();
        }

        sd.maxAnisotropy = 0.0f;
        if (json["maxAnisotropy"].is_number()) {
            sd.maxAnisotropy = json["maxAnisotropy"].get<float>();
        }

        sd.minLod = 0.f;
        if (json["minLod"].is_number()) {
            sd.minLod = json["minLod"].get<float>();
        }

        sd.maxLod = VK_LOD_CLAMP_NONE;
        if (json["maxLod"].is_number()) {
            sd.maxLod = json["maxLod"].get<float>();
        }

        sd.borderColor = 0;
        if (json["borderColor"].is_string()) {
            if (json["borderColor"].get<std::string>() == "float-transparent-black") {
                sd.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
            } else if (json["borderColor"].get<std::string>() == "int-transparent-black") {
                sd.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
            } else if (json["borderColor"].get<std::string>() == "float-opaque-black") {
                sd.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            } else if (json["borderColor"].get<std::string>() == "int-opaque-black") {
                sd.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            } else if (json["borderColor"].get<std::string>() == "float-opaque-white") {
                sd.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            } else if (json["borderColor"].get<std::string>() == "int-opaque-white") {
                sd.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
            } else {
                throw std::runtime_error(
                    R"(Invalid value for borderColor - valid values are "float-transparent-black", "int-transparent-black", "float-opaque-black", "int-opaque-black", "float-opaque-white", "int-opaque-white" )");
            }
        }
    }

    void Loader::loadMaterial(MaterialData2 & materialData, const std::filesystem::path & path)
    {
        auto vfs = Vfs::getInstance();

        auto load_path = path;

        if (load_path.extension() == ".mat") {
            load_path.replace_extension(".mat.json");
        }

        auto size = vfs->getFilesize(load_path);
        if (!size.has_value()) {
            throw AssetException("Error loading material asset:", path);
        }

        std::vector<std::byte> data(size.value());
        vfs->getFileContents(load_path, data.data());

        nlohmann::json j = nlohmann::json::parse(data.begin(), data.end());

        if (!j["materialBaseName"].is_string()) {
            throw AssetException("No material base name specified:", path);
        }

        materialData.materialBaseName = j["materialBaseName"].get<std::string>();
        std::ranges::fill(materialData.val1, 0.f);
        std::ranges::fill(materialData.val2, 0.f);
        std::ranges::fill(materialData.val3, 0.f);
        std::ranges::fill(materialData.val4, 0.f);
        //materialData.val1 = materialData.val2 = materialData.val3 = materialData.val4 = 0.0f;

        if (j["value1"].is_array()) {
            for(size_t i = 0; i < j["value1"].size() && i < 4; i++) {
                if(j["value1"][i].is_number()) {
                    materialData.val1[i] = j["value1"][i].get<float>();
                }
            }
        }
        if (j["value2"].is_array()) {
            for(size_t i = 0; i < j["value2"].size() && i < 4; i++) {
                if(j["value2"][i].is_number()) {
                    materialData.val2[i] = j["value2"][i].get<float>();
                }
            }
        }
        if (j["value3"].is_array()) {
            for(size_t i = 0; i < j["value3"].size() && i < 4; i++) {
                if(j["value3"][i].is_number()) {
                    materialData.val3[i] = j["value3"][i].get<float>();
                }
            }
        }
        if (j["value4"].is_array()) {
            for(size_t i = 0; i < j["value4"].size() && i < 4; i++) {
                if(j["value4"][i].is_number()) {
                    materialData.val4[i] = j["value4"][i].get<float>();
                }
            }
        }
    }

    void Loader::loadMaterialBase(MaterialBaseData & materialBaseData, const std::filesystem::path & path)
    {
        auto vfs = Vfs::getInstance();

        auto load_path = path;

        if (load_path.extension() == ".matbase") {
            load_path.replace_extension(".matbase.json");
        }

        auto size = vfs->getFilesize(load_path);
        if (!size.has_value()) {
            throw AssetException("Error loading materialbase asset:", path);
        }

        std::vector<std::byte> data(size.value());
        vfs->getFileContents(load_path, data.data());

        nlohmann::json j = nlohmann::json::parse(data.begin(), data.end());

        if (!j["opaquePipeline"].is_string() && !j["shadowPipeline"].is_string() &&
            !j["transparentPipeline"].is_string()) {
            throw AssetException("No material pipeline specified:", path);
        }

        materialBaseData.opaquePipelineName = j["opaquePipeline"].get<std::string>();
        materialBaseData.shadowPipelineName = j["shadowPipeline"].get<std::string>();
        materialBaseData.transparentPipelineName = j["transparentPipeline"].get<std::string>();

        std::ranges::fill(materialBaseData.val1, 0.f);
        std::ranges::fill(materialBaseData.val2, 0.f);
        std::ranges::fill(materialBaseData.val3, 0.f);
        std::ranges::fill(materialBaseData.val4, 0.f);

        if (j["value1"].is_array()) {
            for(size_t i = 0; i < j["value1"].size() && i < 4; i++) {
                if(j["value1"][i].is_number()) {
                    materialBaseData.val1[i] = j["value1"][i].get<float>();
                }
            }
        }
        if (j["value2"].is_array()) {
            for(size_t i = 0; i < j["value2"].size() && i < 4; i++) {
                if(j["value2"][i].is_number()) {
                    materialBaseData.val2[i] = j["value2"][i].get<float>();
                }
            }
        }
        if (j["value3"].is_array()) {
            for(size_t i = 0; i < j["value3"].size() && i < 4; i++) {
                if(j["value3"][i].is_number()) {
                    materialBaseData.val3[i] = j["value3"][i].get<float>();
                }
            }
        }
        if (j["value4"].is_array()) {
            for(size_t i = 0; i < j["value4"].size() && i < 4; i++) {
                if(j["value4"][i].is_number()) {
                    materialBaseData.val4[i] = j["value4"][i].get<float>();
                }
            }
        }

        if (j["texture1"].is_string()) {
            materialBaseData.texture1 = j["texture1"].get<std::string>();
        }
        if (j["texture2"].is_string()) {
            materialBaseData.texture2 = j["texture2"].get<std::string>();
        }
        if (j["texture3"].is_string()) {
            materialBaseData.texture3 = j["texture3"].get<std::string>();
        }
        if (j["texture4"].is_string()) {
            materialBaseData.texture4 = j["texture4"].get<std::string>();
        }
    }

    void Loader::loadMaterialPipeline(MaterialPipelineData & materialPipelineData, const std::filesystem::path & path)
    {
        auto vfs = Vfs::getInstance();

        auto load_path = path;

        if (load_path.extension() == ".matpipe") {
            load_path.replace_extension(".matpipe.json");
        }

        auto size = vfs->getFilesize(load_path);
        if (!size.has_value()) {
            throw AssetException("Error loading material pipeline asset:", path);
        }

        std::vector<std::byte> data(size.value());
        vfs->getFileContents(load_path, data.data());

        nlohmann::json j = nlohmann::json::parse(data.begin(), data.end());

        if (!j["vertexShader"].is_string()) {
            throw AssetException("No vertexShader specified:", path);
        }

        materialPipelineData.vertexShader = j["vertexShader"].get<std::string>();

        if (!j["fragmentShader"].is_string()) {
            throw AssetException("No Fragment Shader specified:", path);
        }

        if (!j["renderStage"].is_string()) {
            throw AssetException("No Render Stage specified:", path);
        }

        if (j["renderStage"].get<std::string>() == "opaque") {
            materialPipelineData.stage = PipelineRenderStage::Opaque;
        } else if (j["renderStage"].get<std::string>() == "shadow") {
            materialPipelineData.stage = PipelineRenderStage::Shadow;
        } else if (j["renderStage"].get<std::string>() == "transparent") {
            materialPipelineData.stage = PipelineRenderStage::Transparent;
        } else if (j["renderStage"].get<std::string>() == "ui") {
            materialPipelineData.stage = PipelineRenderStage::UI;
        } else {
            throw std::runtime_error(
                R"(Invalid value for renderStage - valid values: "opaque", "shadow", "transparent", "ui")");
        }

        materialPipelineData.fragmentShader = j["fragmentShader"].get<std::string>();
        materialPipelineData.lineWidth = 1.0f;
        materialPipelineData.fillMode = MaterialPipelineFillMode::eFill;
        materialPipelineData.depthTestEnable = false;
        materialPipelineData.cullMode = MaterialPipelineCullMode::eBack;
        materialPipelineData.frontFace = MaterialPipelineFrontFace::eCounterClockwise;

        materialPipelineData.depthTestEnable = false;
        materialPipelineData.depthWriteEnable = false;
        materialPipelineData.depthCompareOp = MaterialPipelineDepthCompareOp::eLessOrEqual;
        materialPipelineData.stencilTest = false;
        materialPipelineData.minDepth = 0.0f;
        materialPipelineData.maxDepth = 1.0f;

        materialPipelineData.name = path.generic_string();

        if (j["lineWidth"].is_number()) {
            materialPipelineData.lineWidth = j["lineWidth"].get<float>();
        }

        if (j["fillMode"].is_string()) {
            if (j["fillMode"].get<std::string>() == "fill") {
                materialPipelineData.fillMode = MaterialPipelineFillMode::eFill;
            } else if (j["fillMode"].get<std::string>() == "line") {
                materialPipelineData.fillMode = MaterialPipelineFillMode::eLine;
            } else if (j["fillMode"].get<std::string>() == "point") {
                materialPipelineData.fillMode = MaterialPipelineFillMode::ePoint;
            } else {
                throw std::runtime_error(
                    R"(Invalid value for fillMode - valid values: "fill", "line", "point")");
            }
        }

        if (j["depthClamp"].is_boolean()) {
            materialPipelineData.depthClamp = j["depthClamp"].get<bool>();
        }

        if (j["cullMode"].is_string()) {
            if (j["cullMode"].get<std::string>() == "back") {
                materialPipelineData.cullMode = MaterialPipelineCullMode::eBack;
            } else if (j["cullMode"].get<std::string>() == "front") {
                materialPipelineData.cullMode = MaterialPipelineCullMode::eFront;
            } else if (j["cullMode"].get<std::string>() == "both") {
                materialPipelineData.cullMode = MaterialPipelineCullMode::eFrontAndBack;
            } else if (j["cullMode"].get<std::string>() == "none") {
                materialPipelineData.cullMode = MaterialPipelineCullMode::eNone;
            } else {
                throw std::runtime_error(
                    R"(Invalid value for fillMode - valid values: "back", "front", "both", "none")");
            }
        }

        if (j["frontFace"].is_string()) {
            if (j["frontFace"].get<std::string>() == "counter-clockwise") {
                materialPipelineData.frontFace = MaterialPipelineFrontFace::eCounterClockwise;
            } else if (j["frontFace"].get<std::string>() == "clockwise") {
                materialPipelineData.frontFace = MaterialPipelineFrontFace::eClockwise;
            } else {
                throw std::runtime_error(
                    R"(Invalid value for fillMode - valid values: "counter-clockwise", "clockwise")");
            }
        }

        if (j["depthTestEnable"].is_boolean()) {
            materialPipelineData.depthTestEnable = j["depthTestEnable"].get<bool>();
        }

        if (j["depthWriteEnable"].is_boolean()) {
            materialPipelineData.depthWriteEnable = j["depthWriteEnable"].get<bool>();
        }

        if (j["stencilTest"].is_boolean()) {
            materialPipelineData.stencilTest = j["stencilTest"].get<bool>();
        }

        if (j["depthCompare"].is_string()) {
            if (j["depthCompare"].get<std::string>() == "never") {
                materialPipelineData.depthCompareOp = MaterialPipelineDepthCompareOp::eNever;
            } else if (j["depthCompare"].get<std::string>() == "less") {
                materialPipelineData.depthCompareOp = MaterialPipelineDepthCompareOp::eLess;
            } else if (j["depthCompare"].get<std::string>() == "equal") {
                materialPipelineData.depthCompareOp = MaterialPipelineDepthCompareOp::eEqual;
            } else if (j["depthCompare"].get<std::string>() == "less-equal") {
                materialPipelineData.depthCompareOp = MaterialPipelineDepthCompareOp::eLessOrEqual;
            } else if (j["depthCompare"].get<std::string>() == "greater") {
                materialPipelineData.depthCompareOp = MaterialPipelineDepthCompareOp::eGreater;
            } else if (j["depthCompare"].get<std::string>() == "not-equal") {
                materialPipelineData.depthCompareOp = MaterialPipelineDepthCompareOp::eNotEqual;
            } else if (j["depthCompare"].get<std::string>() == "greater-equal") {
                materialPipelineData.depthCompareOp = MaterialPipelineDepthCompareOp::eGreaterOrEqual;
            } else if (j["depthCompare"].get<std::string>() == "always") {
                materialPipelineData.depthCompareOp = MaterialPipelineDepthCompareOp::eAlways;
            } else {
                throw std::runtime_error(
                    R"(Invalid value for fillMode - valid values: "never", "less", "equal", "less-equal", "greater", "not-equal", "greater-equal", "always")");
            }
        }

        if (j["minDepth"].is_number()) {
            materialPipelineData.minDepth = j["minDepth"].get<float>();
        }

        if (j["maxDepth"].is_number()) {
            materialPipelineData.maxDepth = j["maxDepth"].get<float>();
        }

        if (j["vertices"].is_array()) {
            for (auto & v : j["vertices"]) {
                uint32_t count;
                uint32_t offset;

                count = v["count"].get<uint32_t>();
                offset = v["offset"].get<uint32_t>();

                auto & ip = materialPipelineData.inputs.emplace_back();
                ip.offset = offset;
                ip.count = count;

                if (v["type"].get<std::string>() == "float") {
                    ip.inputType = MaterialPipelineInputType::eFloat;
                } else if (v["type"].get<std::string>() == "byte") {
                    ip.inputType = MaterialPipelineInputType::eByte;
                } else {
                    throw std::runtime_error(
                        R"(Invalid value for input type - valid values: "byte", "float")");
                }
            }
        }

        if (j["blends"].is_array()) {

            auto & b = materialPipelineData.blends.emplace_back();
            b.enable = false;
            b.sourceFactor = MaterialPipelineBlendFactor::eSrcAlpha;
            b.destFactor = MaterialPipelineBlendFactor::eOneMinusSrcAlpha;
            b.colorBlendOp = MaterialPipelineBlendOp::eAdd;
            b.sourceAlphaFactor = MaterialPipelineBlendFactor::eOneMinusSrcAlpha;
            b.destAlphaFactor = MaterialPipelineBlendFactor::eZero;
            b.alphaBlendOp = MaterialPipelineBlendOp::eAdd;

            for (auto & jblend: j["blends"]) {
                if (jblend["enable"].is_boolean()) {
                    b.enable = jblend["enable"].get<bool>();
                }

                if (jblend["sourceFactor"].is_string()) {
                    b.sourceFactor = getBlendFactor("sourceFactor", jblend);
                }

                if (jblend["destFactor"].is_string()) {
                    b.destFactor = getBlendFactor("destFactor", jblend);
                }

                if (jblend["sourceAlphaFactor"].is_string()) {
                    b.sourceAlphaFactor = getBlendFactor("sourceAlphaFactor", jblend);
                }

                if (jblend["destAlphaFactor"].is_string()) {
                    b.destAlphaFactor = getBlendFactor("destAlphaFactor", jblend);
                }

                if (jblend["colorOp"].is_string()) {
                    b.colorBlendOp = getBlendOp("colorOp", jblend);
                }

                if (jblend["alphaOp"].is_string()) {
                    b.alphaBlendOp = getBlendOp("alphaOp", jblend);
                }
            }
        }
    }

    MaterialPipelineBlendFactor Loader::getBlendFactor(const std::string & name, nlohmann::json & json)
    {
        if (json[name].get<std::string>() == "zero") {
            return MaterialPipelineBlendFactor::eZero;
        } else if (json[name].get<std::string>() == "one") {
            return MaterialPipelineBlendFactor::eOne;
        } else if (json[name].get<std::string>() == "src-color") {
            return MaterialPipelineBlendFactor::eSrcColor;
        } else if (json[name].get<std::string>() == "1-src-color") {
            return MaterialPipelineBlendFactor::eOneMinusSrcColor;
        } else if (json[name].get<std::string>() == "dest-color") {
            return MaterialPipelineBlendFactor::eDstColor;
        } else if (json[name].get<std::string>() == "src-alpha") {
            return MaterialPipelineBlendFactor::eSrcAlpha;
        } else if (json[name].get<std::string>() == "1-src-alpha") {
            return MaterialPipelineBlendFactor::eOneMinusSrcAlpha;
        } else if (json[name].get<std::string>() == "dest-alpha") {
            return MaterialPipelineBlendFactor::eDstAlpha;
        } else {
            throw std::runtime_error(
                R"(Invalid value for blendFactor - valid values: "zero", "one", "src-color", "1-src-color", "dest-color", "src-alpha", "1-src-alpha", "dest-alpha")");
        }
    }

    MaterialPipelineBlendOp Loader::getBlendOp(const std::string & name, nlohmann::json & json)
    {
        if (json[name].get<std::string>() == "add") {
            return MaterialPipelineBlendOp::eAdd;
        } else if (json[name].get<std::string>() == "subtract") {
            return MaterialPipelineBlendOp::eSubtract;
        } else if (json[name].get<std::string>() == "reverse-subtract") {
            return MaterialPipelineBlendOp::eReverseSubtract;
        } else if (json[name].get<std::string>() == "min") {
            return MaterialPipelineBlendOp::eMin;
        } else if (json[name].get<std::string>() == "max") {
            return MaterialPipelineBlendOp::eMax;
        } else {
            throw std::runtime_error(
                R"(Invalid value for blendOp - valid values: "add", "subtract", "reverse-subtract", "min", "max")");
        }
    }

    void Loader::loadEntity(EntityData & entityData, const std::filesystem::path & path)
    {
        auto vfs = Vfs::getInstance();

        auto load_path = path;

        if (load_path.extension() == ".ent") {
            load_path.replace_extension(".ent.json");
        }

        auto size = vfs->getFilesize(load_path);
        if (!size.has_value()) {
            throw AssetException("Error loading entity asset:", path);
        }

        std::vector<std::byte> data(size.value());
        vfs->getFileContents(load_path, data.data());

        nlohmann::json j = nlohmann::json::parse(data.begin(), data.end());

        if (!j["material"].is_string()) {
            throw AssetException("No material specified:", path);
        }

        if (!j["lod"].is_array()) {
            throw AssetException("No LOD specified:", path);
        }

        entityData.lodCount = 0;

        for (auto & jj: j["lod"]) {
            if (!jj["mesh"].is_string()) {
                throw AssetException("No mesh specified for lod:", path);
            }
            if (!jj["amount"].is_number()) {
                throw AssetException("No amount specified for lod:", path);
            }
            entityData.LODS[entityData.lodCount].modelName = jj["mesh"];
            entityData.LODS[entityData.lodCount].screenSpace = jj["amount"];
            entityData.lodCount++;
            if (entityData.lodCount == 4) {
                break;
            }
        }

        entityData.materialname = j["material"].get<std::string>();
    }
}