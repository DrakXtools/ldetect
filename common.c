#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "common.h"

char *table_name_to_file(const char *name) {

	const char *share_path = getenv("SHARE_PATH");
	char *fname;
	if (!share_path || !*share_path) share_path = "/usr/share";

	asprintf(&fname, "%s/ldetect-lst/%s", share_path, name);

	return fname;
}

fh fh_open(const char *name) {
	fh ret;
	char *fname = table_name_to_file(name);

	if (access(fname, R_OK) == 0) {
		/* prefer gzip type when not compressed, more direct than zlib access */
		ret.gztype = GZIP;
		ret.u.gzip_fh.f = fopen(fname, "r");
		ret.u.gzip_fh.pid = 0;
	} else {
		char *fname_gz;
                asprintf(&fname_gz, "%s.gz", fname);
                if (access(GZIP_BIN, R_OK) == 0) {
                        int fdno[2];
                        ret.gztype = GZIP;
                        if (access(fname_gz, R_OK) != 0) {
                                fprintf(stderr, "Missing %s (should be %s)\n", name, fname);
                                exit(1);
                        }
                        if (pipe(fdno)) {
                                perror("pciusb");
                                exit(1);
                        }

                        if ((ret.u.gzip_fh.pid = fork()) != 0) {
                                ret.u.gzip_fh.f = fdopen(fdno[0], "r");
                                close(fdno[1]);
                        } else {
                                char* cmd[5];
                                int ip = 0;
                                char *ld_loader = getenv("LD_LOADER");

                                if (ld_loader && *ld_loader)
                                        cmd[ip++] = ld_loader;

                                cmd[ip++] = GZIP_BIN;
                                cmd[ip++] = "-cd";
                                cmd[ip++] = fname_gz;
                                cmd[ip++] = NULL;

                                dup2(fdno[1], STDOUT_FILENO);
                                close(fdno[0]);
                                close(fdno[1]);
                                execvp(cmd[0], cmd);
                                perror("pciusb"); 
                                exit(2);
                        }
		}
#ifdef HAVE_LIBZ
		else {
                        ret.gztype = ZLIB;
                        ret.u.zlib_fh = gzopen(fname_gz, "r");
                        if (!ret.u.zlib_fh) {
                                perror("pciusb");
                                exit(3);
                        }
                }
		free(fname_gz);
#endif
	}

	free(fname);
	return ret;
}

char* fh_gets(char *line, int size, fh *f) {
        char *ret = NULL;
        switch (f->gztype) {
        case ZLIB:
#ifdef HAVE_LIBZ
                ret = gzgets(f->u.zlib_fh, line, size);
                break;
#endif
        case GZIP:
                ret = fgets(line, size, f->u.gzip_fh.f);
                break;
        }
        return ret;
}

int fh_close(fh *f) {
        int ret = EOF;
        switch (f->gztype) {
        case ZLIB:
#ifdef HAVE_LIBZ
                ret = gzclose(f->u.zlib_fh);
                break;
#endif
        case GZIP:
                ret = fclose(f->u.gzip_fh.f);
                if (f->u.gzip_fh.pid > 0)
                        waitpid(f->u.gzip_fh.pid, NULL, 0);
                break;
        }
        return ret;
}

