#define psizeof(a) (sizeof(a) / sizeof(*(a)))

static inline void *memdup(void *src, size_t size) {
  void *r = malloc(size);
  memcpy(r, src, size);
  return r;
}
#define ifree(p) do { if (p) { free(p); p = NULL; } } while (0)
