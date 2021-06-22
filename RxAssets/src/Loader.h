//
// Created by shane on 17/02/2021.
//

#ifndef RX_LOADER_H
#define RX_LOADER_H

#include <filesystem>
#include "SerialisationData.h"
#include "nlohmann/json.hpp"

namespace RxAssets
{
    class Loader
    {
    public:
        static void loadEntity(EntityData & entityData, const std::filesystem::path & path);
        static void loadTexture(TextureData & textureData, const std::filesystem::path & path);
        static void loadImage(ImageData & imageData, const std::filesystem::path & path);
        static void loadShader(ShaderData & shaderData, const std::filesystem::path & path);
        static void loadMesh(MeshSaveData & meshData, const std::filesystem::path & path);
        static void loadMaterial(MaterialData2 & meshData, const std::filesystem::path & path);
        static void loadMaterialBase(MaterialBaseData & materialBaseData, const std::filesystem::path & path);
        static void loadMaterialPipeline(
            MaterialPipelineData & materialPipelineData,
            const std::filesystem::path & path);

    private:
        static void getSamplerDetails(nlohmann::json & json, RxAssets::SamplerData & sd);
        static void importPng(ImageData & imageData, const std::filesystem::path & path);
        static void importDds(ImageData & imageData, const std::filesystem::path & path);

        static MaterialPipelineBlendFactor getBlendFactor(const std::string & name, nlohmann::json & json);
        static MaterialPipelineBlendOp getBlendOp(const std::string & name, nlohmann::json & json);
    };
}
#endif //RX_LOADER_H
