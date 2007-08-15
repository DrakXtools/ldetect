
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <zlib.h>
#include "common.h"

static char *table_name_to_file(const char *name) {

	char *share_path = getenv("SHARE_PATH");
	char *fname;
	if (!share_path || !*share_path) share_path = "/usr/share";

	asprintf(&fname, "%s/ldetect-lst/%s", share_path, name);

	return fname;
}

fh fh_open(const char *name) {
	char *fname = table_name_to_file(name);

	if (access(fname, R_OK) != 0) {
	    char *fname_gz;
	    asprintf(&fname_gz, "%s.gz", fname);
	    free(fname);
	    fname = fname_gz;
	}

	fh f = gzopen(fname, "r");
	if (!f) {
	    perror("pciusb");
	    exit(1);
	}

	free(fname);
	return f;
}
