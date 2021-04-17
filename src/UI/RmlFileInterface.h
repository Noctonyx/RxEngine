//
// Created by shane on 19/02/2021.
//

#ifndef RX_RMLFILEINTERFACE_H
#define RX_RMLFILEINTERFACE_H

#include <unordered_map>
#include <RmlUi/Core/FileInterface.h>
#include "Vfs.h"

namespace RxEngine
{
    class EngineMain;

    struct FileHandle {
        std::vector<std::byte> buffer;
        size_t size;
        size_t pointer;
    };

    class RmlFileInterface: public Rml::FileInterface
    {
    public:
        RmlFileInterface();

        Rml::FileHandle Open(const Rml::String & path) override;
        void Close(Rml::FileHandle file) override;
        size_t Read(void * buffer, size_t size, Rml::FileHandle file) override;
        bool Seek(Rml::FileHandle file, long offset, int origin) override;
        size_t Tell(Rml::FileHandle file) override;

    private:
        //EngineMain * engine_;
        RxAssets::Vfs * vfs_;
        uintptr_t nextHandle = 1;
        std::unordered_map<uintptr_t , FileHandle> fileData;
    };
}
#endif //RX_RMLFILEINTERFACE_H
