NAME = ldetect
LIB_MAJOR = 0.12
LIB_MINOR = 2
VERSION=$(LIB_MAJOR).$(LIB_MINOR)

prefix = /usr
bindir = $(prefix)/bin
libdir = $(prefix)/lib
includedir = $(prefix)/include

binaries = lspcidrake
lib_objs = common.o hid.o modalias.o pciusb.o pci.o usb.o pciclass.o usbclass.o dmi.o sysfs_attr.o sysfs_utils.o names.o
lib_major = libldetect.so.$(LIB_MAJOR)
libraries = libldetect.so $(lib_major) $(lib_major).$(LIB_MINOR) libldetect.a
CFLAGS = -Wall -W -Wstrict-prototypes -Os -fPIC -fvisibility=hidden -g
CPPFLAGS += $(shell getconf LFS_CFLAGS) $(shell pkg-config --cflags libkmod libpci liblzma zlib)
LIBS += $(shell pkg-config --libs libkmod libpci liblzma zlib)

RPM ?= $(HOME)/rpm

build: $(binaries) $(libraries)

lspcidrake.static: lspcidrake.c libldetect.a
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lcompat -lc $(LIBS)
	#$(CC) $(CFLAGS) $^ libldetect.a -lkmod -lxz -lz.a  -o $@

lspcidrake: lspcidrake.c libldetect.so

$(lib_major).$(LIB_MINOR): $(lib_objs)
	$(CC) $(LDFLAGS) -shared -Wl,-z,relro -Wl,-O1,-soname,$(lib_major) -o $@ $^ $(LIBS)
$(lib_major): $(lib_major).$(LIB_MINOR)
	ln -sf $< $@
libldetect.so: $(lib_major)
	ln -sf $< $@

libldetect.a: $(lib_objs)
	ar -cru $@ $^
	ranlib $@

pciclass.c: /usr/include/linux/pci.h /usr/include/linux/pci_ids.h
	rm -f $@
	perl generate_pciclass.pl $^ > $@
	chmod a-w $@

usbclass.c: /usr/share/usb.ids
	rm -f $@
	perl generate_usbclass.pl $^ > $@
	chmod a-w $@

common.o:	common.c common.h
pciusb.o:	pciusb.c libldetect.h common.h
pci.o:	pci.c libldetect.h common.h
usb.o:	usb.c libldetect.h common.h names.h
dmi.o:	dmi.c libldetect.h common.h
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
