#ifndef LIBLDETECT_COMMON
#define LIBLDETECT_COMMON

#include "libldetect.h"
#include <zlib.h>

#define NON_EXPORTED __attribute__((visibility("hidden")))

typedef enum {
     LOAD,
     DO_NOT_LOAD,
} descr_lookup;

extern int pciusb_find_modules(struct pciusb_entries *entries, const char *fpciusbtable, const descr_lookup, int is_pci) NON_EXPORTED;
extern void pciusb_initialize(struct pciusb_entry *e) NON_EXPORTED;

#define MAX_DEVICES 100
#define BUF_SIZE 512


#define psizeof(a) (sizeof(a) / sizeof(*(a)))
#define ifree(p) do { if (p) { free(p); p = NULL; } } while (0)

typedef gzFile fh;
extern fh fh_open(const char *name) NON_EXPORTED;
#define fh_gets(line, size, f) gzgets(f, line, size)
#define fh_close(f) gzclose(f);

#endif
