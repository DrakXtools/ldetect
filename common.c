
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "common.h"

char *table_name_to_file(const char *name) {

	char *share_path = getenv("SHARE_PATH");
	char *fname;
	if (!share_path || !*share_path) share_path = "/usr/share";

	asprintf(&fname, "%s/ldetect-lst/%s", share_path, name);

	return fname;
}

fh fh_open(const char *name) {
	fh ret;
	char *fname = table_name_to_file(name);

	if (access(fname, R_OK) != 0) {
	    char *fname_gz;
	    asprintf(&fname_gz, "%s.gz", fname);
	    free(fname);
	    fname = fname_gz;
	}

	ret.zlib_fh = gzopen(fname, "r");
	if (!ret.zlib_fh) {
	    perror("pciusb");
	    exit(1);
	}

	free(fname);
	return ret;
}

char* fh_gets(char *line, int size, fh *f) {
        char *ret;
        ret = gzgets(f->zlib_fh, line, size);
        return ret;
}

int fh_close(fh *f) {
        int ret;
        ret = gzclose(f->zlib_fh);
        return ret;
}
