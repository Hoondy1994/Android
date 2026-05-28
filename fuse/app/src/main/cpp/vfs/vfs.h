#pragma once

#include "vfs_backend.h"

#include <memory>
#include <mutex>
#include <string>

namespace filefuse {

class VirtualFileSystem {
public:
    VfsError mount(std::unique_ptr<VfsBackend> backend);
    void unmount();
    bool isMounted() const;
    std::string backendName() const;

    VfsError stat(const std::string& path, VfsStat& out) const;
    VfsError readdir(const std::string& path, std::vector<VfsDirEntry>& entries) const;
    VfsError read(const std::string& path, uint64_t offset, size_t size,
                  std::vector<uint8_t>& out) const;
    VfsError write(const std::string& path, uint64_t offset, const uint8_t* data,
                   size_t size) const;
    VfsError mkdir(const std::string& path, uint32_t mode) const;
    VfsError unlink(const std::string& path) const;

    static std::string normalizePath(const std::string& path);

private:
    mutable std::mutex mutex_;
    std::unique_ptr<VfsBackend> backend_;
};

VirtualFileSystem& globalVfs();

}  // namespace filefuse
