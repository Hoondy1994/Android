/*-
 * Copyright (c) 2016 Stanley Uche Godfrey
 * Copyright (c) 2016, 2018 Jonathan Anderson
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
 * @file  libpreopen.c
 * Implementation of high-level libpreopen functions.
 *
 * The functions defined in this source file are the highest-level API calls
 * that client code will mostly use (plus po_map_create and po_map_release).
 * po_isprefix is also defined here because it doesn't fit anywhere else.
 */
#include <assert.h>
#include <fcntl.h>
#include "internal.h"
#include <iostream>
#include <libseqmap.h>.
using namespace std;


bool cap_rights_get(int fd, cap_rights_t *rights)
{
	//后续实现
	return true;
}

bool cap_rights_contains(uint64_t rights, cap_rights_t *right)
{
	//后续实现
	return true;
}

bool
po_isprefix(const char *dir, size_t dirlen, const char *path) {
	size_t i;
	assert(dir != nullptr);
	assert(path != nullptr);
	for (i = 0; i < dirlen; i++)
	{
		if (path[i] != dir[i])
			return false;
	}
	return path[i] == '/' || path[i] == '\0';
}

