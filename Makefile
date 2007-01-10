NAME = ldetect
LIB_MAJOR = 0.6
LIB_MINOR = 7
VERSION=$(LIB_MAJOR).$(LIB_MINOR)

prefix = /usr
bindir = $(prefix)/bin
libdir = $(prefix)/lib
includedir = $(prefix)/include

binaries = lspcidrake
lib_major = libldetect.so.$(LIB_MAJOR)
libraries = libldetect.so $(lib_major) $(lib_major).$(LIB_MINOR)
CFLAGS = -Wall -W -Wstrict-prototypes -Os -fPIC

RPM ?= $(HOME)/rpm

build: $(binaries) $(libraries)

lspcidrake: lspcidrake.c libldetect.so

$(lib_major).$(LIB_MINOR): common.o pciusb.o pci.o usb.o pciclass.o usbclass.o dmi.o
	$(CC) -shared -Wl,-soname,$(lib_major) -o $@ $^
$(lib_major): $(lib_major).$(LIB_MINOR)
	ln -sf $< $@
libldetect.so: $(lib_major)
	ln -sf $< $@

pciclass.c: /usr/include/linux/pci.h /usr/include/linux/pci_ids.h
	rm -f $@
	perl generate_pciclass.pl $^ > $@
	chmod a-w $@

usbclass.c: /usr/share/usb.ids
	rm -f $@
	perl generate_usbclass.pl $^ > $@
	chmod a-w $@

pciusb.o:	pciusb.c libldetect.h common.h
pci.o:	pci.c libldetect.h common.h
usb.o:	usb.c libldetect.h common.h
dmi.o:	dmi.c libldetect.h common.h

clean:
	rm -f *~ *.o pciclass.c usbclass.c $(binaries) $(libraries)

install: build
	install -d $(bindir) $(libdir) $(includedir)
	install $(binaries) $(bindir)
	cp -a $(libraries) $(libdir)
	install libldetect.h $(includedir)

dis ../$(NAME)-$(VERSION).tar.bz2: clean
	rm -rf $(NAME)-$(VERSION)
	mkdir -p $(NAME)-$(VERSION)
	find -not -name $(NAME)-$(VERSION) | cpio -pd $(NAME)-$(VERSION)/
	find $(NAME)-$(VERSION) -type d -name .svn | xargs rm -rf
	tar cfj ../$(NAME)-$(VERSION).tar.bz2 $(NAME)-$(VERSION)
	rm -rf $(NAME)-$(VERSION)

rpm: dis $(RPM)
	rpm -ta ../$(NAME)-$(VERSION).tar.bz2

srpm: dis $(RPM)
	rpm -ts ../$(NAME)-$(VERSION).tar.bz2

log:
	svn2cl --authors ../common/username.xml --accum
