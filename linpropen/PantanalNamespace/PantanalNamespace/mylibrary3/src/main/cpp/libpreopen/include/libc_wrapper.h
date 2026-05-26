#ifndef LIBWRAP_H
#define LIBWRAP_H

#include <libpreopen.h>
#define MAXPATHLEN 100

__BEGIN_DECLS

int rename_(uint64_t id ,const char *from, const char *to);

int open_(uint64_t id, const char *path, int flags, ...);

int access_(uint64_t id, const char *path, int mode);

int lstat_(uint64_t id, const char *path, struct stat *st);

int unlink_(uint64_t id, const char *path);

int stat_(uint64_t id, const char *path, struct stat *st);

__END_DECLS

#endif