//
// Created by shane on 19/02/2021.
//

//#include "bitsery/adapter/buffer.h"
#include "dds-ktx.h"
#include "stb_image.h"
#include "Vfs.h"
#include "Loader.h"
#include "vulkan/vulkan.h"
#include <fstream>
#include "AssetException.h"

namespace RxAssets
{
    AssetException::AssetException(const std::string & what_arg, const std::string & asset)
        : runtime_error(what_arg + asset)
    {
    }
    AssetException::AssetException(const std::string & what_arg, const std::filesystem::path & asset)
        : runtime_error(what_arg + asset.generic_string())
    {

    }
}