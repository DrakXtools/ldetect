project = ldetect
prefix = /usr
bindir = $(prefix)/bin
libdir = $(prefix)/lib
includedir = $(prefix)/include

binaries = lspcidrake
libraries = libldetect.a
CFLAGS = -Wall -W -Wstrict-prototypes -Os -g

RPM ?= $(HOME)/rpm

build: $(binaries) $(libraries)

lspcidrake: lspcidrake.c libldetect.a

libldetect.a: pciusb.o pci.o usb.o pciclass.o usbclass.o
	ar rsc $@ $^

pciclass.c: /usr/include/linux/pci.h /usr/include/linux/pci_ids.h
	rm -f $@
	perl generate_pciclass.pl $^ > $@
	chmod a-w $@

usbclass.c: /usr/share/usb.ids
	rm -f $@
	perl generate_usbclass.pl $^ > $@
	chmod a-w $@

pciusb.o:	pciusb.c libldetect.h libldetect-private.h common.h
pci.o:	pci.c libldetect.h libldetect-private.h common.h
usb.o:	usb.c libldetect.h libldetect-private.h common.h

clean:
	rm -f *~ *.o pciclass.c usbclass.c $(binaries) $(libraries)

install: build
	install -d $(bindir) $(libdir) $(includedir)
	install $(binaries) $(bindir)
	install $(libraries) $(libdir)
	install libldetect.h $(includedir)

rpm: srpm
	rpm -bb --clean --rmsource --rmspec $(RPM)/SPECS/$(project).spec

srpm: clean $(RPM)
	(echo "# !! DON'T MODIFY HERE, MODIFY IN THE CVS !!" ; \
         cat $(project).spec \
        ) > $(RPM)/SPECS/$(project).spec

	(cd .. ; tar cfj $(RPM)/SOURCES/$(project).tar.bz2 $(project))
	rpm -bs $(RPM)/SPECS/$(project).spec