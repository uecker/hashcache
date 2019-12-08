/* Copyright 2015. Martin Uecker.
 * All rights reserved. Use of this source code is governed by
 * a BSD-style license which can be found in the LICENSE file.
 *
 * Author: 2015 Martin Uecker <muecker@gwdg.de>
 *
 * Comment: This is similar to the git index. There are also
 * seem to be similar tools (bitrot.sh, etc.).
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/xattr.h>
#include <sys/stat.h>
#include <linux/fs.h>

#include <attr/xattr.h>

#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "sha2.h"

#include "hc.h"

static const char* attribute = "user.hc1_sha256";


int hashcache(unsigned char digest[32], int fd, unsigned int flags)
{
	bool ask_recomp = flags & HC_RECOMPUTE;
	bool ask_comp = !(flags & HC_NOCOMPUTE);
	bool ask_update = !(flags & HC_NOUPDATE);
	bool ask_del = flags & HC_DELETE;

	struct stat st;

	if (-1 == fstat(fd, &st))
		return -ERR_SYSTEM;

	if (!S_ISREG(st.st_mode))
		return -ERR_NOFILE;

	uint64_t ino = st.st_ino;
	uint64_t size = st.st_size;
	uint64_t mt = st.st_mtime;

	uint32_t generation = 0;

	if (0 != ioctl(fd, FS_IOC_GETVERSION, &generation))
		if (errno != ENOTTY)	// unsupported
			return -ERR_SYSTEM;
#if 1
	// enforce that changes made after computing the hash
	// go along with a newer modification time

	if (mt == time(NULL))
		sleep(1);
#endif
	// not sure we need generation, also see racy-git

	unsigned char str[64] = { 0 };
	memcpy(str +  0, "HC01", 4);
	memcpy(str +  4, &generation, 4);
	memcpy(str +  8, &ino, 8);
	memcpy(str + 16, &mt, 8);
	memcpy(str + 24, &size, 8);

	unsigned char buf[64] = { 0 };
	bool have_attr = true;

	if (64 != fgetxattr(fd, attribute, buf, 64)) {

	 	// maybe handle wrong size by ignoring it

		if (errno != ENOATTR)
			return -ERR_SYSTEM;

		have_attr = false;
	}

	bool is_upd = have_attr && (0 == memcmp(str, buf, 32));

	// decide what to do

	bool do_comp = ask_recomp || (!(have_attr && is_upd) && ask_comp);
	bool do_update = ask_update && do_comp;
	bool do_delete = have_attr && (ask_del || (!do_comp && !is_upd && ask_update));

	memcpy(str + 32, buf + 32, 32);

	if (0 == size)
		sha256(NULL, 0, str + 32);

	if (do_comp && (size > 0)) {

		void* addr;
		if (MAP_FAILED == (addr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0)))
			return -ERR_SYSTEM;

		sha256(addr, size, str + 32);

		if (-1 == munmap(addr, size))
			return -ERR_SYSTEM;
	}

	bool mismatch = do_comp && have_attr && (0 != memcmp(str + 32, buf + 32, 32));

	// reconsider update
	do_update &= !(is_upd && !mismatch);

	if (do_update)
		if (-1 == fsetxattr(fd, attribute, str, sizeof(str), 0))
			return -ERR_SYSTEM;

	if (do_delete)
		if (-1 == fremovexattr(fd, attribute))
			return -ERR_SYSTEM;

	memcpy(digest, str + 32, 32);

	unsigned int rflags = 0;

	if (mismatch)
		rflags |= HC_RET_MISMATCH;

	if (!have_attr)
		rflags |= HC_RET_NOATTR;

	if (!do_comp)
		rflags |= HC_RET_NOCOMP;

	if (!is_upd)
		rflags |= HC_RET_STALE;

	return rflags;
}

