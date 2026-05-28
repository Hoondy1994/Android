#pragma once

#include "../vfs/vfs_backend.h"

#include <string>

namespace filefuse {

class LocalBackend : public VfsBackend {
public:
    explicit LocalBackend(std::string rootPath);

    VfsError connect() override;
    void disconnect() override;
    bool isConnected() const override;
    std::string name() const override;

    VfsError stat(const std::string& path, VfsStat& out) override;
    VfsError readdir(const std::string& path, std::vector<VfsDirEntry>& entries) override;
    VfsError read(const std::string& path, uint64_t offset, size_t size,
                  std::vector<uint8_t>& out) override;
    VfsError write(const std::string& path, uint64_t offset, const uint8_t* data,
                   size_t size) override;
    VfsError mkdir(const std::string& path, uint32_t mode) override;
    VfsError unlink(const std::string& path) override;

private:
    std::string rootPath_;
    bool connected_ = false;

    std::string resolvePhysicalPath(const std::string& virtualPath) const;
};

}  // namespace filefuse
