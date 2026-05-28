#include "fuse_adapter.h"

#include <cstring>

namespace filefuse {

namespace {

int vfsErrorToErrno(VfsError error) {
    return toErrno(error);
}

}  // namespace

FuseAdapter::FuseAdapter(VirtualFileSystem& vfs) : vfs_(vfs) {}

void FuseAdapter::fillStat(const VfsStat& source, struct stat* target) {
    std::memset(target, 0, sizeof(struct stat));
    switch (source.type) {
        case VfsNodeType::Directory:
            target->st_mode = S_IFDIR | (source.mode & 0777);
            break;
        case VfsNodeType::File:
            target->st_mode = S_IFREG | (source.mode & 0777);
            break;
        case VfsNodeType::Symlink:
            target->st_mode = S_IFLNK | (source.mode & 0777);
            break;
        default:
            target->st_mode = source.mode;
            break;
    }
    target->st_size = static_cast<off_t>(source.size);
    target->st_uid = static_cast<uid_t>(source.uid);
    target->st_gid = static_cast<gid_t>(source.gid);
    target->st_mtime = static_cast<time_t>(source.mtime);
    target->st_nlink = 1;
}

int FuseAdapter::getattr(const std::string& path, struct stat* stbuf) const {
    VfsStat stat {};
    const VfsError result = vfs_.stat(path, stat);
    if (result != VfsError::Ok) {
        return vfsErrorToErrno(result);
    }
    fillStat(stat, stbuf);
    return 0;
}

int FuseAdapter::readdir(const std::string& path, std::vector<FuseDirEntry>& entries) const {
    std::vector<VfsDirEntry> vfsEntries;
    const VfsError result = vfs_.readdir(path, vfsEntries);
    if (result != VfsError::Ok) {
        return vfsErrorToErrno(result);
    }

    entries.clear();
    for (const VfsDirEntry& entry : vfsEntries) {
        FuseDirEntry fuseEntry;
        fuseEntry.name = entry.name;
        switch (entry.type) {
            case VfsNodeType::Directory:
                fuseEntry.type = 4;  // DT_DIR
                break;
            case VfsNodeType::File:
                fuseEntry.type = 8;  // DT_REG
                break;
            case VfsNodeType::Symlink:
                fuseEntry.type = 10;  // DT_LNK
                break;
            default:
                fuseEntry.type = 0;
                break;
        }
        entries.push_back(std::move(fuseEntry));
    }
    return 0;
}

int FuseAdapter::read(const std::string& path, char* buffer, size_t size, off_t offset) const {
    std::vector<uint8_t> data;
    const VfsError result = vfs_.read(path, static_cast<uint64_t>(offset), size, data);
    if (result != VfsError::Ok) {
        return vfsErrorToErrno(result);
    }
    if (!data.empty()) {
        std::memcpy(buffer, data.data(), data.size());
    }
    return static_cast<int>(data.size());
}

int FuseAdapter::write(const std::string& path, const char* buffer, size_t size,
                       off_t offset) const {
    const VfsError result =
            vfs_.write(path, static_cast<uint64_t>(offset),
                       reinterpret_cast<const uint8_t*>(buffer), size);
    if (result != VfsError::Ok) {
        return vfsErrorToErrno(result);
    }
    return static_cast<int>(size);
}

int FuseAdapter::mkdir(const std::string& path, mode_t mode) const {
    const VfsError result = vfs_.mkdir(path, static_cast<uint32_t>(mode & 0777));
    return result == VfsError::Ok ? 0 : vfsErrorToErrno(result);
}

int FuseAdapter::unlink(const std::string& path) const {
    const VfsError result = vfs_.unlink(path);
    return result == VfsError::Ok ? 0 : vfsErrorToErrno(result);
}

}  // namespace filefuse
