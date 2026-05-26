#pragma once

#include <sys/stat.h>

#include <node_api.h>

int ns_lstat(napi_env* env, const char* path, int mode);
int ns_stat(napi_env* env, const char* path, struct stat* st);