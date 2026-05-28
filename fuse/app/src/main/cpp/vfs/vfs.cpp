#include "vfs.h"

#include <algorithm>

namespace filefuse {

namespace {

std::string trimSlashes(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && value[start] == '/') {
        ++start;
    }
    size_t end = value.size();
    while (end > start && value[end - 1] == '/') {
        --end;
    }
    return value.substr(start, end - start);
}

}  // namespace

VfsError VirtualFileSystem::mount(std::unique_ptr<VfsBackend> backend) {
    if (!backend) {
        return VfsError::Io;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (backend_) {
        backend_->disconnect();
        backend_.reset();
    }

    const VfsError result = backend->connect();
    if (result != VfsError::Ok) {
        backend->disconnect();
        return result;
    }

    backend_ = std::move(backend);
    return VfsError::Ok;
}

void VirtualFileSystem::unmount() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (backend_) {
        backend_->disconnect();
        backend_.reset();
    }
}

bool VirtualFileSystem::isMounted() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return backend_ != nullptr && backend_->isConnected();
}

std::string VirtualFileSystem::backendName() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return backend_ ? backend_->name() : "none";
}

std::string VirtualFileSystem::normalizePath(const std::string& path) {
    if (path.empty() || path == "/") {
        return "/";
    }

    std::string normalized = "/";
    normalized += trimSlashes(path);
    return normalized;
}

VfsError VirtualFileSystem::stat(const std::string& path, VfsStat& out) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!backend_) {
        return VfsError::NotConnected;
    }
    return backend_->stat(normalizePath(path), out);
}

VfsError VirtualFileSystem::readdir(const std::string& path,
                                    std::vector<VfsDirEntry>& entries) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!backend_) {
        return VfsError::NotConnected;
    }
    return backend_->readdir(normalizePath(path), entries);
}

VfsError VirtualFileSystem::read(const std::string& path, uint64_t offset, size_t size,
                                 std::vector<uint8_t>& out) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!backend_) {
        return VfsError::NotConnected;
    }
    return backend_->read(normalizePath(path), offset, size, out);
}

VfsError VirtualFileSystem::write(const std::string& path, uint64_t offset,
                                  const uint8_t* data, size_t size) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!backend_) {
        return VfsError::NotConnected;
    }
    return backend_->write(normalizePath(path), offset, data, size);
}

VfsError VirtualFileSystem::mkdir(const std::string& path, uint32_t mode) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!backend_) {
        return VfsError::NotConnected;
    }
    return backend_->mkdir(normalizePath(path), mode);
}

VfsError VirtualFileSystem::unlink(const std::string& path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!backend_) {
        return VfsError::NotConnected;
    }
    return backend_->unlink(normalizePath(path));
}

VirtualFileSystem& globalVfs() {
    static VirtualFileSystem instance;
    return instance;
}

}  // namespace filefuse
