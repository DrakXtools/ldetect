#include <cstring>

#include "gzstream.h"

namespace ldetect {

gzstreambuf::gzstreambuf(const gzstreambuf &gzsb)
    : std::streambuf(), file(gzsb.file), opened(gzsb.opened) {
    memcpy(buffer, gzsb.buffer, sizeof(buffer));
}

gzstreambuf* gzstreambuf::open(const char *name) {
    if (is_open())
	return (gzstreambuf*)nullptr;
    file = gzopen(name, "rb");
    if (file == nullptr)
	return (gzstreambuf*)nullptr;
    opened = true;
    return this;
}

gzstreambuf * gzstreambuf::close() {
    if (is_open()) {
	sync();
	opened = false;
	if (gzclose(file) == Z_OK)
	    return this;
    }
    return (gzstreambuf*)nullptr;
}

std::streambuf::int_type gzstreambuf::underflow() {
    if (gptr() && (gptr() < egptr()))
	return *reinterpret_cast<uint8_t*>(gptr());

    if (!opened)
	return EOF;
    int n_putback = gptr() - eback();
    if (n_putback > 4)
	n_putback = 4;
    memcpy(buffer + (4 - n_putback), gptr() - n_putback, n_putback);

    int num = gzread(file, buffer+4, bufferSize-4);
    if (num <= 0)
	return EOF;

    setg(buffer + (4 - n_putback),
	    buffer + 4,
	    buffer + 4 + num);

    return *reinterpret_cast<uint8_t*>(gptr());    
}

int gzstreambuf::flush_buffer() {
    int w = pptr() - pbase();
    if (gzwrite(file, pbase(), w) != w)
	return EOF;
    pbump(-w);
    return w;
}

int gzstreambuf::sync() {
    if (pptr() && pptr() > pbase()) {
	if (flush_buffer() == EOF)
	    return -1;
    }
    return 0;
}

gzstreambase::gzstreambase(const char *name) : buf(){
    init(&buf);
    open(name);
}

gzstreambase::~gzstreambase() {
    buf.close();
}

void gzstreambase::open(const char *name) {
    if (!buf.open(name))
	clear(rdstate() | std::ios::badbit);
}

void gzstreambase::close() {
    if (buf.is_open() && !buf.close())
	clear(rdstate() | std::ios::badbit);
}

}
