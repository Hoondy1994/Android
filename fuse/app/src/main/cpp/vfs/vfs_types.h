#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace filefuse {

enum class VfsNodeType { File, Directory, Symlink, Unknown };

struct VfsStat {
    VfsNodeType type = VfsNodeType::Unknown;
    uint64_t size = 0;
    uint32_t mode = 0;
    uint32_t uid = 0;
    uint32_t gid = 0;
    int64_t mtime = 0;
};

struct VfsDirEntry {
    std::string name;
    VfsNodeType type = VfsNodeType::Unknown;
};

enum class VfsError {
    Ok = 0,
    NotFound = -2,
    PermissionDenied = -13,
    Exists = -17,
    NotDirectory = -20,
    IsDirectory = -21,
    Io = -5,
    NotConnected = -100,
    Unsupported = -101,
};

inline int toErrno(VfsError error) {
    return static_cast<int>(error);
}

}  // namespace filefuse
