lib_src = common.c modalias.c pciusb.c pci.c usb.c pciclass.c usbclass.c dmi.c dmi_hid.c hid.c sysfs_attr.c sysfs_utils.c names.c
lib_objs = $(subst .c,.o,$(lib_src))
lib_major = libldetect.so.$(LIB_MAJOR)
libraries = libldetect.so $(lib_major) $(lib_major).$(LIB_MINOR) libldetect.a
CFLAGS += -std=gnu99 -Wall -W -Wstrict-prototypes -Os -fPIC -fvisibility=hidden -g
CPPFLAGS += $(shell getconf LFS_CFLAGS) $(shell pkg-config --cflags libkmod libpci)
LIBS += $(shell pkg-config --libs libkmod libpci)
ifneq ($(ZLIB),0)
CPPFLAGS += $(shell pkg-config --cflags zlib liblzma) -DHAVE_LIBZ
LIBS += $(shell pkg-config --libs zlib liblzma)
endif
WHOLE_PROGRAM = 1

ldetect_srcdir ?= .

$(ldetect_srcdir)/pciclass.c: /usr/include/pci/pci.h /usr/include/pci/header.h
	rm -f $@
	perl $(ldetect_srcdir)/generate_pciclass.pl $^ > $@

$(ldetect_srcdir)/usbclass.c: /usr/share/usb.ids
	rm -f $@
	perl $(ldetect_srcdir)/generate_usbclass.pl $^ > $@

ifndef MDK_STAGE_ONE
NAME = ldetect
LIB_MAJOR = 0.13
LIB_MINOR = 0
VERSION=$(LIB_MAJOR).$(LIB_MINOR)

prefix = /usr
bindir = $(prefix)/bin
libdir = $(prefix)/lib
includedir = $(prefix)/include

binaries = lspcidrake

build: $(binaries) $(libraries)

ifneq (0, $(WHOLE_PROGRAM))
lspcidrake.static: lspcidrake.c $(lib_src)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -Os -fwhole-program -flto -Wl,-O2 -o $@ $^ $(LIBS)

lspcidrake: lspcidrake.c libldetect.so
	$(CC) $(CFLAGS) $(LDFLAGS) -Os -fwhole-program -flto -Wl,-z,relro -Wl,-O2 -o $@ $^

$(lib_major).$(LIB_MINOR): $(lib_src)
	$(CC) $(CFLAGS) $(LDFLAGS) -Os -fwhole-program -flto -shared -Wl,-z,relro -Wl,-O2,-soname,$(lib_major) -o $@ $^ $(LIBS)
else
lspcidrake.static: lspcidrake.c libldetect.a
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

lspcidrake: lspcidrake.c libldetect.so

$(lib_major).$(LIB_MINOR): $(lib_objs)
	$(CC) $(LDFLAGS) -shared -Wl,-z,relro -flto -Wl,-O2,-soname,$(lib_major) -o $@ $^ $(LIBS)
endif
$(lib_major): $(lib_major).$(LIB_MINOR)
	ln -sf $< $@
libldetect.so: $(lib_major)
	ln -sf $< $@

libldetect.a: $(lib_objs)
	ar -cru $@ $^
	ranlib $@

common.o:	common.c common.h
pciusb.o:	pciusb.c libldetect.h common.h
pci.o:		pci.c libldetect.h common.h
usb.o:		usb.c libldetect.h common.h names.h
dmi.o:		dmi.c libldetect.h common.h
dmi_hid.o:	dmi_hid.c libldetect.h common.h
hid.o:		hid.c libldetect.h common.h
names.o:	names.c names.h
sysfs_attr.o:	sysfs_attr.c sysfs.h libsysfs.h
sysfs_utils.o:	sysfs_utils.c sysfs.h libsysfs.h

clean:
	rm -f *~ *.o pciclass.c usbclass.c $(binaries) $(libraries)

install: build
	install -d $(bindir) $(libdir) $(includedir)
	install $(binaries) $(bindir)
	cp -a $(libraries) $(libdir)
	install libldetect.h $(includedir)

dist: dis
dis ../$(NAME)-$(VERSION).tar.bz2: tar

tar:
	@if [ -e ".svn" ]; then \
		$(MAKE) dist-svn; \
	elif [ -e ".git" ]; then \
		$(MAKE) dist-git; \
	else \
		echo "Unknown SCM (not SVN nor GIT)";\
		exit 1; \
	fi;
	$(info $(NAME)-$(VERSION).tar.bz2 is ready)

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
