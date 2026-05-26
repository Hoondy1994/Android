//
// Created by 80244960 on 2022/12/6.
//

#include <libpreopen.h>
#include "namespace/NamespaceLibc.h"

int ns_access(napi_env* env, const char* path, int mode) {
    return 0;
}

int ns_faccessat(napi_env* env,
                 int dirfd,
                 const char* pathname,
                 int mode,
                 int flags) {
    return 0;
}

int ns_unlink(napi_env* env, const char* path) {
    return 0;
}

int ns_rename(napi_env* env, const char* from, const char* to) {
    return 0;
}

int ns_stat(napi_env* env, const char* path, struct stat* st) {
    return 0;
}

int ns_lstat(napi_env* env, const char* path, struct stat* st) {
    return 0;
}

int ns_fstatat(napi_env* env, const char* path, struct stat* st) {
    return 0;
}

int ns_creat(napi_env* env, const char *pathname, mode_t mode) {
    return 0;
}

int ns_open(napi_env* env, const char* path, int flags, ...) {
    return 0;
}

int ns_openat(napi_env* env, int dirfd, const char *pathname, int flags, ...) {
    return 0;
}