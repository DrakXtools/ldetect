#include <stdlib.h>

#include "libldetect.h"

void free_entries(struct entries_s entries) {
	for (unsigned int i = 0; i < entries.nb; i++) {
		free(entries.entries[i].constraints);
		free(entries.entries[i].module);
	}
	free(entries.entries);
}
