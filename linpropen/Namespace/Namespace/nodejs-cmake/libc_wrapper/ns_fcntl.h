#pragma once

#include <sys/stat.h>

#include <node_api.h>

int ns_open(napi_env* env, const char* path, int flags, ...);
int ns_creat(napi_env* env, const char *pathname, mode_t mode);
int ns_openat(napi_env* env, int dirfd, const char *pathname, int flags, ...);