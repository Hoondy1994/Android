//
// Created by 80244960 on 2022/12/6.
//

#ifndef NAMESPACE_NAMESPACELIBC_H
#define NAMESPACE_NAMESPACELIBC_H

#include <sys/socket.h>

#include <napi/node_api.h>

#ifdef __cplusplus
extern "C" {
#endif

// unistd.h
/**
 * checks whether the calling JS Runtime can access file path \n
 * see https://man7.org/linux/man-pages/man2/access.2.html
 * @param env JS Runtime napi environment
 * @param path file path
 * @param mode The mode specifies the accessibility check(s) to be performed,
       and is either the value F_OK, or a mask consisting of the bitwise
       OR of one or more of R_OK, W_OK, and X_OK.  F_OK tests for the
       existence of the file.  R_OK, W_OK, and X_OK test whether the
       file exists and grants read, write, and execute permissions,
       respectively.
 * @return 0 if succeed
 */
int ns_access(napi_env *env, const char *path, int mode);

/**
 * determine accessibility of a file relative to directory file descriptor \n
 * see https://man7.org/linux/man-pages/man3/faccessat.3p.html
 * @param env JS Runtime napi environment
 * @param dirfd directory fd
 * @param pathname relative path
 * @param mode The value of amode is either the bitwise-inclusive OR of the
       access permissions to be checked (R_OK, W_OK, X_OK) or the
       existence test (F_OK).
 * @param flags Values for flag are constructed by a bitwise-inclusive OR of
       flags from the following list, defined in <fcntl.h>:
       AT_EACCESS  The checks for accessibility (including directory
                   permissions checked during pathname resolution) shall
                   be performed using the effective user ID and group ID
                   instead of the real user ID and group ID as required
                   in a call to access().
 * @return 0 if succeed
 */
int ns_faccessat(napi_env *env,
                 int dirfd,
                 const char *pathname,
                 int mode,
                 int flags);

/**
 * delete a name and possibly the file it refers to \n
 * see https://man7.org/linux/man-pages/man2/unlink.2.html
 * @param env JS Runtime napi environment
 * @param path file path name
 * @return 0 if succeed
 */
int ns_unlink(napi_env *env, const char *path);

// stdio.h
/**
 * change the name or location of a file \n
 * see https://man7.org/linux/man-pages/man2/rename.2.html
 * @param env JS Runtime napi environment
 * @param from old path of a file
 * @param to new path of a file
 * @return 0 if succeed
 */
int ns_rename(napi_env *env, const char *from, const char *to);

// stat.h
/**
 * read information about a file \n
 * see https://man7.org/linux/man-pages/man2/lstat.2.html
 * @param env JS Runtime napi environment
 * @param path file path
 * @param st file information read from stat
 * @return 0 if succeed
 */
int ns_stat(napi_env *env, const char *path, struct stat *st);
/**
 * read information about a file \n
 * see https://man7.org/linux/man-pages/man2/lstat.2.html
 * @param env JS Runtime napi environment
 * @param path file path
 * @param st file information read from stat
 * @return 0 if succeed
 */
int ns_lstat(napi_env *env, const char *path, struct stat *st);
/**
 * read information about a file \n
 * see https://man7.org/linux/man-pages/man2/lstat.2.html
 * @param env JS Runtime napi environment
 * @param path file path
 * @param st file information read from stat
 * @return 0 if succeed
 */
int ns_fstatat(napi_env *env, const char *path, struct stat *st);

// fcnt.h
/**
 * create a new file or rewrite an existing one \n
 * see https://man7.org/linux/man-pages/man3/creat.3p.html
 * @param env JS Runtime napi environment
 * @param pathname file path
 * @param mode file mode
 * @return file fd if succeed
 */
int ns_creat(napi_env *env, const char *pathname, mode_t mode);
/**
 * system call opens the file specified by path \n
 * see https://man7.org/linux/man-pages/man2/open.2.html
 * @param env JS Runtime napi environment
 * @param path file path
 * @param flags access modes: O_RDONLY, O_WRONLY, or O_RDWR.
 * @param ...
 * @return file fd if succeed
 */
int ns_open(napi_env *env, const char *path, int flags, ...);
/**
 * system call opens the file specified by path \n
 * see https://man7.org/linux/man-pages/man2/open.2.html
 * @param env JS Runtime napi environment
 * @param dirfd directory fd
 * @param pathname file path
 * @param flags access modes: O_RDONLY, O_WRONLY, or O_RDWR.
 * @param ...
 * @return file fd if succeed
 */
int ns_openat(napi_env *env, int dirfd, const char *pathname, int flags, ...);

#ifdef __cplusplus
}
#endif

#endif //NAMESPACE_NAMESPACELIBC_H
