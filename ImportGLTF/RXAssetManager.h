#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"
#include <RXAssets.h>

#include "SerialisationData.h"

namespace tinygltf {
    class Model;
}

enum AssetType
{
    GTLF
};
#if 0
std::unordered_map<std::string, AssetType> map{
    {"gltf", AssetType::GTLF},
};
#endif
struct gltfImport
{
    RxAssets::MeshSaveData md;
    std::vector<RxAssets::MaterialData> mats;
    std::vector<RxAssets::ImageData> ims;
};

struct importList
{
    std::vector<std::pair<std::string, std::vector<uint8_t>>> items;
};

bool CreateGTLFData(std::string importFile, gltfImport & importData/*, nlohmann::json & options*/, tinygltf::Model & model);

void importGltf(
    importList & importList,
    nlohmann::json & j,
    std::string assetId,
    std::filesystem::path pp);

void importShader(
    importList & importList,
    nlohmann::json & j,
    std::string assetId,
    std::filesystem::path pp);

void importMaterial(
    importList & importList,
    nlohmann::json & j,
    std::string assetId);