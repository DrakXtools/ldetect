#ifndef LIBLDETECT
#define LIBLDETECT

#include <cstdint>
#include <vector>
#include <string>

#pragma GCC visibility push(default)
#define EXPORTED __attribute__((externally_visible)) __attribute__((visibility("default")))

namespace ldetect {

struct entry {
		entry(std::string name = "", std::string val = "") : name(name), val(val) {};
		std::string name;
		std::string val;


};

/******************************************************************************/
/* dmi & hid ******************************************************************/
/******************************************************************************/

/******************************************************************************/
/* dmi ************************************************************************/
/******************************************************************************/

std::vector <entry>* dmi_probe(void) EXPORTED;
extern const char *dmidecode_file EXPORTED;

/******************************************************************************/
/* hid ************************************************************************/
/******************************************************************************/

}

#endif
