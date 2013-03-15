#ifndef LIBLDETECT
#define LIBLDETECT

#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <memory>

#pragma GCC visibility push(default)
#define EXPORTED __attribute__((externally_visible)) __attribute__((visibility("default")))
#define NON_EXPORTED __attribute__((visibility("hidden")))

namespace ldetect {

#ifdef __UCLIBCXX_MAJOR__
    typedef std::auto_ptr<std::istream> instream;
#else
    typedef std::unique_ptr<std::istream> instream;
#endif
    instream i_open(std::string &&name) NON_EXPORTED;

    class entry {
	public:
	    entry(const std::string &module, const std::string &text) EXPORTED :
		module(module), kmodules(), text(text) {}
	    entry() : module(), kmodules(), text() {}
	    virtual ~entry() {}

	    std::string module;
	    std::vector<std::string> kmodules;
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
