project = ldetect
prefix = /usr
bindir = $(prefix)/bin
libdir = $(prefix)/lib
includedir = $(prefix)/include

binaries = lspcidrake
libraries = libldetect.a
CFLAGS = -Wall -Wstrict-prototypes -g

build: $(binaries) $(libraries)

lspcidrake: lspcidrake.c libldetect.a

libldetect.a: pci.o pciclass.o
	ar rsc $@ $^

pciclass.c: /usr/include/linux/pci.h generate_pciclass.pl
	rm -f $@
	perl generate_pciclass.pl < $< > $@
	chmod a-w $@

clean:
	rm -f *~ *.o pciclass.c $(binaries) $(libraries)

install: build
	install -d $(bindir) $(libdir) $(includedir)
	install $(binaries) $(bindir)
	install $(libraries) $(libdir)
	install libldetect.h $(includedir)

rpm: clean $(RPM)
	(echo "# !! DON'T MODIFY HERE, MODIFY IN THE CVS !!" ; \
         cat $(project).spec \
        ) > $(RPM)/SPECS/$(project).spec

	(cd .. ; tar cfj $(RPM)/SOURCES/$(project).tar.bz2 $(project))
	rpm -ba --clean --rmsource --rmspec $(RPM)/SPECS/$(project).spec
