#define psizeof(a) (sizeof(a) / sizeof(*(a)))

static inline void *memdup(void *src, size_t size) {
  void *r = malloc(size);
  memcpy(r, src, size);
  return r;
}
static inline void ifree(void *p) {
  if (p) { free(p); p = NULL; }
}
