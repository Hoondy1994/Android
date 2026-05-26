#pragma once

#include <sys/socket.h>

#include <node_api.h>

int ns_connect(napi_env* env,
               int s,
               const struct sockaddr* name,
               socklen_t namelen);