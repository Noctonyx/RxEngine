//
// Created by shane on 19/02/2021.
//

#ifndef RX_ASSETEXCEPTION_H
#define RX_ASSETEXCEPTION_H

#include <filesystem>
#include "SerialisationData.h"
#include "nlohmann/json.hpp"

namespace RxAssets
{

    class AssetException : public std::runtime_error
    {
    public:
        AssetException(const std::string & what_arg, const std::string & asset);
        AssetException(const std::string & what_arg, const std::filesystem::path & asset);

        std::string asset;
    };
}

#endif //RX_ASSETEXCEPTION_H
