#ifndef LIBLDETECT
#define LIBLDETECT

#include <cstdint>
#include <vector>
#include <string>
#include <iostream>

#pragma GCC visibility push(default)
#define EXPORTED __attribute__((externally_visible)) __attribute__((visibility("default")))

namespace ldetect {

    class entry {
	public:
	    entry(const std::string &module, const std::string &text) EXPORTED :
		module(module), text(text) {}
	    entry() : module(), text() {}
	    virtual ~entry() {}

	    std::string module;
	    std::string text;

	    friend std::ostream& operator<<(std::ostream& os, const entry& e) EXPORTED;
    };

    class bus {
	public:
	    bus() {}
	    virtual ~bus() {}

	    virtual void probe(void) = 0;
    };

/******************************************************************************/
/* dmi & hid ******************************************************************/
/******************************************************************************/

/******************************************************************************/
/* dmi ************************************************************************/
/******************************************************************************/

/******************************************************************************/
/* hid ************************************************************************/
/******************************************************************************/

}

#endif
