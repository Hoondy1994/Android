#pragma once

#include <node_api.h>

int ns_access(napi_env* env, const char* path, int mode);
int ns_eacess(napi_env* env, const char* path, int mode);
int ns_faccessat(napi_env* env,
                 int dirfd,
                 const char* pathname,
                 int mode,
                 int flags);
int ns_unlink(napi_env* env, const char* path);