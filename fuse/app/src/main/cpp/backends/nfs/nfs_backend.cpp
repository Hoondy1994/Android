#include "nfs_backend.h"

#include <android/log.h>

#include <algorithm>

#define NFS_LOG(...) __android_log_print(ANDROID_LOG_INFO, "filefuse-nfs", __VA_ARGS__)

namespace filefuse {

namespace {

constexpr uint32_t kPortmapperProgram = 100000;
constexpr uint32_t kPortmapperVersion = 2;
constexpr uint32_t kPortmapperGetPort = 3;

constexpr uint32_t kMountProgram = 100005;
constexpr uint32_t kMountVersion = 3;
constexpr uint32_t kMountMnt = 1;

constexpr uint32_t kNfsProgram = 100003;
constexpr uint32_t kNfsVersion = 3;
constexpr uint32_t kNfsProcGetAttr = 1;
constexpr uint32_t kNfsProcLookup = 3;
constexpr uint32_t kNfsProcRead = 6;
constexpr uint32_t kNfsProcReadDir = 16;

constexpr uint32_t kNfsOk = 0;
constexpr uint32_t kNfsErrNoEnt = 2;
constexpr uint32_t kNfsErrAcces = 13;
constexpr uint32_t kNfsErrExist = 17;
constexpr uint32_t kNfsErrNotDir = 20;
constexpr uint32_t kNfsErrIsDir = 21;
constexpr uint32_t kNfsErrNotSupp = 10004;

void encodeFhandle(XdrWriter& writer, const std::vector<uint8_t>& handle) {
    writer.writeOpaque(handle);
}

void encodeDirPath(XdrWriter& writer, const std::string& path) {
    if (path == "/") {
        writer.writeUint32(0);
        return;
    }

    std::string current;
    size_t start = 1;
    while (start <= path.size()) {
        size_t end = path.find('/', start);
        if (end == std::string::npos) {
            end = path.size();
        }
        if (end > start) {
            writer.writeString(path.substr(start, end - start));
        }
        start = end + 1;
    }
    writer.writeUint32(0);
}

VfsNodeType nfsTypeToNodeType(uint32_t type) {
    switch (type) {
        case 1:
            return VfsNodeType::File;
        case 2:
            return VfsNodeType::Directory;
        case 5:
            return VfsNodeType::Symlink;
        default:
            return VfsNodeType::Unknown;
    }
}

}  // namespace

NfsBackend::NfsBackend(std::string host, std::string exportPath, uint16_t nfsPort)
    : host_(std::move(host)), exportPath_(std::move(exportPath)), nfsPort_(nfsPort) {}

VfsError NfsBackend::connect() {
    disconnect();

    uint16_t mountPort = 0;
    RpcClient portmapper;
    if (!portmapper.connect(host_, 111)) {
        NFS_LOG("portmapper connect failed");
        return VfsError::Io;
    }

    XdrWriter getPortArgs;
    getPortArgs.writeUint32(kMountProgram);
    getPortArgs.writeUint32(kMountVersion);
    getPortArgs.writeUint32(6);
    getPortArgs.writeUint32(0);

    std::vector<uint8_t> getPortReply;
    if (!portmapper.call(kPortmapperProgram, kPortmapperVersion, kPortmapperGetPort,
                         getPortArgs.data(), getPortReply)) {
        NFS_LOG("GETPORT for mount failed");
        return VfsError::Io;
    }

    XdrReader getPortReader(getPortReply.data(), getPortReply.size());
    uint32_t mountPortNet = 0;
    if (!getPortReader.readUint32(mountPortNet) || mountPortNet == 0) {
        NFS_LOG("mount port unavailable");
        return VfsError::Io;
    }
    mountPort = static_cast<uint16_t>(mountPortNet);
    portmapper.close();

    if (!mountRpc_.connect(host_, mountPort)) {
        NFS_LOG("mount RPC connect failed");
        return VfsError::Io;
    }

    if (!mountExport()) {
        NFS_LOG("mount export failed");
        return VfsError::Io;
    }
    mountRpc_.close();

    if (!nfsRpc_.connect(host_, nfsPort_)) {
        NFS_LOG("NFS RPC connect failed");
        return VfsError::Io;
    }

    handleCache_["/"] = rootHandle_;
    connected_ = true;
    return VfsError::Ok;
}

void NfsBackend::disconnect() {
    mountRpc_.close();
    nfsRpc_.close();
    rootHandle_.clear();
    handleCache_.clear();
    connected_ = false;
}

bool NfsBackend::isConnected() const {
    return connected_;
}

std::string NfsBackend::name() const {
    return "nfs";
}

bool NfsBackend::lookupPort(uint16_t portmapperPort, uint32_t program, uint32_t version,
                            uint16_t& outPort) {
    RpcClient portmapper;
    if (!portmapper.connect(host_, portmapperPort)) {
        return false;
    }

    XdrWriter args;
    args.writeUint32(program);
    args.writeUint32(version);
    args.writeUint32(6);
    args.writeUint32(0);

    std::vector<uint8_t> reply;
    if (!portmapper.call(kPortmapperProgram, kPortmapperVersion, kPortmapperGetPort,
                         args.data(), reply)) {
        return false;
    }

    XdrReader reader(reply.data(), reply.size());
    uint32_t port = 0;
    if (!reader.readUint32(port) || port == 0) {
        return false;
    }
    outPort = static_cast<uint16_t>(port);
    return true;
}

bool NfsBackend::mountExport() {
    XdrWriter args;
    args.writeString(exportPath_);

    std::vector<uint8_t> reply;
    if (!mountRpc_.call(kMountProgram, kMountVersion, kMountMnt, args.data(), reply)) {
        return false;
    }

    XdrReader reader(reply.data(), reply.size());
    uint32_t status = 0;
    if (!reader.readUint32(status) || status != 0) {
        return false;
    }
    return reader.readOpaque(rootHandle_);
}

std::vector<std::string> NfsBackend::splitPath(const std::string& path) {
    std::vector<std::string> parts;
    if (path == "/") {
        return parts;
    }

    size_t start = 1;
    while (start < path.size()) {
        size_t end = path.find('/', start);
        if (end == std::string::npos) {
            end = path.size();
        }
        if (end > start) {
            parts.push_back(path.substr(start, end - start));
        }
        start = end + 1;
    }
    return parts;
}

VfsError NfsBackend::nfsStatusToError(uint32_t status) const {
    switch (status) {
        case kNfsErrNoEnt:
            return VfsError::NotFound;
        case kNfsErrAcces:
            return VfsError::PermissionDenied;
        case kNfsErrExist:
            return VfsError::Exists;
        case kNfsErrNotDir:
            return VfsError::NotDirectory;
        case kNfsErrIsDir:
            return VfsError::IsDirectory;
        case kNfsErrNotSupp:
            return VfsError::Unsupported;
        default:
            return VfsError::Io;
    }
}

bool NfsBackend::nfsGetAttr(const std::vector<uint8_t>& handle, VfsStat& out) {
    XdrWriter args;
    encodeFhandle(args, handle);

    std::vector<uint8_t> reply;
    if (!nfsRpc_.call(kNfsProgram, kNfsVersion, kNfsProcGetAttr, args.data(), reply)) {
        return false;
    }

    XdrReader reader(reply.data(), reply.size());
    uint32_t status = 0;
    if (!reader.readUint32(status) || status != kNfsOk) {
        return false;
    }

    uint32_t type = 0;
    uint32_t mode = 0;
    uint32_t nlink = 0;
    uint32_t uid = 0;
    uint32_t gid = 0;
    uint64_t size = 0;
    uint64_t used = 0;
    uint32_t rdevMajor = 0;
    uint32_t rdevMinor = 0;
    uint64_t fsid = 0;
    uint64_t fileid = 0;
    uint32_t atimeSec = 0;
    uint32_t atimeNsec = 0;
    uint32_t mtimeSec = 0;
    uint32_t mtimeNsec = 0;
    uint32_t ctimeSec = 0;
    uint32_t ctimeNsec = 0;

    if (!reader.readUint32(type) || !reader.readUint32(mode) || !reader.readUint32(nlink) ||
        !reader.readUint32(uid) || !reader.readUint32(gid) || !reader.readUint64(size) ||
        !reader.readUint64(used) || !reader.readUint32(rdevMajor) || !reader.readUint32(rdevMinor) ||
        !reader.readUint64(fsid) || !reader.readUint64(fileid) || !reader.readUint32(atimeSec) ||
        !reader.readUint32(atimeNsec) || !reader.readUint32(mtimeSec) ||
        !reader.readUint32(mtimeNsec) || !reader.readUint32(ctimeSec) ||
        !reader.readUint32(ctimeNsec)) {
        return false;
    }

    out.type = nfsTypeToNodeType(type);
    out.size = size;
    out.mode = mode & 0777;
    out.uid = uid;
    out.gid = gid;
    out.mtime = static_cast<int64_t>(mtimeSec);
    return true;
}

bool NfsBackend::lookupPathComponents(const std::vector<std::string>& components,
                                      std::vector<uint8_t>& handle, VfsStat* statOut) {
    handle = rootHandle_;
    std::string currentPath = "/";

    for (const std::string& component : components) {
        XdrWriter args;
        encodeFhandle(args, handle);
        args.writeString(component);

        std::vector<uint8_t> reply;
        if (!nfsRpc_.call(kNfsProgram, kNfsVersion, kNfsProcLookup, args.data(), reply)) {
            return false;
        }

        XdrReader reader(reply.data(), reply.size());
        uint32_t status = 0;
        if (!reader.readUint32(status) || status != kNfsOk) {
            return false;
        }

        std::vector<uint8_t> objectHandle;
        if (!reader.readOpaque(objectHandle)) {
            return false;
        }

        if (statOut) {
            uint32_t attrFollows = 0;
            if (!reader.readUint32(attrFollows) || attrFollows == 0) {
                return false;
            }
            uint32_t type = 0;
            uint32_t mode = 0;
            uint32_t nlink = 0;
            uint32_t uid = 0;
            uint32_t gid = 0;
            uint64_t size = 0;
            uint64_t used = 0;
            uint32_t rdevMajor = 0;
            uint32_t rdevMinor = 0;
            uint64_t fsid = 0;
            uint64_t fileid = 0;
            uint32_t atimeSec = 0;
            uint32_t atimeNsec = 0;
            uint32_t mtimeSec = 0;
            uint32_t mtimeNsec = 0;
            uint32_t ctimeSec = 0;
            uint32_t ctimeNsec = 0;
            if (!reader.readUint32(type) || !reader.readUint32(mode) || !reader.readUint32(nlink) ||
                !reader.readUint32(uid) || !reader.readUint32(gid) || !reader.readUint64(size) ||
                !reader.readUint64(used) || !reader.readUint32(rdevMajor) ||
                !reader.readUint32(rdevMinor) || !reader.readUint64(fsid) ||
                !reader.readUint64(fileid) || !reader.readUint32(atimeSec) ||
                !reader.readUint32(atimeNsec) || !reader.readUint32(mtimeSec) ||
                !reader.readUint32(mtimeNsec) || !reader.readUint32(ctimeSec) ||
                !reader.readUint32(ctimeNsec)) {
                return false;
            }
            statOut->type = nfsTypeToNodeType(type);
            statOut->size = size;
            statOut->mode = mode & 0777;
            statOut->uid = uid;
            statOut->gid = gid;
            statOut->mtime = static_cast<int64_t>(mtimeSec);
        }

        handle = objectHandle;
        currentPath += currentPath.back() == '/' ? component : "/" + component;
        handleCache_[currentPath] = handle;
    }

    return true;
}

bool NfsBackend::lookupHandle(const std::string& path, std::vector<uint8_t>& handle) {
    const auto cached = handleCache_.find(path);
    if (cached != handleCache_.end()) {
        handle = cached->second;
        return true;
    }

    return lookupPathComponents(splitPath(path), handle, nullptr);
}

VfsError NfsBackend::stat(const std::string& path, VfsStat& out) {
    if (path == "/") {
        return nfsGetAttr(rootHandle_, out) ? VfsError::Ok : VfsError::Io;
    }

    std::vector<uint8_t> handle;
    if (!lookupHandle(path, handle)) {
        return VfsError::NotFound;
    }
    return nfsGetAttr(handle, out) ? VfsError::Ok : VfsError::Io;
}

VfsError NfsBackend::readdir(const std::string& path, std::vector<VfsDirEntry>& entries) {
    std::vector<uint8_t> handle;
    if (!lookupHandle(path, handle)) {
        return VfsError::NotFound;
    }

    entries.clear();
    uint64_t cookie = 0;
    bool eof = false;

    while (!eof) {
        XdrWriter args;
        encodeFhandle(args, handle);
        args.writeUint64(cookie);
        args.writeUint32(4096);
        args.writeUint32(4096);

        std::vector<uint8_t> reply;
        if (!nfsRpc_.call(kNfsProgram, kNfsVersion, kNfsProcReadDir, args.data(), reply)) {
            return VfsError::Io;
        }

        XdrReader reader(reply.data(), reply.size());
        uint32_t status = 0;
        if (!reader.readUint32(status)) {
            return VfsError::Io;
        }
        if (status != kNfsOk) {
            return nfsStatusToError(status);
        }

        uint32_t attrFollows = 0;
        if (!reader.readUint32(attrFollows)) {
            return VfsError::Io;
        }

        uint32_t hasEntries = 0;
        if (!reader.readUint32(hasEntries)) {
            return VfsError::Io;
        }

        if (hasEntries == 0) {
            break;
        }

        while (true) {
            uint32_t present = 0;
            if (!reader.readUint32(present) || present == 0) {
                break;
            }

            uint64_t fileid = 0;
            std::string name;
            uint64_t nextCookie = 0;
            if (!reader.readUint64(fileid) || !reader.readString(name) ||
                !reader.readUint64(nextCookie)) {
                return VfsError::Io;
            }

            uint32_t nameAttrFollows = 0;
            if (!reader.readUint32(nameAttrFollows) || nameAttrFollows == 0) {
                return VfsError::Io;
            }

            uint32_t type = 0;
            uint32_t mode = 0;
            uint32_t nlink = 0;
            uint32_t uid = 0;
            uint32_t gid = 0;
            uint64_t size = 0;
            uint64_t used = 0;
            uint32_t rdevMajor = 0;
            uint32_t rdevMinor = 0;
            uint64_t fsid = 0;
            uint64_t entryFileid = 0;
            uint32_t atimeSec = 0;
            uint32_t atimeNsec = 0;
            uint32_t mtimeSec = 0;
            uint32_t mtimeNsec = 0;
            uint32_t ctimeSec = 0;
            uint32_t ctimeNsec = 0;
            if (!reader.readUint32(type) || !reader.readUint32(mode) || !reader.readUint32(nlink) ||
                !reader.readUint32(uid) || !reader.readUint32(gid) || !reader.readUint64(size) ||
                !reader.readUint64(used) || !reader.readUint32(rdevMajor) ||
                !reader.readUint32(rdevMinor) || !reader.readUint64(fsid) ||
                !reader.readUint64(entryFileid) || !reader.readUint32(atimeSec) ||
                !reader.readUint32(atimeNsec) || !reader.readUint32(mtimeSec) ||
                !reader.readUint32(mtimeNsec) || !reader.readUint32(ctimeSec) ||
                !reader.readUint32(ctimeNsec)) {
                return VfsError::Io;
            }

            if (name != "." && name != "..") {
                VfsDirEntry entry;
                entry.name = name;
                entry.type = nfsTypeToNodeType(type);
                entries.push_back(std::move(entry));
            }

            cookie = nextCookie;
        }

        uint32_t eofFlag = 0;
        if (!reader.readUint32(eofFlag)) {
            return VfsError::Io;
        }
        eof = eofFlag != 0;
    }

    return VfsError::Ok;
}

VfsError NfsBackend::read(const std::string& path, uint64_t offset, size_t size,
                          std::vector<uint8_t>& out) {
    std::vector<uint8_t> handle;
    if (!lookupHandle(path, handle)) {
        return VfsError::NotFound;
    }

    XdrWriter args;
    encodeFhandle(args, handle);
    args.writeUint64(offset);
    args.writeUint32(static_cast<uint32_t>(size));

    std::vector<uint8_t> reply;
    if (!nfsRpc_.call(kNfsProgram, kNfsVersion, kNfsProcRead, args.data(), reply)) {
        return VfsError::Io;
    }

    XdrReader reader(reply.data(), reply.size());
    uint32_t status = 0;
    if (!reader.readUint32(status)) {
        return VfsError::Io;
    }
    if (status != kNfsOk) {
        return nfsStatusToError(status);
    }

    uint32_t attrFollows = 0;
    if (!reader.readUint32(attrFollows)) {
        return VfsError::Io;
    }
    if (attrFollows != 0) {
        uint32_t skip[17];
        for (int i = 0; i < 17; ++i) {
            if (!reader.readUint32(skip[i])) {
                return VfsError::Io;
            }
        }
        uint64_t skip64[4];
        for (int i = 0; i < 4; ++i) {
            if (!reader.readUint64(skip64[i])) {
                return VfsError::Io;
            }
        }
    }

    uint32_t count = 0;
    if (!reader.readUint32(count)) {
        return VfsError::Io;
    }

    out.resize(count);
    if (count > 0 && !reader.readRaw(out.data(), count)) {
        return VfsError::Io;
    }

    const size_t remainder = count % 4;
    if (remainder != 0) {
        reader.skip(4 - remainder);
    }

    return VfsError::Ok;
}

VfsError NfsBackend::write(const std::string& /*path*/, uint64_t /*offset*/,
                           const uint8_t* /*data*/, size_t /*size*/) {
    return VfsError::Unsupported;
}

VfsError NfsBackend::mkdir(const std::string& /*path*/, uint32_t /*mode*/) {
    return VfsError::Unsupported;
}

VfsError NfsBackend::unlink(const std::string& /*path*/) {
    return VfsError::Unsupported;
}

}  // namespace filefuse
