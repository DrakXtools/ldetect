NAME = ldetect
LIB_MAJOR = 0.9
LIB_MINOR = 0
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

RPM ?= $(HOME)/rpm

build: $(binaries) $(libraries)

lspcidrake: lspcidrake.c libldetect.so

$(lib_major).$(LIB_MINOR): $(lib_objs)
	$(CC) -shared -Wl,-z,relro -Wl,-O1,-soname,$(lib_major) -o $@ $^ -lpci -lmodprobe -lz
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
	tar cfj ../$(NAME)-$(VERSION).tar.bz2 $(NAME)-$(VERSION)
	rm -rf $(NAME)-$(VERSION)

dist-git:
	@git archive --prefix=$(NAME)-$(VERSION)/ HEAD | bzip2 >../$(NAME)-$(VERSION).tar.bz2;


log:
	svn2cl --authors ../common/username.xml --accum

run:
	LD_LIBRARY_PATH=$(PWD)  ./lspcidrake
