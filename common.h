#ifndef LIBLDETECT_COMMON
#define LIBLDETECT_COMMON

#include "libldetect.h"
#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#define NON_EXPORTED __attribute__((visibility("hidden")))

#pragma GCC visibility push(hidden) 

#define GZIP_BIN "/bin/gzip"

extern char *table_name_to_file(const char *name);

typedef enum {
     LOAD,
     DO_NOT_LOAD,
} descr_lookup;

extern int pciusb_find_modules(struct pciusb_entries *entries, const char *fpciusbtable, const descr_lookup, int is_pci) NON_EXPORTED;
extern void pciusb_initialize(struct pciusb_entry *e) NON_EXPORTED;
extern char *modalias_resolve_module(const char *modalias) NON_EXPORTED;
extern void modalias_cleanup(void) NON_EXPORTED;

#define MAX_DEVICES 100
#define BUF_SIZE 512

typedef struct {
	enum { ZLIB, GZIP } gztype;
	union {
		struct {
			FILE *f;
			pid_t pid;
		} gzip_fh;
#ifdef HAVE_LIBZ
		gzFile zlib_fh;
#endif
	} u;
} fh;

#define psizeof(a) (sizeof(a) / sizeof(*(a)))
#define ifree(p) do { if (p) { free(p); p = NULL; } } while (0)

extern fh fh_open(const char *name) NON_EXPORTED;
extern char* fh_gets(char *line, int size, fh *f) NON_EXPORTED;
extern int fh_close(fh *f) NON_EXPORTED;
#pragma GCC visibility pop

#endif
