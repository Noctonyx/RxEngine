#include <algorithm>
#include <string>
#include "RXAssetManager.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_JSON
//#define TINYGLTF_NO_STB_IMAGE
#include <iostream>
#include "tiny_gltf.h"
//#include "glm/gtc/type_ptr.hpp"

#if 0
void importGltf(importList & importList, nlohmann::json & j, std::string assetId, std::filesystem::path pp)
{
    gltfImport gi;

    CreateGTLFData(pp.generic_string(), gi, j["options"]);
    {
        int mix = 0;
        /*
                        for (auto & prim: gi.md.primitives) {
                            std::ostringstream ss;
            
                            ss << assetId << "_mat[" << std::to_string(prim.materialIndex) << "]";
                            prim.materialName = ss.str();
                        }
                        */
        for (auto & m: gi.mats) {
            std::ostringstream ss;

            ss << assetId << "_mat[" << std::to_string(mix) << "]";

            gi.md.materials.emplace_back(ss.str());
            mix++;
        }
        auto & newi = importList.items.emplace_back();
        auto w = RXAssets::QuickSerialise(newi.second, gi.md);
        newi.second.resize(w);
        newi.first = assetId;
    }

    for (auto & ai: gi.ims) {
        std::ostringstream ss;
        ss << assetId << "[" << ai.name << "]";
        ai.name = ss.str();

        auto & newi = importList.items.emplace_back();
        newi.first = ai.name;
        auto w = RXAssets::QuickSerialise(newi.second, ai);
        newi.second.resize(w);
    }

    int mix = 0;
    for (auto & m: gi.mats) {
        std::ostringstream ss, ss2, ss3;

        ss << assetId << "[" << m.colorTextureAssetName << "]";
        m.colorTextureAssetName = ss.str();
        ss2 << assetId << "[" << m.emissionTextureAssetName << "]";
        m.emissionTextureAssetName = ss2.str();
        ss3 << assetId << "_mat[" << std::to_string(mix) << "]";

        auto & newi = importList.items.emplace_back();
        newi.first = ss3.str();
        auto w = RXAssets::QuickSerialise(newi.second, m);
        newi.second.resize(w);
        mix++;
    }
}
#endif
bool CreateGTLFData(std::string importFile, gltfImport & importData/*, nlohmann::json & options*/, tinygltf::Model & model)
{
    //tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool import_materials = true;
    bool import_images = true;
    std::string material_name = "";

    /*
    if (options["material_import"].is_boolean()) {
        import_materials = options["material_import"].get<bool>();
    }
    if (options["image_import"].is_boolean()) {
        import_images = options["image_import"].get<bool>();
        if (!import_images) {
            import_materials = false;
        }
    }
    if (options["material_name"].is_string()) {
        material_name = options["material_name"].get<std::string>();
    }
*/
    //auto hashAsset = entt::hashed_string::value(assetId.c_str());

    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, importFile);

    if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
    }
    if (!err.empty()) {
        printf("Err: %s\n", err.c_str());
    }
    if (!ret) {
        throw std::exception("Unable to read gltf file");
    }

    if (model.meshes.size() != 1) {
        throw std::exception("There should only be 1 mesh in the file");
    }

    //RXAssets::MeshData md;
    //RXAssets::MaterialData mat;
    ///std::vector<std::pair<std::string, RXAssets::ImageData>> ims;

    importData.md.minpx = 1e9;
    importData.md.minpy = 1e9;
    importData.md.minpz = 1e9;
    importData.md.maxpx = -1e9;
    importData.md.maxpy = -1e9;
    importData.md.maxpz = -1e9;

    auto & mesh = model.meshes[0];

    for (auto & prim: mesh.primitives) {
        const float * positionBuffer = nullptr;
        const float * normalsBuffer = nullptr;
        const float * texCoordsBuffer = nullptr;

        uint32_t firstIndex = static_cast<uint32_t>(importData.md.indices.size());
        uint32_t vertexStart = static_cast<uint32_t>(importData.md.vertices.size());
        uint32_t indexCount = 0;

        size_t vertexCount = 0;

        if (prim.attributes.find("POSITION") != prim.attributes.end()) {
            const tinygltf::Accessor & accessor = model.accessors[prim.attributes.find("POSITION")->
                second];
            const tinygltf::BufferView & view = model.bufferViews[accessor.bufferView];

            positionBuffer = reinterpret_cast<const float *>(&(model.buffers[view.buffer].data[
                accessor.byteOffset +
                view.byteOffset]));
            vertexCount = accessor.count;
        }
        if (prim.attributes.find("NORMAL") != prim.attributes.end()) {
            const tinygltf::Accessor & accessor = model.accessors[prim.attributes.find("NORMAL")->
                second];
            const tinygltf::BufferView & view = model.bufferViews[accessor.bufferView];

            normalsBuffer = reinterpret_cast<const float *>(&(model.buffers[view.buffer].data[
                accessor.byteOffset +
                view.byteOffset]));
        }
        if (prim.attributes.find("TEXCOORD_0") != prim.attributes.end()) {
            const tinygltf::Accessor & accessor = model.accessors[prim.attributes.find("TEXCOORD_0")
                ->second];
            const tinygltf::BufferView & view = model.bufferViews[accessor.bufferView];

            texCoordsBuffer = reinterpret_cast<const float *>(&(model.buffers[view.buffer].data[
                accessor.byteOffset +
                view.byteOffset]));
        }

        for (size_t v = 0; v < vertexCount; v++) {
            RxAssets::MeshSaveVertex vert{};
            vert.x = positionBuffer[v * 3];
            vert.y = positionBuffer[v * 3 + 1];
            vert.z = positionBuffer[v * 3 + 2];
            //vert.vertex = DirectX::XMFLOAT3(&positionBuffer[v * 3]);
            //vert.normal = DirectX::XMFLOAT3(&normalsBuffer[v * 3]);
            vert.nx = normalsBuffer[v * 3];
            vert.ny = normalsBuffer[v * 3 + 1];
            vert.nz = normalsBuffer[v * 3 + 2];

            //auto n = DirectX::XMLoadFloat3(&vert.normal);
            //DirectX::XMStoreFloat3(&vert.normal, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&vert.normal)));
            //vert.uvs =  texCoordsBuffer ? DirectX::XMFLOAT2(&texCoordsBuffer[v * 2]) : DirectX::XMFLOAT2{0.f, 0.f};
            vert.uvx = texCoordsBuffer[v * 2];
            vert.uvy = texCoordsBuffer[v * 2 + 1];
#if 0
            //vert.vertex
            vert.vertex = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
            vert.normal = glm::normalize(
                glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
            vert.uvs = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
#endif
            importData.md.vertices.push_back(vert);
            if (vert.x < importData.md.minpx) {
                importData.md.minpx = vert.x;
            }
            if (vert.y < importData.md.minpy) {
                importData.md.minpy = vert.y;
            }
            if (vert.z < importData.md.minpz) {
                importData.md.minpz = vert.z;
            }
            if (vert.x > importData.md.maxpx) {
                importData.md.maxpx = vert.x;
            }
            if (vert.y > importData.md.maxpy) {
                importData.md.maxpy = vert.y;
            }
            if (vert.z > importData.md.maxpz) {
                importData.md.maxpz = vert.z;
            }
        }

        const tinygltf::Accessor & accessor = model.accessors[prim.indices];
        const tinygltf::BufferView & bufferView = model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer & buffer = model.buffers[bufferView.buffer];

        indexCount += static_cast<uint32_t>(accessor.count);

        switch (accessor.componentType) {
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
            {
                uint32_t * buf = new uint32_t[accessor.count];
                memcpy(
                    buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                    accessor.count * sizeof(uint32_t));
                for (size_t index = 0; index < accessor.count; index++) {
                    importData.md.indices.push_back(buf[index] + vertexStart);
                }
                delete[] buf;
                break;
            }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
            {
                uint16_t * buf = new uint16_t[accessor.count];
                memcpy(
                    buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                    accessor.count * sizeof(uint16_t));
                for (size_t index = 0; index < accessor.count; index++) {
                    importData.md.indices.push_back(buf[index] + vertexStart);
                }
                delete[] buf;
                break;
            }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
            {
                uint8_t * buf = new uint8_t[accessor.count];
                memcpy(
                    buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                    accessor.count * sizeof(uint8_t));
                for (size_t index = 0; index < accessor.count; index++) {
                    importData.md.indices.push_back(buf[index] + vertexStart);
                }
                delete[] buf;
                break;
            }
        default:
            std::cerr << "Index component type " << accessor.componentType << " not supported!" <<
                std::endl;
            return false;
        }

        importData.md
                  .primitives
                  .push_back({
                      firstIndex, indexCount, !material_name.empty() ? -1 : prim.material,
                      material_name
                  });
    }

    int ixn = 0;
    if (import_images) {
        for (auto & i: model.images) {
            size_t ii = importData.ims.size();
            auto & id = importData.ims.emplace_back();
            id.width = i.width;
            id.height = i.height;
            id.mipLevels.resize(1);
            id.mipLevels[0].width = i.width;
            id.mipLevels[0].height = i.height;
            //id.formatType = i.pixel_type;

            id.mipLevels[0].bytes.resize(i.image.size());
            std::ranges::copy(i.image, id.mipLevels[0].bytes.begin());
            id.name = i.name;
            if (i.name.empty()) {
                std::ostringstream s;
                s << "_tex_" << std::to_string(ixn);
                id.name = s.str();
            }
            ixn++;
        }
    }
    if (import_materials) {
        for (auto & m: model.materials) {
            size_t mi = importData.mats.size();
            auto & matd = importData.mats.emplace_back();

            matd.name = m.name;

            matd.metallicValue = static_cast<float>(m.pbrMetallicRoughness.metallicFactor);
            matd.roughnessValue = static_cast<float>(m.pbrMetallicRoughness.roughnessFactor);

            matd.transparency = m.alphaMode;
            if (m.pbrMetallicRoughness.baseColorTexture.index >= 0) {
                matd.colorTextureAssetName = importData.ims[m.pbrMetallicRoughness.baseColorTexture.
                                                              index].name;
            }
            if (m.emissiveTexture.index >= 0) {
                matd.emissionTextureAssetName = importData.ims[m.emissiveTexture.index].name;
            }
            if (m.emissiveTexture.index >= 0) {
                matd.emissionTextureAssetName = importData.ims[m.emissiveTexture.index].name;
            }
            if (m.normalTexture.index >= 0) {
                matd.normalTextureAssetName= importData.ims[m.normalTexture.index].name;
            }
        }
    }

    return true;
}
