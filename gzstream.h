#ifndef GZSTREAM_H
#define GZSTREAM_H 1

#include <istream>
#include <fstream>
#include <zlib.h>

namespace ldetect {

class gzstreambuf : public std::streambuf {
    public:
	gzstreambuf(const char *name=nullptr)
	    : file(name ? gzopen(name, "rb") : nullptr), opened(false) {
	    setp(buffer, buffer + (bufferSize-1));
	    setg(buffer + 4,
		    buffer + 4,
		    buffer + 4);
	}
	gzstreambuf(const gzstreambuf &gzsb);
	const  gzstreambuf &operator=(const gzstreambuf &gzsb) { return (*this = gzsb); }

	bool is_open() { return opened; }
	gzstreambuf* open(const char *name);
	gzstreambuf* close();
	~gzstreambuf() { close(); }

	virtual int     underflow();
	virtual int	sync();

    private:
	int		flush_buffer();
	static const int bufferSize = 47+256;
	// totals 512 bytes under g++ for igzstream at the end.

	gzFile		file;
	char		buffer[bufferSize];
	bool		opened;

};

class gzstreambase : virtual public std::ios {
    public:
	gzstreambase() : buf(nullptr) { init(&buf); }
	gzstreambase(const char *name);
	~gzstreambase();
	void open(const char *name);
	void close();
	gzstreambuf* rdbuf() { return &buf; }

    protected:
	gzstreambuf buf;

};

class igzstream : public gzstreambase, public std::istream {
    public:
	igzstream() : std::istream(&buf) {} 
	igzstream(const char *name) : gzstreambase(name), std::istream(&buf) {}  
	gzstreambuf* rdbuf() { return gzstreambase::rdbuf(); }
	void open(const char *name) {
	    gzstreambase::open(name);
	}
};

}

#endif
