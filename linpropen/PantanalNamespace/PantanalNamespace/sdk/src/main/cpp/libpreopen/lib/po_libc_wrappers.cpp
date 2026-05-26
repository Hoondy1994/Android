/*-
 * Copyright (c) 2016 Stanley Uche Godfrey
 * Copyright (c) 2018 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed at Memorial University under the
 * NSERC Discovery program (RGPIN-2015-06048).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * @file   po_libc_wrappers.c
 * @brief  Wrappers of libc functions that access global variables.
 */
#include <fcntl.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <libseqmap.h>

/**
 * Get the map that was handed into the process via `SHARED_MEMORYFD`
 * (if it exists).
 */

static struct po_relpath find_relative(const char *path, uint64_t id);


/*
 * Wrappers around system calls:
 */

/**
 * Capability-safe wrapper around the `_open(2)` system call.
 *
 * `_open(2)` accepts a path argument that can reference the global filesystem
 * namespace. This is not a capability-safe operation, so this wrapper function
 * attempts to look up the path (or a prefix of it) within the current global
 * po_map and converts the call into the capability-safe `openat(2)` if
 * possible. If the current po_map does not contain the sought-after path,
 * this wrapper will call `openat(AT_FDCWD, original_path, ...)`, which is
 * the same as the unwrapped `open(2)` call (i.e., will fail with `ECAPMODE`).
 */
int
open_(uint64_t id, const char *path, int flags, ...) {
	struct po_relpath rel;
	va_list args;
	int mode;

	va_start(args, flags);
	mode = va_arg(args, int);

	rel = find_relative(path, id);
	// If the file is already opened, no need of relative opening!
	if( strcmp(rel.relative_path,".") == 0 ) {

        return dup(rel.dirfd);
    }
	else {
        return openat(rel.dirfd, rel.relative_path, flags, mode);
    }
}

/**
 * Capability-safe wrapper around the `access(2)` system call.
 *
 * `access(2)` accepts a path argument that can reference the global filesystem
 * namespace. This is not a capability-safe operation, so this wrapper function
 * attempts to look up the path (or a prefix of it) within the current global
 * po_map and converts the call into the capability-safe `faccessat(2)` if
 * possible. If the current po_map does not contain the sought-after path,
 * this wrapper will call `faccessat(AT_FDCWD, original_path, ...)`, which is
 * the same as the unwrapped `access(2)` call (i.e., will fail with `ECAPMODE`).
 */
int
access_(uint64_t id, const char *path, int mode) {
	struct po_relpath rel = find_relative(path, id);

	return faccessat(rel.dirfd, rel.relative_path, mode,0);
}


/**
 * Capability-safe wrapper around the `eaccess(2)` system call.
 *
 * `eaccess(2)` accepts a path argument that can reference the global filesystem
 * namespace. This is not a capability-safe operation, so this wrapper function
 * attempts to look up the path (or a prefix of it) within the current global
 * po_map and converts the call into the capability-safe `faccessat(2)` if
 * possible. If the current po_map does not contain the sought-after path, this
 * wrapper will call `faccessat(AT_FDCWD, original_path, ...)`, which is the
 * same as the unwrapped `eaccess(2)` call (i.e., will fail with `ECAPMODE`).
 */
int
eaccess(const char *path, int mode) {
	uint64_t id;
	struct po_relpath rel = find_relative(path, id);

	return faccessat(rel.dirfd, rel.relative_path, mode, 0);
}

/**
 * Capability-safe wrapper around the `lstat(2)` system call.
 *
 * `lstat(2)` accepts a path argument that can reference the global filesystem
 * namespace. This is not a capability-safe operation, so this wrapper function
 * attempts to look up the path (or a prefix of it) within the current global
 * po_map and converts the call into the capability-safe `fstatat(2)` if
 * possible. If the current po_map does not contain the sought-after path,
 * this wrapper will call `fstatat(AT_FDCWD, original_path, ...)`, which is
 * the same as the unwrapped `lstat(2)` call (i.e., will fail with `ECAPMODE`).
 */
int
lstat_(uint64_t id, const char *path, struct stat *st) {

	struct po_relpath rel = find_relative(path, id);

	return fstatat(rel.dirfd, rel.relative_path,st,AT_SYMLINK_NOFOLLOW);
}


/**
 * Capability-safe wrapper around the `rename(2)` system call.
 *
 * `rename(2)` accepts a path argument that can reference the global filesystem
 * namespace. This is not a capability-safe operation, so this wrapper function
 * attempts to look up the path (or a prefix of it) within the current global
 * po_map and converts the call into the capability-safe `renameat(2)` if
 * possible. If the current po_map does not contain the sought-after path,
 * this wrapper will call `renameat(AT_FDCWD, original_path, ...)`, which is
 * the same as the unwrapped `rename(2)` call (i.e., will fail with `ECAPMODE`).
 */
int
rename_(uint64_t id, const char *from, const char *to) {
	struct po_relpath rel_from = find_relative(from, id);
	struct po_relpath rel_to = find_relative(to, id);

	return renameat(rel_from.dirfd, rel_from.relative_path, rel_to.dirfd,
		rel_to.relative_path);
}

/**
 * Capability-safe wrapper around the `stat(2)` system call.
 *
 * `stat(2)` accepts a path argument that can reference the global filesystem
 * namespace. This is not a capability-safe operation, so this wrapper function
 * attempts to look up the path (or a prefix of it) within the current global
 * po_map and converts the call into the capability-safe `fstatat(2)` if
 * possible. If the current po_map does not contain the sought-after path,
 * this wrapper will call `fstatat(AT_FDCWD, original_path, ...)`, which is
 * the same as the unwrapped `stat(2)` call (i.e., will fail with `ECAPMODE`).
 */
int
stat_(uint64_t id, const char *path, struct stat *st) {
	struct po_relpath rel = find_relative(path, id);

	return fstatat(rel.dirfd, rel.relative_path,st, AT_SYMLINK_NOFOLLOW);
}

/**
 * Capability-safe wrapper around the `unlink(2)` system call.
 *
 * `unlink(2)` accepts a path argument that can reference the global filesystem
 * namespace. This is not a capability-safe operation, so this wrapper function
 * attempts to look up the path (or a prefix of it) within the current global
 * po_map and converts the call into the capability-safe `unlinkat(2)` if
 * possible. If the current po_map does not contain the sought-after path,
 * this wrapper will call `unlinkat(AT_FDCWD, original_path, 0) which is
 * the same as the unwrapped `unlink(2)` call (i.e., will fail with `ECAPMODE`).
 */
int
unlink_(uint64_t id, const char *path) {
	struct po_relpath rel = find_relative(path, id);

	return unlinkat(rel.dirfd, rel.relative_path, 0);
}

static struct po_relpath
find_relative(const char *path, uint64_t id) {
	struct po_relpath rel;

	rel = Seqmap::GetInstance().GetRel(path, id);


	return (rel);
}

