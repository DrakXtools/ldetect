/* DMI (Desktop Management Interface)
   also called
   SMBIOS (System Management BIOS)
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "libldetect.h"

namespace ldetect {

const char *dmidecode_file = nullptr;


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
	} else return nullptr;
}

static const struct category *lookup_category(const char *cat_name) {
	for (int i = 0; i < nb_categories; i++)
		if (streq(categories[i].cat_name, cat_name))
			return &categories[i];
	return nullptr;
}

static int lookup_field(const struct category *category, const char *field_name) {
	for (unsigned int i = 0; i < category->nb_fields; i++)
		if (streq(category->fields[i], field_name))
			return 1;
	return 0;
}

static const char* lookup_criteria(const std::vector<entry> &criteria, const char *field) {
	for (unsigned int i = 0; i < criteria.size(); i++)
		if (streq(criteria[i].name.c_str(), field))
			return criteria[i].val.c_str();
	return nullptr;
}

/*********************************************************************************/
static std::vector<entry>* criteria_from_dmidecode(void) {
	FILE *f;
	char buf[BUF_SIZE];

	std::vector<entry> *criteria = nullptr;

	if (!(f = dmidecode_file ? fopen(dmidecode_file, "r") : popen("dmidecode", "r"))) {
		perror("dmidecode");
		return criteria;
	}

	criteria = new std::vector<entry>(0);

	const struct category *category = nullptr;

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
				std::string name = category->cat_name;
				name += "/", name += s;
				remove_ending_spaces(val);
				criteria->push_back(entry(name, skip_leading_spaces(val)));
			}
		}
	}
	if (dmidecode_file ? fclose(f) != 0 : pclose(f) == -1) {
		criteria->clear();
		return criteria;
	}

	return criteria;
}

static std::vector<entry>* entries_matching_criteria(const std::vector<entry> &criteria) {
	fh f;
	char buf[2048];
	std::vector<entry> *entries;
	#define MAX_INDENT 20
	int valid[MAX_INDENT];
	char *constraints[MAX_INDENT];

	enum state { in_constraints, in_implies } state = in_implies;
	int was_a_blank_line = 1;

	f = fh_open("dmitable");

#define die(err) do { fprintf(stderr, "%s %d: " err "\n", "dmitable", line); exit(1); } while (0)

	entries = new std::vector<entry>(0);

#define foreach_indent(min, action) do { for (int i = min; i < MAX_INDENT; i++) { action; } } while (0)
	
	foreach_indent(0, valid[i] = 1; constraints[i] = nullptr);

	int previous_refine = 0;

	for (int line = 1; fh_gets(buf, sizeof(buf) - 1, f); line++) {
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
					s += strlen("=> ");
					char *val = get_after_colon(s);
					if (!val) die("invalid value");
					std::string module(s);
					module += ":", module += val;

					std::string constraint;
					for (int i = 0; i <= refine; i++)
						if (constraints[i]) {
							if (i) constraint += "|";
							constraint += constraints[i];
						}
					entries->push_back(entry(module, constraint));
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
					const char *val = lookup_criteria(criteria, s);

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
	fh_close(f);

	return entries;
}

std::vector<entry>* dmi_probe(void) {
	std::vector<entry>* criteria = criteria_from_dmidecode();
	std::vector<entry>* entries = entries_matching_criteria(*criteria);
	delete criteria;

	return entries;
}

}
