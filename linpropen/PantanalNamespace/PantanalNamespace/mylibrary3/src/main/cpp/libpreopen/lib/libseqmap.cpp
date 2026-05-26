#include <fcntl.h>
#include <libseqmap.h>

#include <android/log.h>
#include <libpreopen.h>

#include "internal.h"

#define NS_LOG_TAG "PreopenDemo"
#define NS_LOGI(...) __android_log_print(ANDROID_LOG_INFO, NS_LOG_TAG, __VA_ARGS__)
#define NS_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, NS_LOG_TAG, __VA_ARGS__)

using namespace std;
Seqmap::~Seqmap() {
    // free memory
    std::map<uint64_t, po_map*>::iterator iter;
    for(iter=pathmap_.begin(); iter != pathmap_.end();)
    {
        auto map = iter->second;
        po_map_release(map);
        pathmap_.erase(iter++);
    }
}

// 插入数据
int Seqmap::InsertMap(uint64_t id, const char *path, const char *pathname) {
    NS_LOGE("InsertMap id=%llu realRoot=%s virtual=%s",
            static_cast<unsigned long long>(id),
            path != nullptr ? path : "(null)",
            pathname != nullptr ? pathname : "(null)");

    mutex_.lock();
    struct po_map *p_map = nullptr;
    std::map<uint64_t, po_map*>::iterator iter;
    iter = pathmap_.find(id);
    int fd = openat(AT_FDCWD, path, O_RDONLY);
    // id is not exit,create new po_map
    if(iter == pathmap_.end()) {

        p_map = CreatePreOpenMap(4);
        //path is real path,pathname is virtual path
        if (po_add(p_map, path, pathname, fd) == nullptr) {
            NS_LOGE("InsertMap po_add failed id=%llu fd=%d",
                    static_cast<unsigned long long>(id), fd);
            mutex_.unlock();
            return (-1);
        }
        pathmap_.insert(std::make_pair(id, p_map));

    }
    else {
        // id is exit,use id's pomap
        p_map = iter->second;
        fd = openat(AT_FDCWD, path, O_RDONLY);
        struct po_relpath  rel = po_find(p_map, path, nullptr);
        if( (strcmp(rel.relative_path, ".") != 0) && (rel.dirfd == -1))
        {
            if (po_add(p_map, path, pathname, fd) == nullptr) {
                NS_LOGE("InsertMap po_add failed (existing id) id=%llu fd=%d",
                        static_cast<unsigned long long>(id), fd);
                mutex_.unlock();
                return (-1);
            }
            pathmap_.insert(std::make_pair(id, p_map));
        }
    }
    mutex_.unlock();
    NS_LOGI("InsertMap ret=0 id=%llu", static_cast<unsigned long long>(id));
    return 0;
}


// 根据id删除所对应的map<key,value>
int Seqmap::DeleteMap(uint64_t id) {
    mutex_.lock();
    std::map<uint64_t,po_map*>::iterator iter;
    iter = pathmap_.find(id);

    if(iter != pathmap_.end()) {
        // 释放内存
        auto map = iter->second;
        po_map_release(map);
        pathmap_.erase(iter);
    }
    mutex_.unlock();

    return 0;
}

struct po_relpath  Seqmap::GetRel(const char *path, uint64_t id) {
    struct po_map *map = nullptr;
    struct po_relpath rel;
    std::map<uint64_t, po_map *>::iterator iter;
    iter = pathmap_.find(id);

    if (iter != pathmap_.end()) {
        map = iter->second;
    }

    if (map == nullptr) {
        rel.dirfd = AT_FDCWD;
        rel.relative_path = path;
    } else {
        rel = po_find(map, path, nullptr);
    }
    return rel;
}

struct po_map* Seqmap::CreatePreOpenMap(int capacity) {
    struct po_map *map;
    map = static_cast<struct po_map *>(malloc(sizeof(struct po_map)));

    if (map == nullptr) {
        return (nullptr);
    }
    map->entries = static_cast<struct po_map_entry*>(calloc(sizeof(struct po_map_entry), capacity));

    if (map->entries == nullptr) {
        free(map);
        return (nullptr);
    }
    map->refcount = 1;
    map->capacity = capacity;
    map->length = 0;
    po_map_assertvalid(map);

    return (map);
}

struct po_map* Seqmap::po_map_enlarge(struct po_map *map) {
    struct po_map_entry *enlarged;
    enlarged = static_cast<struct po_map_entry*>(calloc(sizeof(struct po_map_entry), 2 * map->capacity));
    if (enlarged == nullptr) {
        return (nullptr);
    }
    memcpy(enlarged, map->entries, map->length * sizeof(*enlarged));
    free(map->entries);
    map->entries = enlarged;
    map->capacity = 2 * map->capacity;

    return map;
}

void Seqmap::po_map_release(struct po_map *map) {

    if (map == nullptr) {
        return;
    }
    po_map_assertvalid(map);
    map->refcount -= 1;

    if (map->refcount == 0) {
        free(map->entries);
        free(map);
    }
}


struct po_relpath Seqmap::po_find(struct po_map* map, const char *path, cap_rights_t * rights) {
        const char *relpath ;
        struct po_relpath match = { .dirfd = -1, .relative_path = nullptr };
        size_t bestlen = 0;
        int best = -1;

        po_map_assertvalid(map);

        if (path == nullptr) {
            return ( match);
        }

        for(size_t i = 0; i < map->length; i++) {
            const struct po_map_entry *entry = map->entries + i;
            const char *name = entry->pathmap;
            size_t len = strnlen(name, MAXPATHLEN);

            if ((len <= bestlen) || !po_isprefix(name, len, path)) {
                continue;
            }

            // 后续定义 cap_rights_contains
            //      if (rights && !cap_rights_contains(entry->rights, rights)) {
            //		continue;
            //	}

            best = entry->fd;
            bestlen = len;
        }

        relpath = path + bestlen;

        while (*relpath == '/') {
            relpath++;
        }

        if (*relpath == '\0') {
            relpath = ".";
        }

        match.relative_path = relpath;
        match.dirfd = best;

        return match;
}

struct po_map*
Seqmap::po_add(struct po_map *map, const char *path, const char *map_path, int fd) {
    struct po_map_entry *entry;
    po_map_assertvalid(map);
    cout<<"path = "<<path<<endl;
    cout<<"fd = "<<fd<<endl;

    if (path == nullptr || fd < 0) {
        std::cout<<"      66666666666666"<<endl;
        return (nullptr);
    }
    std::cout<<"      5555555555555555"<<endl;
    if (map->length == map->capacity) {
        map = po_map_enlarge(map);
        if (map == nullptr) {
            return (nullptr);
        }
    }

    entry = map->entries + map->length;
    map->length++;

    entry->name = strdup(path);
    entry->fd = fd;
    entry->pathmap =strdup(map_path);
    // 后续定义cap_rights_get
//	if (cap_rights_get(fd, &entry->rights) != 0) {
//		return (nullptr);
//	}


    po_map_assertvalid(map);
    return (map);
}


static struct po_relpath
find_relative(const char *path, uint64_t id) {
    struct po_relpath rel;
    struct po_map *map;



    rel = Seqmap::GetInstance().GetRel(path, id);

    return (rel);
}
