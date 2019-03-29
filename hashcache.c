/* Copyright 2015, 2019 Martin Uecker.
 * All rights reserved. Use of this source code is governed by
 * a BSD-style license which can be found in the LICENSE file.
 *
 * Author: 2015, 2019 Martin Uecker <muecker@gwdg.de>
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

#include <fcntl.h>
#include <errno.h>

#include "hc.h"



static void help(void)
{
	printf(	"Computes the sha256 hash for files and caches it in an extended attribute.\n"
		"Advanced options (defaults: -scu):\n"
		"\t-i\tcheck integrity\n"
		"\t-c\tcompute hash if not cached\n"
		"\t-r\talway recompute hash\n"
		"\t-s\tshow hash if available (states: n|ew, c|ached, s|tale,\n"
		"\t\t  r|efreshed, u|pdated, v|erified, !=unexpected mismatch)\n"
		"\t-d\tdelete cached hash\n"
		"\t-u\tupdate cached hash\n"
		"\t-q\tquiet\n"
		"\t-h\tprint this help\n");
}

static void usage(FILE* fp, const char* name)
{
	fprintf(fp, "%s: [...] [-h] files\n", name);
}



int main(int argc, char* argv[])
{
	int c;
	bool ask_show = false;
	bool ask_comp = false;
	bool ask_recomp = false;
	bool ask_check = false;
	bool ask_del = false;
	bool ask_update = false;
	bool ask_quiet = false;
	bool defaults = true;
	int err = ERR_USAGE;

	bool mismatch_any = false;
	bool errsys_any = false;

	while (-1 != (c = getopt(argc, argv, "cdhirsuq"))) {
	
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
		case 'q':
			ask_quiet = true;
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

		unsigned int flags = 0;
	
		if (!ask_update)
			flags |= HC_NOUPDATE;

		if (!ask_comp)
			flags |= HC_NOCOMPUTE;

		if (ask_del)
			flags |= HC_DELETE;

		if (ask_recomp)
			flags |= HC_RECOMPUTE;

		unsigned char digest[32];
		int ret;

		if (0 > (ret = hashcache(digest, fd, flags))) {

			err = -ret;

			if (ERR_NOFILE == err) {

				fprintf(stderr, "Not a regular file: %s\n", argv[n]);
				continue;
			}
		}

		bool mismatch = ret & HC_RET_MISMATCH;

		if (mismatch && !ask_quiet && !ask_show)
			printf("File changed: %s\n", argv[n]);

		bool have_attr = !(ret & HC_RET_NOATTR);
		bool do_comp = !(ret & HC_RET_NOCOMP);
		bool is_upd = !(ret & HC_RET_STALE);

		bool do_check = have_attr && ask_check;
		bool do_show = ask_show;

		if (do_check)
			mismatch_any |= mismatch;

		if (do_show) {

			for (int i = 0; i < 32; i++)
				if (!(have_attr || do_comp))
					printf("##");
				else
					printf("%02x", (int)digest[i]);

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

		err = ERR_SUCCESS;

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

