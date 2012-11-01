#ifndef LIBLDETECT_COMMON
#define LIBLDETECT_COMMON

#include "libldetect.h"
#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#define NON_EXPORTED __attribute__((visibility("hidden")))

#pragma GCC visibility push(hidden) 

#define GZIP_BIN "/bin/gzip"

char *table_name_to_file(const char *name);

typedef enum {
     LOAD,
     DO_NOT_LOAD,
} descr_lookup;

int pciusb_find_modules(std::vector<pciusb_entry> &entries, const char *fpciusbtable, const descr_lookup, int is_pci) NON_EXPORTED;
struct kmod_ctx* modalias_init(void) NON_EXPORTED;
std::string modalias_resolve_module(struct kmod_ctx *ctx, const char *modalias) NON_EXPORTED;
void modalias_cleanup(struct kmod_ctx *ctx) NON_EXPORTED;

#define MAX_DEVICES 100
#define BUF_SIZE 512

typedef enum { ZLIB, GZIP } gztype_t;
typedef struct {
	gztype_t gztype;
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

#define psizeof(a) (sizeof(a) / sizeof(a[0]))
#define ifree(p) do { if (p) { free(p); p = NULL; } } while (0)
#define alloc_v(v) calloc(1, sizeof(v[0]));

fh fh_open(const char *name) NON_EXPORTED;
char* fh_gets(char *line, int size, fh *f) NON_EXPORTED;
int fh_close(fh *f) NON_EXPORTED;
#pragma GCC visibility pop

#endif
