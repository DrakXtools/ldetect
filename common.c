
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "common.h"

static char *table_name_to_file(const char *name) {

	char *share_path = getenv("SHARE_PATH");
	char *fname;
	if (!share_path || !*share_path) share_path = "/usr/share";

	asprintf(&fname, "%s/ldetect-lst/%s", share_path, name);
	return fname;
}

extern fh fh_open(const char *name) {
	fh ret;
	char *fname = table_name_to_file(name);
	int length = strlen(fname);

	if (access(fname, R_OK) == 0) {
		ret.f = fopen(fname, "r");
		ret.pid = 0;
	} else {
		int fdno[2];
		char *fname_gz = alloca(length + sizeof(".gz"));
		sprintf(fname_gz, "%s.gz", fname);
		if (access(fname_gz, R_OK) != 0) {
			fprintf(stderr, "Missing %s (should be %s)\n", name, fname);
			exit(1);
		}
		if (pipe(fdno)) {
			perror("pciusb");
			exit(1);
		}

		if ((ret.pid = fork()) != 0) {
			ret.f = fdopen(fdno[0], "r");
			close(fdno[1]);
		} else {
			char* cmd[5];
			int ip = 0;
			char *ld_loader = getenv("LD_LOADER");

			if (ld_loader && *ld_loader)
				cmd[ip++] = ld_loader;

			cmd[ip++] = "gzip";
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
	free(fname);
	return ret;
}

extern void fh_close(fh *f) {
	fclose(f->f);
	if (f->pid > 0)
		waitpid(f->pid, NULL, 0);
}
