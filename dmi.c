/* DMI (Desktop Management Interface)
   also called
   SMBIOS (System Management BIOS)
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"

char *dmidecode_file = NULL;


static const char *cat_BIOS[] = { "Vendor", "Version" };
static const char *cat_System[] = { "Manufacturer", "Product Name", "Version" };
static const char *cat_Base_Board[] = { "Manufacturer", "Product Name", "Version" };

struct category {
	const char *cat_name;
	unsigned int nb_fields;
	const char **fields;
};

static const struct category categories[] = {
	{ "BIOS", psizeof(cat_BIOS), cat_BIOS },
	{ "System", psizeof(cat_System), cat_System },
	{ "Base Board", psizeof(cat_Base_Board), cat_Base_Board },

};
static int nb_categories = psizeof(categories);

/*********************************************************************************/
static int streq(const char *s1, const char *s2) {
	return strcmp(s1, s2) == 0;
}
static int str_begins_with(const char *s, const char *prefix) {
	return strncmp(s, prefix, strlen(prefix)) == 0;
}
static int remove_suffix_in_place(char *s, const char *suffix) {
	unsigned int l = strlen(s);
	unsigned int l_suffix = strlen(suffix);
	if (l >= l_suffix && streq(s + l - l_suffix, suffix)) {
		s[l - l_suffix] = '\0';
		return 1;
	} else
		return 0;
}
static void remove_ending_spaces(char *s) {
	for (char *p = s + strlen(s) - 1; p >= s; p--) {
		if (*p != '\n' && *p != '\r' && *p != ' ' && *p != '\t')
			break;
		*p = '\0';
	}
}
static char *skip_leading_spaces(char *s) {
	for (; *s; s++)
		if (*s != '\n' && *s != '\r' && *s != ' ' && *s != '\t')
			break;
	return s;
}

/*********************************************************************************/
static char *get_after_colon(char *s) {
	char *p = strchr(s, ':');
	if (p) {
		*p = '\0';
		return p + (p[1] == ' ' ? 2 : 1);
	} else return NULL;
}

static const struct category *lookup_category(const char *cat_name) {
	for (int i = 0; i < nb_categories; i++)
		if (streq(categories[i].cat_name, cat_name))
			return &categories[i];
	return NULL;
}

static int lookup_field(const struct category *category, const char *field_name) {
	for (unsigned int i = 0; i < category->nb_fields; i++)
		if (streq(category->fields[i], field_name))
			return 1;
	return 0;
}

static char *lookup_criteria(criteria_t criteria, const char *field) {
	for (unsigned int i = 0; i < criteria.nb; i++)
		if (streq(criteria.criteria[i].name, field))
			return criteria.criteria[i].val;
	return NULL;
}

/*********************************************************************************/
static criteria_t criteria_from_dmidecode(void) {
	FILE *f;
	char buf[BUF_SIZE];

	criteria_t r = { .nb = 0, .criteria = NULL };

	if (!(f = dmidecode_file ? fopen(dmidecode_file, "r") : popen("dmidecode", "r"))) {
		perror("dmidecode");
		return r;
	}

	r.criteria = malloc(sizeof(*r.criteria) * MAX_DEVICES);

	const struct category *category = NULL;

	/* dmidecode output is less indented as of 2.7 */
	int tab_level = 1;
        if (fgets(buf, sizeof(buf) - 1, f)) {
		int major, minor;
		if (sscanf(buf, "# dmidecode %d.%d", &major, &minor) == 2 && major >= 2 && minor >= 7)
			tab_level = 0;
	}

	while (fgets(buf, sizeof(buf) - 1, f)) {
		if (!buf[0] || !buf[1] || (tab_level && buf[0] != '\t'))
			; /* don't care */
		else if (buf[tab_level] != '\t') {
			char *s = buf + tab_level;
			if (!str_begins_with(s, "DMI type ")) {
				remove_ending_spaces(s);
				remove_suffix_in_place(s, " Information");
				category = lookup_category(s);
			}
		} else if (category) {
			/* don't even look if we don't have an interesting category */
			char *s = buf + tab_level + 1;
			char *val = get_after_colon(s);
			if (val && lookup_field(category, s)) {
				criterion_t *criterion = &r.criteria[r.nb++];
				asprintf(&criterion->name, "%s/%s", category->cat_name, s);
				remove_ending_spaces(val);
				criterion->val = strdup(skip_leading_spaces(val));
			}
		}
	}
	if (dmidecode_file ? fclose(f) != 0 : pclose(f) == -1) {
		r.nb = 0;
		return r;
	}
	r.criteria = realloc(r.criteria, sizeof(*r.criteria) * r.nb);

	return r;
}

static dmi_hid_entries_t entries_matching_criteria(criteria_t criteria) {
	fh f;
	char buf[2048];
	dmi_hid_entries_t r;
	#define MAX_INDENT 20
	int valid[MAX_INDENT];
	char *constraints[MAX_INDENT];

	enum state { in_constraints, in_implies } state = in_implies;
	int was_a_blank_line = 1;

	r.nb = 0;
	f = fh_open("dmitable");

#define die(err) do { fprintf(stderr, "%s %d: " err "\n", "dmitable", line); exit(1); } while (0)

	r.entries = malloc(sizeof(*r.entries) * MAX_DEVICES);

#define foreach_indent(min, action) do { for (int i = min; i < MAX_INDENT; i++) { action; } } while (0)
	
	foreach_indent(0, valid[i] = 1; constraints[i] = NULL);

	int previous_refine = 0;

	for (int line = 1; fh_gets(buf, sizeof(buf) - 1, &f); line++) {
		char *s = skip_leading_spaces(buf);
		if (*s == '#') continue; // skip comments

		if (!*s) {
			was_a_blank_line = 1;
		} else {
			int refine = s - buf;
			if (refine > MAX_INDENT) die("too indented constraints");

			remove_ending_spaces(s);
			if (str_begins_with(s, "=> ")) {
				if (refine != previous_refine) die("\"=>\" must not be indented");
				state = in_implies;
				was_a_blank_line = 0;
				
				if (valid[refine]) {
					dmi_hid_entry_t *entry = &r.entries[r.nb++];

					s += strlen("=> ");
					char *val = get_after_colon(s);
					if (!val) die("invalid value");
					asprintf(&entry->module, "%s:%s", s, val);

					char tmp[BUF_SIZE];
					tmp[0] = '\0';

					for (int i = 0; i <= refine; i++)
						if (constraints[i]) {
							if (i) strncat(tmp, "|", BUF_SIZE);
							strncat(tmp, constraints[i], BUF_SIZE);
						}
					entry->constraints = strdup(tmp);
				}
			} else {
				if (state == in_constraints && refine == previous_refine) die("to refine, indent");
				if (!was_a_blank_line) die("missing empty line");
				state = in_constraints;
				previous_refine = refine;
				was_a_blank_line = 0;

				if (refine == 0 || valid[refine - 1]) {

					char *wanted_val = get_after_colon(s);
					if (!wanted_val) die("bad format");

					char *wanted_val_orig = strdup(wanted_val);
					char *val = lookup_criteria(criteria, s);

					int ok = wanted_val && val && (
						remove_suffix_in_place(wanted_val, ".*") ?
						str_begins_with(val, wanted_val) :
						streq(val, wanted_val));
				
					foreach_indent(refine, valid[i] = ok; ifree(constraints[i]));
					if (ok) 
						constraints[refine] = wanted_val_orig;
					else
						free(wanted_val_orig);

				} /* otherwise no need checking */				
			}			
		}
	}
	foreach_indent(0, ifree(constraints[i]));
	fh_close(&f);

	r.entries = realloc(r.entries, sizeof(*r.entries) * r.nb);
	return r;
}

dmi_hid_entries_t dmi_probe(void) {
	criteria_t criteria = criteria_from_dmidecode();
	dmi_hid_entries_t entries = entries_matching_criteria(criteria);
	free_entries(criteria);

	return entries;
}

