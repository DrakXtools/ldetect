headers = gzstream.h lspcidrake.h names.h
headers_api = common.h dmi.h hid.h libldetect.h pci.h pciusb.h usb.h
lib_src = common.cpp modalias.cpp pciusb.cpp pci.cpp usb.cpp pciclass.cpp usbclass.cpp dmi.cpp hid.cpp names.cpp gzstream.cpp libldetect.cpp
lib_objs = $(subst .cpp,.o,$(lib_src))
lib_major = libldetect.so.$(LIB_MAJOR)
libraries = libldetect.so $(lib_major) $(lib_major).$(LIB_MINOR) libldetect.a
CXXFLAGS += -Wall -Wextra -pedantic -Os -fPIC -fvisibility=hidden -g 
LDFLAGS += -Wl,--no-undefined
ifeq (uclibc, $(LIBC))
CC=uclibc-gcc
CXX=uclibc-g++ -std=gnu++11
CXXFLAGS += -fno-exceptions -fno-rtti
else
CXX = g++ -std=gnu++11
CXXFLAGS += -Weffc++ 
endif
CPPFLAGS += $(shell getconf LFS_CFLAGS) $(shell pkg-config --cflags libkmod libpci)
LIBS += $(shell pkg-config --libs libkmod libpci)
ifneq ($(ZLIB),0)
CPPFLAGS += $(shell pkg-config --cflags zlib) -DHAVE_LIBZ
LIBS += $(shell pkg-config --libs zlib)
endif
WHOLE_PROGRAM = 1
FLTO = -flto

ldetect_srcdir ?= .

ifndef MDK_STAGE_ONE
NAME = ldetect
LIB_MAJOR = 0.13
LIB_MINOR = 1
VERSION=$(LIB_MAJOR).$(LIB_MINOR)

prefix = /usr
bindir = $(prefix)/bin
libdir = $(prefix)/lib
includedir = $(prefix)/include

binaries = lspcidrake

all: $(binaries) $(libraries) .depend

.depend: $(headers) $(headers_api) $(lib_src) lspcidrake.cpp
	$(CXX) $(DEFS) $(INCLUDES) $(CXXFLAGS) -M $^ > .depend 

ifeq (.depend,$(wildcard .depend))
include .depend
endif

ifneq (0, $(WHOLE_PROGRAM))
lspcidrake.static: lspcidrake.cpp $(lib_src)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -Os -fwhole-program -Wl,--no-warn-common $(FLTO) -Wl,-O1 -o $@ $^ $(LIBS)

lspcidrake: lspcidrake.cpp libldetect.so
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -Os -fwhole-program -Wl,--no-warn-common $(FLTO) -Wl,-z,relro -Wl,-O1 -o $@ $^

$(lib_major).$(LIB_MINOR): $(lib_src)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -Os -fwhole-program -Wl,--no-warn-common $(FLTO) -shared -Wl,-z,relro -Wl,-O1,-soname,$(lib_major) -o $@ $^ $(LIBS)
else
lspcidrake.static: lspcidrake.cpp libldetect.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

lspcidrake: lspcidrake.cpp libldetect.so
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

$(lib_major).$(LIB_MINOR): $(lib_objs)
	$(CXX) $(LDFLAGS) -shared -Wl,-z,relro $(FLTO) -Wl,-O1,-soname,$(lib_major) -o $@ $^ $(LIBS)
endif
$(lib_major): $(lib_major).$(LIB_MINOR)
	ln -sf $< $@
libldetect.so: $(lib_major)
	ln -sf $< $@

libldetect.a: $(lib_objs)
	ar -cru $@ $^
	ranlib $@

common.o:	common.cpp common.h
pciusb.o:	pciusb.cpp libldetect.h common.h
pci.o:		pci.cpp libldetect.h common.h
usb.o:		usb.cpp libldetect.h common.h names.h
dmi.o:		dmi.cpp libldetect.h common.h
hid.o:		hid.cpp libldetect.h common.h
names.o:	names.cpp names.h

clean:
	rm -f *~ *.o pciclass.cpp usbclass.cpp $(binaries) $(libraries) .depend

install: $(binaries) $(libraries)
	install -d $(DESTDIR)$(bindir) $(DESTDIR)$(libdir)/pkgconfig $(DESTDIR)$(includedir)/ldetect
	install -m755 $(binaries) $(DESTDIR)$(bindir)
	cp -a $(libraries) $(DESTDIR)$(libdir)
	install -m644 $(headers_api) $(DESTDIR)$(includedir)/ldetect
	sed -e "s#@PREFIX@#$(prefix)#g" -e "s#@LIBDIR@#$(libdir)#g" -e "s#@INCLUDEDIR@#$(includedir)#g" \
		-e "s#@VERSION@#$(VERSION)#g" ldetect.pc.in > $(DESTDIR)$(libdir)/pkgconfig/ldetect.pc

dist: dis
dis ../$(NAME)-$(VERSION).tar.xz: tar

tar:
	@if [ -e ".svn" ]; then \
		$(MAKE) dist-svn; \
	elif [ -e ".git" ]; then \
		$(MAKE) dist-git; \
	else \
		echo "Unknown SCM (not SVN nor GIT)";\
		exit 1; \
	fi;
	$(info $(NAME)-$(VERSION).tar.xz is ready)

dist-svn:
	svn export -q -rBASE . $(NAME)-$(VERSION)
	tar cfa ../$(NAME)-$(VERSION).tar.xz $(NAME)-$(VERSION)
	rm -rf $(NAME)-$(VERSION)

dist-git:
	@git archive --prefix=$(NAME)-$(VERSION)/ HEAD | xz >../$(NAME)-$(VERSION).tar.xz;


log:
	svn2cl --authors ../common/username.xml --accum

run: lspcidrake
	LD_LIBRARY_PATH=$(PWD)  ./lspcidrake
endif

$(ldetect_srcdir)/pciclass.cpp: $(ldetect_srcdir)/generate_pciclass.py /usr/include/pci/pci.h /usr/include/pci/header.h
	rm -f $@
	python $(ldetect_srcdir)/generate_pciclass.py $@ $^

$(ldetect_srcdir)/usbclass.cpp: $(ldetect_srcdir)/generate_usbclass.pl /usr/share/usb.ids 
	rm -f $@
	perl $(ldetect_srcdir)/generate_usbclass.pl $^ > $@

