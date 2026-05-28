#include "local_backend.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <fstream>

namespace filefuse {

namespace {

VfsNodeType modeToType(mode_t mode) {
    if (S_ISDIR(mode)) {
        return VfsNodeType::Directory;
    }
    if (S_ISREG(mode)) {
        return VfsNodeType::File;
    }
    if (S_ISLNK(mode)) {
        return VfsNodeType::Symlink;
    }
    return VfsNodeType::Unknown;
}

VfsError errnoToVfsError(int err) {
    switch (err) {
        case ENOENT:
            return VfsError::NotFound;
        case EACCES:
        case EPERM:
            return VfsError::PermissionDenied;
        case EEXIST:
            return VfsError::Exists;
        case ENOTDIR:
            return VfsError::NotDirectory;
        case EISDIR:
            return VfsError::IsDirectory;
        default:
            return VfsError::Io;
    }
}

}  // namespace

LocalBackend::LocalBackend(std::string rootPath) : rootPath_(std::move(rootPath)) {}

VfsError LocalBackend::connect() {
    struct stat st {};
    if (::stat(rootPath_.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        connected_ = false;
        return VfsError::Io;
    }
    connected_ = true;
    return VfsError::Ok;
}

void LocalBackend::disconnect() {
    connected_ = false;
}

bool LocalBackend::isConnected() const {
    return connected_;
}

std::string LocalBackend::name() const {
    return "local";
}

std::string LocalBackend::resolvePhysicalPath(const std::string& virtualPath) const {
    if (virtualPath == "/") {
        return rootPath_;
    }
    return rootPath_ + virtualPath;
}

VfsError LocalBackend::stat(const std::string& path, VfsStat& out) {
    struct stat st {};
    if (::stat(resolvePhysicalPath(path).c_str(), &st) != 0) {
        return errnoToVfsError(errno);
    }

    out.type = modeToType(st.st_mode);
    out.size = static_cast<uint64_t>(st.st_size);
    out.mode = static_cast<uint32_t>(st.st_mode & 0777);
    out.uid = static_cast<uint32_t>(st.st_uid);
    out.gid = static_cast<uint32_t>(st.st_gid);
    out.mtime = static_cast<int64_t>(st.st_mtime);
    return VfsError::Ok;
}

VfsError LocalBackend::readdir(const std::string& path, std::vector<VfsDirEntry>& entries) {
    const std::string physical = resolvePhysicalPath(path);
    DIR* dir = opendir(physical.c_str());
    if (!dir) {
        return errnoToVfsError(errno);
    }

    entries.clear();
    while (dirent* entry = ::readdir(dir)) {
        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        VfsDirEntry item;
        item.name = entry->d_name;
        if (entry->d_type == DT_DIR) {
            item.type = VfsNodeType::Directory;
        } else if (entry->d_type == DT_REG) {
            item.type = VfsNodeType::File;
        } else if (entry->d_type == DT_LNK) {
            item.type = VfsNodeType::Symlink;
        } else {
            struct stat st {};
            const std::string childPath = physical + "/" + entry->d_name;
            if (::stat(childPath.c_str(), &st) == 0) {
                item.type = modeToType(st.st_mode);
            }
        }
        entries.push_back(std::move(item));
    }

    closedir(dir);
    return VfsError::Ok;
}

VfsError LocalBackend::read(const std::string& path, uint64_t offset, size_t size,
                            std::vector<uint8_t>& out) {
    std::ifstream input(resolvePhysicalPath(path), std::ios::binary);
    if (!input) {
        return VfsError::NotFound;
    }

    input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    out.assign(size, 0);
    input.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(size));
    out.resize(static_cast<size_t>(input.gcount()));
    return VfsError::Ok;
}

VfsError LocalBackend::write(const std::string& path, uint64_t offset, const uint8_t* data,
                             size_t size) {
    std::fstream output(resolvePhysicalPath(path),
                        std::ios::binary | std::ios::in | std::ios::out);
    if (!output) {
        output.open(resolvePhysicalPath(path), std::ios::binary | std::ios::out);
    }
    if (!output) {
        return VfsError::Io;
    }

    output.seekp(static_cast<std::streamoff>(offset), std::ios::beg);
    output.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    return output.good() ? VfsError::Ok : VfsError::Io;
}

VfsError LocalBackend::mkdir(const std::string& path, uint32_t mode) {
    if (::mkdir(resolvePhysicalPath(path).c_str(), static_cast<mode_t>(mode)) != 0) {
        return errnoToVfsError(errno);
    }
    return VfsError::Ok;
}

VfsError LocalBackend::unlink(const std::string& path) {
    const std::string physical = resolvePhysicalPath(path);
    struct stat st {};
    if (::stat(physical.c_str(), &st) != 0) {
        return errnoToVfsError(errno);
    }

    if (S_ISDIR(st.st_mode)) {
        if (rmdir(physical.c_str()) != 0) {
            return errnoToVfsError(errno);
        }
    } else if (::unlink(physical.c_str()) != 0) {
        return errnoToVfsError(errno);
    }
    return VfsError::Ok;
}

}  // namespace filefuse
