#include <fstream>
#include <libkmod.h>
#include <sys/types.h>
#include <dirent.h>

#include "gzstream.h"
#include "libldetect.h"
#include "common.h"
#include "dmi.h"

namespace ldetect {

void dmi::probe(void)
{
    struct dmiTable {
	std::string table;
	std::string name;
	std::string subtable;
	std::string attr;
	std::string item;
	std::string value;
    };

    instream fp = fh_open("dmitable");
    std::vector<dmiTable> dmitable;

    std::string subtableFirst, subtableSecond;
    std::string tableFirst, tableSecond;
    std::string buff;
    while (!fp->eof()) {
	getline(*fp, buff);
	const char *buf = buff.c_str();

	if (*buf == '#') continue; // skip comments
	char *sep = strchr(const_cast<char*>(buf), ':');
	if (!sep)
	    continue;
	if (isalpha(*buf))
	    tableFirst.assign(buf, sep-buf), tableSecond = sep+2;
	else if (buf[0] == ' ' && buf[1] == ' ') {
	    if (isalpha(buf[2]))
		subtableFirst.assign(buf+2, sep-buf-2), subtableSecond = sep+2;
	    else if (buf[2] == '=' && buf[3] == '>' && buf[4] == ' ' && isalpha(buf[5])) {
		dmitable.push_back({tableFirst, tableSecond, subtableFirst, subtableSecond, std::string(buf+5, sep-buf-5), std::string(sep+2)});
	    }
	}
    }

    struct kmod_ctx *ctx = modalias_init();
    DIR *dp;
    struct dirent *dirp;
    if((dp = opendir("/sys/class/dmi")) == nullptr)
	return;

    std::ifstream f;
    std::string dmiPath, f1val, f2val;
    while ((dirp = readdir(dp)) != nullptr) {
	if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
	    continue;
	size_t pos;
	dmiPath.assign("/sys/class/dmi/").append(dirp->d_name).append("/");
	for(std::vector<dmiTable>::const_iterator it = dmitable.begin(); it != dmitable.end(); ++it) {
	    f.open(dmiPath + it->table, std::ifstream::in);

	    if (f.is_open()) {
		std::getline(f, f1val);
		if (((pos = it->name.find_last_of(".*")) != std::string::npos &&
			    !it->name.compare(0, pos-1, f1val, 0, pos-1)) ||
			it->name == f1val) {
		    f.close();
		    f.open(dmiPath + it->subtable, std::ifstream::in);

		    if (f.is_open()) {
			std::getline(f, f2val);
			if ((((pos = it->attr.find_last_of(".*")) != std::string::npos &&
					!it->attr.compare(0, pos-1, f2val, 0, pos-1)) ||
				    it->attr == f2val) &&
				it->item == "Module")
			    _entries.push_back(dmiEntry(it->value, f1val + "|" + f2val));
		    }
		}
		f.close();
	    }
	}

	std::string deviceName;
	f.open(dmiPath + "sys_vendor", std::ifstream::in);
	if (f.is_open()) {
	    getline(f, deviceName);
	    f.close();
	}

	f.open(dmiPath + "product_name", std::ifstream::in);
	if (f.is_open()) {
	    if (!deviceName.empty())
		deviceName += "|";
	    std::string val;
	    getline(f, val);
	    deviceName += val;
	    f.close();
	}

	f.open(dmiPath + "modalias", std::ifstream::in);
	if (f.is_open()) {
	    std::string modalias;
	    getline(f, modalias);
	    std::string modname = modalias_resolve_module(ctx, modalias.c_str());
	    if (!modname.empty()) 
		_entries.push_back(dmiEntry(modname, deviceName));
	}
    }

    closedir(dp);
    kmod_unref(ctx);
}

std::ostream& operator<<(std::ostream& os, const dmiEntry& e) {
    return os << std::setw(16) << std::left << e.module << ": " << e.text;
}

}
