//
// Created by shane on 19/02/2021.
//

#include "RmlFileInterface.h"

namespace RxEngine
{
    Rml::FileHandle RmlFileInterface::Open(const Rml::String & path)
    {
        auto size = vfs_->getFilesize(path);
        if (size == 0) {
            return 0;
        }

        auto handle = nextHandle++;

        auto[iterator, r] = fileData.emplace(handle, FileHandle{});
        assert(r);

        iterator->second.size = size;
        iterator->second.pointer = 0;
        iterator->second.buffer.resize(size);
        vfs_->getFileContents(path, iterator->second.buffer.data());

        return handle;
    }

    void RmlFileInterface::Close(Rml::FileHandle file)
    {
        fileData.erase(file);
    }

    size_t RmlFileInterface::Read(void * buffer, size_t size, Rml::FileHandle file)
    {
        assert(fileData.contains(file));

        auto & fd = fileData[file];

        if (!(fd.pointer + size <= fd.size)) {
            size = fd.size - fd.pointer;
        }

        memcpy(buffer, fd.buffer.data() + fd.pointer, size);
        fd.pointer += size;

        return size;
    }

    bool RmlFileInterface::Seek(Rml::FileHandle file, long offset, int origin)
    {
        assert(fileData.contains(file));

        auto & fd = fileData[file];

        switch (origin) {
            case SEEK_CUR:
                if (offset <= static_cast<long>(fd.size)) {
                    fd.pointer = offset;
                    return true;
                }
                return false;
            case SEEK_END:
                fd.pointer = fd.size - offset;
                return true;
            case SEEK_SET:
                if (offset <= static_cast<long>(fd.size)) {
                    fd.pointer = offset;
                    return true;
                }
                return false;
            default:
                return false;
        }
    }

    size_t RmlFileInterface::Tell(Rml::FileHandle file)
    {
        assert(fileData.contains(file));

        auto & fd = fileData[file];

        return fd.pointer;
    }

    RmlFileInterface::RmlFileInterface()
    {
        vfs_ = RxAssets::Vfs::getInstance();
    }
}