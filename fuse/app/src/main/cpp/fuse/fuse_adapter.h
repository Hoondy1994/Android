#pragma once

#include "../vfs/vfs.h"

#include <sys/stat.h>

#include <string>
#include <vector>

namespace filefuse {

struct FuseDirEntry {
    std::string name;
    int type = 0;
};

class FuseAdapter {
public:
    explicit FuseAdapter(VirtualFileSystem& vfs);

    int getattr(const std::string& path, struct stat* stbuf) const;
    int readdir(const std::string& path, std::vector<FuseDirEntry>& entries) const;
    int read(const std::string& path, char* buffer, size_t size, off_t offset) const;
    int write(const std::string& path, const char* buffer, size_t size, off_t offset) const;
    int mkdir(const std::string& path, mode_t mode) const;
    int unlink(const std::string& path) const;

private:
    VirtualFileSystem& vfs_;

    static void fillStat(const VfsStat& source, struct stat* target);
};

}  // namespace filefuse
