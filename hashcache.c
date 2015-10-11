/* Copyright 2015. Martin Uecker.
 * All rights reserved. Use of this source code is governed by
 * a BSD-style license which can be found in the LICENSE file.
 *
 * Author: 2015 Martin Uecker <muecker@gwdg.de>
 *
 * Comment: This is similar to the git index. There are also
 * seem to be similar tools (bitrot.sh, etc.).
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>

#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/xattr.h>
#include <sys/stat.h>
#include <linux/fs.h>

#include <attr/xattr.h>

#include <errno.h>

#include "sha2.h"



static void help(void)
{
	printf(	"Computes the sha2 hash for files and caches it in an extended attribute.\n"
		"Advanced options (defaults: -scu):\n"
		"\t-i\tcheck integrity\n"
		"\t-c\tcompute hash if not cached\n"
		"\t-r\talway recompute hash\n"
		"\t-s\tshow hash if available (states: n|ew, c|ached, s|tale,\n"
		"\t\t  r|efreshed, u|pdated, v|erified, !=unexpected mismatch)\n"
		"\t-d\tdelete cached hash\n"
		"\t-u\tupdate cached hash\n"
		"\t-h\tprint this help\n");
}

static void usage(FILE* fp, const char* name)
{
	fprintf(fp, "%s: [...] [-h] files\n", name);
}


enum err_ret { ERR_SUCCESS, ERR_MISMATCH, ERR_USAGE, ERR_SYSTEM, ERR_NOFILE };

int main(int argc, char* argv[])
{
	int c;
	bool ask_show = false;
	bool ask_comp = false;
	bool ask_recomp = false;
	bool ask_check = false;
	bool ask_del = false;
	bool ask_update = false;
	bool defaults = true;
	int err = ERR_USAGE;

	bool mismatch_any = false;
	bool errsys_any = false;

	while (-1 != (c = getopt(argc, argv, "cdhirsu"))) {
	
		switch (c) {

		case 'c':
			ask_comp = true;
			break;
		case 'd':
			ask_del = true;
			break;
		case 'h':
			usage(stdout, argv[0]);
			help();
			err = ERR_SUCCESS;
			goto out2;
		case 'i':
			ask_check = true;
			break;
		case 's': 
			ask_show = true;
			break;
		case 'r':
			ask_recomp = true;
			break;
		case 'u':
			ask_update = true;
			break;
		default:
			goto out2;
		}

		defaults = false;
	}

	if ((ask_update && ask_del) || (ask_comp && ask_recomp))
		goto out2;

	if (defaults)
		ask_show = ask_comp = ask_update = true;

	for (int n = optind; n < argc; n++) {

		err = ERR_SYSTEM;
		int fd;

		if (-1 == (fd = open(argv[n], O_RDONLY)))
			goto err1;

		struct stat st;
	
		if (-1 == fstat(fd, &st))
			goto err2;

		if (!S_ISREG(st.st_mode)) {

			fprintf(stderr, "Not a regular file: %s\n", argv[n]);
			err = ERR_NOFILE;
			goto err2;
		}

		uint64_t ino = st.st_ino;
		uint64_t size = st.st_size;
		uint64_t mt = st.st_mtime;

		uint32_t generation = 0;

		if (0 != ioctl(fd, FS_IOC_GETVERSION, &generation))
			if (errno != ENOTTY)	// unsupported
				goto err2;

		// not sure we need generation, also see racy-git

		unsigned char str[64] = { 0 };
		memcpy(str +  0, &ino, 8);
		memcpy(str +  8, &generation, 4);
		memcpy(str + 16, &mt, 8);
		memcpy(str + 24, &size, 8);

		unsigned char buf[64] = { 0 };
		bool have_attr = true;

		if (64 != fgetxattr(fd, "user.sha256", buf, 64)) {
	
		 	// maybe handle wrong size by ignoring it

			if (errno != ENOATTR)
				goto err2;

			have_attr = false;
		}

		bool is_upd = have_attr && (0 == memcmp(str, buf, 32));

		// decide what to do

		bool do_check = have_attr && ask_check;
		bool do_comp = ask_recomp || (!(have_attr && is_upd) && ask_comp);
		bool do_update = ask_update && do_comp;
		bool do_delete = have_attr && (ask_del || (!do_comp && !is_upd && ask_update));
		bool do_show = ask_show;

		memcpy(str + 32, buf + 32, 32);

		if (do_comp) {
	
			void* addr;
			if (MAP_FAILED == (addr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0)))
				goto err2;
	
			sha256(addr, size, str + 32);

			if (-1 == munmap(addr, size))
				goto err2;
		}

		bool mismatch = do_comp && have_attr && (0 != memcmp(str + 32, buf + 32, 32));

		// reconsider update
		do_update &= !(is_upd && !mismatch);

		if (do_check)
			mismatch_any |= mismatch;

		if (mismatch)
			printf("File changed: %s\n", argv[n]);

		if (do_show) {

			for (int i = 0; i < 32; i++)
				if (!(have_attr || do_comp))
					printf("##");
				else
					printf("%02x", (int)str[32 + i]);

			// have_attr, is_upd, do_comp, mismatch
			char states[2][2][2][2] = {
			//	    noc          / comp
			//            ok / mismatch  ok / mismatch  
				{ { { '-', 'e', }, { 'n', 'e', }, }, 	// !have_attr	stale
				  { { 'e', 'e', }, { 'e', 'e', }, }, },	// 		update
				{ { { 's', 'e', }, { 'r', 'u', }, }, 	// have_attr	stale
			          { { 'c', 'e', }, { 'v', '!', }, }, },	// 		update
			};

			char st = states[have_attr][is_upd][do_comp][mismatch];
			assert('e' != st);

			printf(" %c %s\n", st, argv[n]);
		}

		if (do_update)
			if (-1 == fsetxattr(fd, "user.sha256", str, sizeof(str), 0))
				goto err2;

		if (do_delete)
			if (-1 == fremovexattr(fd, "user.sha256"))
				goto err2;

		err = ERR_SUCCESS;
	err2:
		if (-1 == close(fd))
			err = ERR_SYSTEM;
	err1:
		if (ERR_SYSTEM == err) {

			perror("error: ");
			errsys_any |= true;
		}
	}
out2:
	if (ERR_USAGE == err)
		usage(stderr, argv[0]);

	if (errsys_any)
		err = ERR_SYSTEM;

	if (mismatch_any)
		err = ERR_MISMATCH;

	exit(err);
}

