#pragma once

#include "vfs_types.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace filefuse {

class VfsBackend {
public:
    virtual ~VfsBackend() = default;

    virtual VfsError connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual std::string name() const = 0;

    virtual VfsError stat(const std::string& path, VfsStat& out) = 0;
    virtual VfsError readdir(const std::string& path, std::vector<VfsDirEntry>& entries) = 0;
    virtual VfsError read(const std::string& path, uint64_t offset, size_t size,
                          std::vector<uint8_t>& out) = 0;
    virtual VfsError write(const std::string& path, uint64_t offset,
                           const uint8_t* data, size_t size) = 0;
    virtual VfsError mkdir(const std::string& path, uint32_t mode) = 0;
    virtual VfsError unlink(const std::string& path) = 0;
};

}  // namespace filefuse
