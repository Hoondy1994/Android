#pragma once

#include "../../vfs/vfs_backend.h"
#include "nfs_rpc.h"

#include <map>
#include <string>

namespace filefuse {

class NfsBackend : public VfsBackend {
public:
    NfsBackend(std::string host, std::string exportPath, uint16_t nfsPort = 2049);

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
    std::string host_;
    std::string exportPath_;
    uint16_t nfsPort_;
    bool connected_ = false;

    RpcClient mountRpc_;
    RpcClient nfsRpc_;
    std::vector<uint8_t> rootHandle_;
    std::map<std::string, std::vector<uint8_t>> handleCache_;

    bool lookupPort(uint16_t portmapperPort, uint32_t program, uint32_t version,
                    uint16_t& outPort);
    bool mountExport();
    bool lookupHandle(const std::string& path, std::vector<uint8_t>& handle);
    bool lookupPathComponents(const std::vector<std::string>& components,
                              std::vector<uint8_t>& handle, VfsStat* statOut);
    bool nfsGetAttr(const std::vector<uint8_t>& handle, VfsStat& out);
    VfsError nfsStatusToError(uint32_t status) const;

    static std::vector<std::string> splitPath(const std::string& path);
};

}  // namespace filefuse
