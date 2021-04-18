#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <iomanip>
#include <chrono>
#include <cstdlib>
#include <direct.h>
#include <fstream>
#include <stb_image_write.h>
#include "Args.h"
#include "nlohmann/json.hpp"
#include "RXAssetManager.h"
//#include "stb_image.h"
#include "spdlog/spdlog.h"
//#include "glslang/Public/ShaderLang.h"
//#include "vulkan/vulkan.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
//#define STBI_WRITE_NO_STDIO

#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

//#include "dds-ktx.h"
//#include "SamplerConfig.h"
//#include "Images.h"
#if 0
void ImportObject(
    importList& importList,
    nlohmann::json& j,
    std::string assetId,
    std::filesystem::path& basePath)
{
    auto type = j["type"].get<std::string>();

    if (type == "gltf") {
        auto filename = j["source"].get<std::string>();
        auto pp = basePath.append(filename).lexically_normal();
        importGltf(importList, j, assetId, pp);
    }
#if 0
    if (type == "image") {
        auto filename = j["source"].get<std::string>();
        auto pp = basePath.append(filename).lexically_normal();

        ImportImage(importList, j, assetId, pp);
    }

    if (type == "shader") {
        auto filename = j["source"].get<std::string>();
        auto pp = basePath.append(filename).lexically_normal();

        importShader(importList, j, assetId, pp);
    }

    if (type == "texture") {
        importTexture(importList, j, assetId);
    }
#endif
    if (type == "material") {
        importMaterial(importList, j, assetId);
    }
}
#if 0
template <class Container>
void split1(const std::string& str, Container& cont)
{
    std::istringstream iss(str);
    std::copy(std::istream_iterator<std::string>(iss),
        std::istream_iterator<std::string>(),
        std::back_inserter(cont));
}
#endif

void BuildPackFile(
    std::filesystem::path packFile,
    std::vector<std::pair<std::string, nlohmann::json>>& jsonData)
{
    RXAssets::DeletePackFile(packFile);

    std::filesystem::path ixFile = packFile;
    ixFile.replace_extension("ix");
    RXAssets::AssetIndex index;
    std::ofstream file;
    file.open(packFile, std::ios::out | std::ios::binary);

    spdlog::info("Packfile = {0} index = {1}", packFile.generic_string(), ixFile.generic_string());

    uint32_t offset = 0;

    for (auto& [filename, j] : jsonData) {
        spdlog::info("Processing file - {0}", filename);

        importList il;
        std::filesystem::path fn2(filename);
        fn2.replace_extension("");
        auto parent = fn2.parent_path();

        ImportObject(il, j, fn2.filename().generic_string(), parent);

        for (auto& i : il.items) {
            spdlog::info(
                "Storing {:36s} - {:12d} bytes at offset {:12d}", i.first, i.second.size(),
                offset);
            auto& nix = index.index.emplace_back();
            nix.size = static_cast<uint32_t>(i.second.size());
            nix.offset = offset;
            nix.asset = i.first;
            offset += static_cast<uint32_t>(i.second.size());

            file.write(reinterpret_cast<char*>(i.second.data()), i.second.size());
            file.flush();
        }
        RXAssets::SaveIndex(ixFile, index);
    }
    file.close();
}

//using namespace std::chrono_literals;

void checkJsonSourceFile(
    bool& rebuild_needed,
    std::filesystem::file_time_type pack_file_time,
    const std::filesystem::directory_entry& entry)
{
    std::ifstream f(entry.path());
    nlohmann::json js;
    f >> js;
    f.close();

    if (!js["source"].is_string()) {
        return;
    }
    auto basePath = entry.path().parent_path();
    auto source = js["source"].get<std::string>();
    auto source_path = basePath.append(source).lexically_normal();

    if (std::filesystem::exists(source_path)) {
        if (std::filesystem::last_write_time(source_path) > pack_file_time) {
            rebuild_needed = true;
            spdlog::info(
                "Asset source file {} from meta {} is newer than the pack - marking for rebuild",
                source_path.generic_string(), entry.path().generic_string());
        }
    }
}

void checkJsonFile(
    bool& rebuild_needed,
    std::filesystem::file_time_type pack_file_time,
    const std::filesystem::directory_entry& entry)
{
    auto json_last_write = entry.last_write_time();

    spdlog::debug("Last Write Time = {}", json_last_write.time_since_epoch().count());

    if (json_last_write > pack_file_time) {
        spdlog::info(
            "Asset meta file {} is newer than the pack - marking for rebuild",
            entry.path().generic_string());
        rebuild_needed = true;
    }
    else {
        checkJsonSourceFile(rebuild_needed, pack_file_time, entry);
    }
}

void checkHeaderFile(
    bool& rebuild_needed,
    std::filesystem::file_time_type pack_file_time,
    const std::filesystem::directory_entry& entry)
{
    auto last_write = entry.last_write_time();

    spdlog::debug("Last Write Time = {}", last_write.time_since_epoch().count());

    if (last_write > pack_file_time) {
        spdlog::info(
            "Header file {} is newer than the pack - marking for rebuild",
            entry.path().generic_string());
        rebuild_needed = true;
    }
}
#endif

void CreateMeshCommand(args::Subparser& parser)
{
    std::filesystem::path executable_path;

    args::Group addGroup(parser, "Blah", args::Group::Validators::All);

    args::Positional<std::string> glb_path(addGroup, "glb-file", "GLTF filename");
    args::Positional<std::string> assetFolder(addGroup, "asset-dir", "Asset Folder");
    //args::PositionalList<std::string> jsonFiles(addGroup, "json", "asset desc files");

    std::vector<std::pair<std::string, nlohmann::json>> json_data;

    parser.Parse();

    std::filesystem::path asset_path(assetFolder.Get());
    std::filesystem::path gltf_path(glb_path.Get());

    gltfImport gli;

    CreateGTLFData(gltf_path.generic_string(), gli);
    for (auto& image : gli.ims) {
        if (!image.name.empty()) {
            auto image_path = asset_path / image.name;
            image_path.replace_extension(".png");
            stbi_write_png(
                image_path.generic_string().c_str(),
                image.width,
                image.height,
                4,
                image.mipLevels[0].bytes.data(),
                image.width * 4);
        }
    }

    auto mesh_path = asset_path;
    mesh_path /= gltf_path.filename();
    mesh_path.replace_extension(".mesh");
    std::ofstream fs(mesh_path, std::ios::binary);

    std::vector<uint8_t> buf;
    auto w = RxAssets::QuickSerialise(buf, gli.md);
    buf.resize(w);

    fs.write(reinterpret_cast<const char*>(buf.data()), buf.size());
    fs.close();

    //importList il;
    //importGltf(il, j, assetId, pp);
}

#if 0
void CreateCommand(args::Subparser& parser)
{
    std::filesystem::path executable_path;

#ifdef WIN32
    char fn[MAX_PATH];
    memset(fn, 0, MAX_PATH);
    GetModuleFileNameA(NULL, fn, MAX_PATH);
    executable_path = std::string(fn);

#endif
    args::Group addGroup(parser, "Blah", args::Group::Validators::All);

    args::Positional<std::string> packFile(addGroup, "pack-file", "Pack filename");
    args::Positional<std::string> assetFolder(addGroup, "asset-dir", "Asset Folder");
    //args::PositionalList<std::string> jsonFiles(addGroup, "json", "asset desc files");

    bool rebuild_needed = true;
    std::vector<std::pair<std::string, nlohmann::json>> json_data;

    parser.Parse();

    std::filesystem::path asset_path(assetFolder.Get());
    std::filesystem::path pack_file_path(packFile.Get());
    std::vector<std::filesystem::path> json_files;

    std::filesystem::file_time_type pack_file_time{};
    std::filesystem::file_time_type executable_file_time{};

    executable_file_time = std::filesystem::last_write_time(executable_path);

    if (!exists(pack_file_path)) {
        rebuild_needed = true;
    }
    else {
        pack_file_time = std::filesystem::last_write_time(pack_file_path);
        if (pack_file_time < executable_file_time) {
            rebuild_needed = true;
        }
    }

    try {
        spdlog::debug(
            "Executable Write time = {}",
            executable_file_time.time_since_epoch().count());
        spdlog::debug("Pack       Write time = {}", pack_file_time.time_since_epoch().count());

        for (auto& entry : std::filesystem::recursive_directory_iterator(asset_path)) {
            spdlog::debug("Processing {}", entry.path().generic_string());

            if (entry.is_regular_file()) {
                if (entry.path().extension() == ".json") {
                    json_files.push_back(entry.path());
                    checkJsonFile(rebuild_needed, pack_file_time, entry);
                }
                if (entry.path().extension() == ".glsl") {
                    checkHeaderFile(rebuild_needed, pack_file_time, entry);
                }
            }
        }

        if (rebuild_needed) {
            for (auto&& j : json_files) {
                std::ifstream f(j);
                nlohmann::json js;
                f >> js;
                f.close();

                json_data.emplace_back(j.generic_string(), js);
            }

            BuildPackFile(std::filesystem::path{ packFile.Get() }, json_data);
        }
    }
    catch (std::exception& e) {
        std::filesystem::remove(pack_file_path);
        throw e;
    }
}
#endif
int main(int argc, char** argv)
{
    args::ArgumentParser parser("AssetManager", "---");
    args::Group commands(parser, "commands");
    args::Command createMesh(commands, "createMesh", "Import a glb file to mesh", &CreateMeshCommand);

    //   glslang::InitializeProcess();

    //spdlog::set_level(spdlog::level::debug);
    try {
        parser.ParseCLI(argc, argv);
    }
    catch (const args::Help&) {
        std::cout << parser;
        return 0;
    }
    catch (const args::Error& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    //   glslang::FinalizeProcess();
    return EXIT_SUCCESS;
}
