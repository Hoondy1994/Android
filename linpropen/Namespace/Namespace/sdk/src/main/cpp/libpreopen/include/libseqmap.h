#ifndef LIBPO_C_PLUS
#define LIBPO_C_PLUS

#include <assert.h>
#include <stdbool.h>

#include <iostream>
#include <mutex>

#include <libpreopen.h>
#include <libc_wrapper.h>

#include "Singleton.h"
#include <map>


class Seqmap : public ::pantanal::ns::Singleton<Seqmap> {
public:
    Seqmap() = default;
    ~Seqmap();

    // 插入map
    int InsertMap(uint64_t id, const char *path, const char *pathname);

    //删除Map
    int DeleteMap(uint64_t id);

    // 获取map指针
    struct po_relpath GetRel(const char *path, uint64_t id);
private:


    // 创建map
    struct po_map* CreatePreOpenMap(int capacity);

    // 判断po_map是否合法
    struct po_map* po_map_enlarge(struct po_map *map);

    // 判断po_map是否合法
    void po_map_release(struct po_map *map);

    struct po_relpath po_find(struct po_map* map, const char *path, cap_rights_t * rights);

    struct po_map* po_add(struct po_map *map, const char *path, const char *map_path, int fd);


     std::map<uint64_t, po_map*> pathmap_;
     std::mutex mutex_;
};


#endif /* LIBPO_C_PLUS */