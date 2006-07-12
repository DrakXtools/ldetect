LIB_MAJOR = 0.6
LIB_MINOR = 0

project = ldetect
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

rpm: srpm
	rpm -bb --clean --rmsource --rmspec $(project).spec

srpm: clean $(RPM)
	(cd .. ; tar cfj $(RPM)/SOURCES/$(project).tar.bz2 $(project))
	rpm -bs $(RPM)/SPECS/$(project).spec

log:
	cvs2cl -U ../common/username -I ChangeLog -W 400 --accum
