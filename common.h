#define psizeof(a) (sizeof(a) / sizeof(*(a)))
#define ifree(p) do { if (p) { free(p); p = NULL; } } while (0)


typedef struct {
	FILE *f;
	pid_t pid;
} fh;
extern fh fh_open(const char *name);
extern void fh_close(fh *f);
