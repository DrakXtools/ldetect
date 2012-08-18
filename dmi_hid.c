#include <stdlib.h>

#include "libldetect.h"

void dmi_hid_entries_free(struct dmi_hid_entries entries) {
	for (unsigned int i = 0; i < entries.nb; i++) {
		free(entries.entries[i].constraints);
		free(entries.entries[i].module);
	}
	free(entries.entries);
}
