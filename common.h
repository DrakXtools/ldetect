#ifndef LIBLDETECT_COMMON
#define LIBLDETECT_COMMON

#include <string>
#include <sstream>
#include <iomanip>

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#include "libldetect.h"

#define NON_EXPORTED __attribute__((visibility("hidden")))

#pragma GCC visibility push(hidden) 

#define GZIP_BIN "/bin/gzip"

namespace ldetect {

std::string table_name_to_file(std::string name);

std::string hexFmt(uint32_t value, uint8_t w = 4, bool prefix = true);

struct kmod_ctx* modalias_init(void) NON_EXPORTED;
#pragma GCC visibility push(default)
std::string modalias_resolve_module(struct kmod_ctx *ctx, const char *modalias) EXPORTED;
void modalias_cleanup(struct kmod_ctx *ctx) EXPORTED;
#pragma GCC visibility pop

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
#define ifree(p) do { if (p) { free(p); p = nullptr; } } while (0)
#define alloc_v(v) calloc(1, sizeof(v[0]));

fh fh_open(std::string name) NON_EXPORTED;
char* fh_gets(char *line, int size, fh *f) NON_EXPORTED;
int fh_close(fh *f) NON_EXPORTED;
}

#pragma GCC visibility pop

#endif
