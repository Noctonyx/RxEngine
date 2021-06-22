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
// Created by shane on 16/02/2021.
//

#include "Vfs.h"
#include <fstream>
#include "miniz.h"
#include "Log.h"
#include "AssetException.h"


namespace RxAssets
{
    Vfs * Vfs::instance_ = nullptr;

    void Vfs::addMount(const std::filesystem::path & path, const std::string & mountPoint)
    {
        mounts_.emplace_back(Mount(path, mountPoint));
    }

    void Vfs::scan()
    {
        clearCatalog();

        for (uint32_t ix = 0; auto & m: mounts_) {
            if (is_directory(m.path)) {
                m.isZip = false;
                loadDirectoryCatalog(m.path, m.mountPoint, ix);
            }

            if (is_regular_file(m.path)) {
                m.isZip = true;
                loadZipCatalog(m.path, m.mountPoint, ix, m);
            }
            ix++;
        }

        for (auto & c: catalog_) {
            spdlog::debug("{} {}", c.name, c.relativePath.generic_string().c_str());
        }
    }

    void Vfs::monitorForChanges()
    {
        static constexpr std::size_t _buffer_size = {1024 * 256};
        monData.clear();

        for (auto & m: mounts_) {
            if (is_directory(m.path)) {
                monData.push_back({
                    m.path,
                });
            }
        }
        closeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        for (auto & md: monData) {
            md.buffer.resize(_buffer_size);
            md.overlapped_buffer.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

            md.directory = CreateFileA(
                md.path.generic_string().c_str(), // pointer to the file name
                FILE_LIST_DIRECTORY, // access (read/write) mode
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, // share mode
                nullptr, // security descriptor
                OPEN_EXISTING, // how to create
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, // file attributes
                HANDLE(0)); // file with attributes to copy
        }

        monitorThread = (std::thread([this]()
            {
                std::vector<HANDLE> events;
                for (auto& md : monData) {
                    events.push_back(md.overlapped_buffer.hEvent);
                }
                events.push_back(closeEvent);

                while (true) {
                    for (auto& md : monData) {
                        ReadDirectoryChangesW(
                            md.directory,
                            md.buffer.data(), static_cast<DWORD>(md.buffer.size()),
                            TRUE,
                            FILE_NOTIFY_CHANGE_SECURITY |
                            FILE_NOTIFY_CHANGE_CREATION |
                            FILE_NOTIFY_CHANGE_LAST_ACCESS |
                            FILE_NOTIFY_CHANGE_LAST_WRITE |
                            FILE_NOTIFY_CHANGE_SIZE |
                            FILE_NOTIFY_CHANGE_ATTRIBUTES |
                            FILE_NOTIFY_CHANGE_DIR_NAME |
                            FILE_NOTIFY_CHANGE_FILE_NAME,
                            &md.bytes_returned,
                            &md.overlapped_buffer, NULL);
                    }

                    auto res = WaitForMultipleObjects(static_cast<DWORD>(events.size()), events.data(), FALSE, INFINITE);
                    if(res == WAIT_FAILED) {
                        return;
                    }

                    res -= WAIT_OBJECT_0;
                    if(res == events.size() - 1) {
                        return;
                    }

                    auto& ad = monData[res];

                    if (!GetOverlappedResult(ad.directory, &ad.overlapped_buffer, &ad.bytes_returned,TRUE)) {
                        throw std::system_error(GetLastError(), std::system_category());
                    }
                    if (ad.bytes_returned > 0) {
                        changed = true;
                    }
                }
            }));        
    }

    bool Vfs::hasChanged()
    {
        auto b = changed;
        changed = false;
        return b;
    }

    void Vfs::stopMonitor()
    {
        SetEvent(closeEvent);
        monitorThread.join();
        for (auto& md : monData) {
            CancelIo(md.directory);
            GetOverlappedResult(md.directory, &md.overlapped_buffer, &md.bytes_returned, TRUE);
            CloseHandle(md.directory);
        }

        CloseHandle(closeEvent);
    }

    void Vfs::clearCatalog()
    {
        catalogIndex_.clear();
        catalog_.clear();
    }

    void Vfs::loadDirectoryCatalog(const std::filesystem::path & path,
                                   const std::string & mount,
                                   uint32_t mountIndex)
    {
        for (auto & entry: std::filesystem::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                auto r = entry.path().lexically_relative(path);
                if (entry.path().generic_string().find(".git") != std::string::npos) {
                    continue;
                }
                auto extension = entry.path().extension().generic_string();
                std::ranges::transform(
                    extension,
                    extension.begin(),
                    [](char c)
                    {
                        return static_cast<char>(::tolower(c));
                    }
                );
                //r.replace_extension(extension);

                if (extension != ".png" && extension != ".dds" && extension != ".spv" && extension
                    != ".lua" &&
                    extension != ".mesh" && extension != ".tex" && extension != ".json") {
                    //                    continue;
                }
                auto ap = (std::filesystem::path(mount) / r);
                ap.replace_extension(extension);
                auto asset_path = (ap).generic_string();

                std::ranges::transform(
                    asset_path,
                    asset_path.begin(),
                    [](char c)
                    {
                        if (c == '\\') {
                            return '/';
                        }
                        return c;
                    });

                auto fix = static_cast<uint32_t>(catalog_.size());

                catalog_.emplace_back(
                    CatalogEntry{
                        asset_path,
                        r,
                        mountIndex,
                        //static_cast<uint32_t>(entry.file_size()),
                        0
                    });
                catalogIndex_.insert_or_assign(asset_path, fix);
            }
        }
    }

    void Vfs::loadZipCatalog(
        const std::filesystem::path & path,
        const std::string & mountPoint,
        uint32_t mountIndex,
        Mount & mount)
    {
        memset(&mount.zip, 0, sizeof(mount.zip));
        mount.zip.m_pAlloc = [](void * memctx, size_t items, size_t size) -> void*
        {
            return malloc(size * items);
        };
        mount.zip.m_pRealloc = [](void * memctx, void * ptr, size_t items, size_t size) -> void*
        {
            return realloc(ptr, items * size);
        };
        mount.zip.m_pFree = [](void * memctx, void * ptr) -> void
        {
            free(ptr);
        };

        auto status = mz_zip_reader_init_file(&mount.zip, path.generic_string().c_str(), 0);
        if (!status) {
            throw AssetException("cannot open zip file", path);
        }

        auto count = mz_zip_reader_get_num_files(&mount.zip);
        for (uint32_t i = 0; i < count; ++i) {

            char filename[1024];
            mz_zip_reader_get_filename(&mount.zip, i, filename, sizeof(filename));

            if (!mz_zip_reader_is_file_a_directory(&mount.zip, i)) {
                mz_zip_archive_file_stat stat;
                const mz_bool result = mz_zip_reader_file_stat(&mount.zip, i, &stat);
                assert(result);
                if (!result) {
                    throw AssetException("Unable to read zip file", path);
                }

                auto rel_path = std::filesystem::path(filename);
                auto asset_path = std::filesystem::path(mountPoint) / rel_path;

                auto extension = asset_path.extension().generic_string();
                std::ranges::transform(
                    extension,
                    extension.begin(),
                    [](const char c)
                    {
                        return static_cast<char>(::tolower(c));
                    }
                );

                if (extension != ".png" && extension != ".dds" && extension != ".spv" && extension
                    != ".lua" &&
                    extension != ".mesh" && extension != ".tex" && extension != ".json") {
                    //  continue;
                }
                auto fix = static_cast<uint32_t>(catalog_.size());
                catalog_.emplace_back(
                    CatalogEntry{
                        asset_path.generic_string(),
                        "",
                        mountIndex,/* static_cast<uint32_t>(stat.m_uncomp_size),*/ stat.m_file_index
                    });
                catalogIndex_.insert_or_assign(asset_path.generic_string(), fix);
            }
        }
    }

    void Vfs::writeFile(const std::filesystem::path & path,
                        std::byte * data,
                        const size_t size) const
    {
        std::filesystem::path file_path;

        if (path.is_absolute()) {
            auto r = path.lexically_relative("/");
            file_path = writeableDirectory_ / r;
        } else {
            file_path = writeableDirectory_ / path;
        }

        std::ofstream fs(file_path, std::ios::binary | std::ios::out);
        fs.write(reinterpret_cast<const char *>(data), size);
        fs.close();
    }

    std::optional<size_t> Vfs::getFilesize(const std::filesystem::path & path) const
    {
        auto [name, relativePath, mountIndex, zipIndex] = getCatalogEntry(path);
        auto mount = mounts_[mountIndex];

        if (mount.isZip) {
            mz_zip_archive_file_stat stat;

            if (!mz_zip_reader_file_stat(&mount.zip, zipIndex, &stat)) {
                return std::nullopt;
            }

            if (relativePath != stat.m_filename) {
                return std::nullopt;
            }
            return stat.m_uncomp_size;
        }

        auto full_path = mount.path / relativePath;
        if (std::filesystem::is_regular_file(full_path)) {
            return std::filesystem::file_size(full_path);
        }
        return std::nullopt;
    }

    size_t Vfs::getFileContents(const std::filesystem::path & path, std::byte * data) const
    {
        auto [name, relativePath, mountIndex, zipIndex] = getCatalogEntry(path);
        auto mount = mounts_[mountIndex];

        if (mount.isZip) {
            mz_zip_archive_file_stat stat;

            if (!mz_zip_reader_file_stat(&mount.zip, zipIndex, &stat)) {
                return 0;
            }

            if (relativePath != stat.m_filename) {
                return 0;
            }
            return getZipAsset(mount, zipIndex, data, stat.m_uncomp_size);
        }

        auto full_path = mount.path / relativePath;
        return getFileAsset(full_path, data);
    }

    bool Vfs::assetExists(const std::filesystem::path & path) const
    {
        if(!catalogIndex_.contains(path.generic_string())) {
            return false;
        }
        return true;
    }

    CatalogEntry Vfs::getCatalogEntry(const std::filesystem::path & path) const
    {
        if (!catalogIndex_.contains(path.generic_string())) {
            throw AssetException("Asset not found in catalog: ", path);
        }

        const auto ix = catalogIndex_.at(path.generic_string());
        return catalog_[ix];
    }

    size_t Vfs::getZipAsset(Mount & mount, uint32_t zipIndex, std::byte * data, const size_t size)
    {
        mz_zip_reader_extract_to_mem_no_alloc(&mount.zip, zipIndex, data, size, 0, nullptr, 0);

        return size;
    }

    size_t Vfs::getFileAsset(const std::filesystem::path & path, std::byte * data)
    {
        FILE * fp = fopen(path.generic_string().c_str(), "rb");
        if (!fp) {
            return 0;
        }

        fseek(fp, 0, SEEK_END);
        const int file_size = static_cast<int>(ftell(fp));
        fseek(fp, 0, SEEK_SET);

        const int size_read = static_cast<int>(fread(data, 1, static_cast<size_t>(file_size), fp));

        fclose(fp);

        return size_read;
    }

    std::string Vfs::getAssetAsString(const std::filesystem::path & path) const
    {
        const auto size = getFilesize(path);
        if (size.has_value()) {
            std::vector<std::byte> data(size.value());
            std::string s;
            s.resize(size.value());
            getFileContents(path, reinterpret_cast<std::byte *>(s.data()));
            return s;
        }
        return std::string();
    }

    Vfs * vfs()
    {
        return Vfs::getInstance();
    }
}
