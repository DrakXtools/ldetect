%define lib_major 0.6
%define lib_minor 3
%define lib_name %mklibname %{name} %{lib_major}

Name:    ldetect
Version: %{lib_major}.%{lib_minor}
Release: 1mdk
Summary: Light hardware detection tool
Source: %name.tar.bz2
Group: System/Libraries
URL:	  http://www.mandrakelinux.com
BuildRoot: %_tmppath/%{name}-buildroot
BuildRequires: usbutils => 0.11-2mdk,  pciutils-devel
Conflicts: drakxtools < 9.2-0.32mdk
License: GPL

%package -n %{lib_name}
Summary: Light hardware detection library
Requires: ldetect-lst common-licenses
Group: System/Libraries

%package -n %{lib_name}-devel
Summary: Development package for ldetect
Requires: %{lib_name} = %{version}
Provides: ldetect-devel libldetect-devel
Obsoletes: ldetect-devel
Group: Development/C

%description
The hardware device lists provided by this package are used as lookup 
table to get hardware autodetection

%description -n %{lib_name}-devel
see %name

%description -n %{lib_name}
see %name

%prep
%setup -q -n %name

%build
%make

%install
rm -rf $RPM_BUILD_ROOT
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%post -n %{lib_name} -p /sbin/ldconfig

%postun -n %{lib_name} -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc AUTHORS
%_bindir/*

%files -n %{lib_name}
%defattr(-,root,root)
%_libdir/*.so.*

%files -n %{lib_name}-devel
%defattr(-,root,root)
%doc ChangeLog
%_includedir/*
%_libdir/*.so

%changelog
* Fri Aug  5 2005 Thierry Vignaud <tvignaud@mandriva.com> 0.6.3-1mdk
- prevent spurious warnings for strange USB interfaces

* Mon May 16 2005 Thierry Vignaud <tvignaud@mandriva.com> 0.6.2-1mdk
- do not try to run dmidecode when not root

* Wed Mar 30 2005 Thierry Vignaud <tvignaud@mandrakesoft.com> 0.6.1-1mdk
- fix SATA detection of latest NVidia controllers

* Mon Mar 14 2005 Pixel <pixel@mandrakesoft.com> 0.6.0-1mdk
- add dmitable parsing and use
- libldetect.so instead of libldetect.a
- libification

* Thu Feb 17 2005 Thierry Vignaud <tvignaud@mandrakesoft.com> 0.5.5-1mdk
- handle a few more special cases (gdth, snd-vx222, 8139too, and agp bridges)
- detect new VIA SATA controllers like kernel does

* Tue Dec  7 2004 Pixel <pixel@mandrakesoft.com> 0.5.4-1mdk
- all PCI_CLASS_BRIDGE_CARDBUS cards are yenta_socket (says kudzu)

* Thu Oct 28 2004 Thierry Vignaud <tvignaud@mandrakesoft.com> 0.5.3-1mdk
- keep existing description string if already reported by USB devices
  when usbtable description is empty (eg: freebox)

* Thu Jun 17 2004 Thierry Vignaud <tvignaud@mandrakesoft.com> 0.5.2-1mdk
- display driver reported by the kernel rather than "unknown"

* Thu Nov 20 2003 Pixel <pixel@mandrakesoft.com> 0.5.1-2mdk
- don't display "Vendor Specific Class" usb class

* Thu Nov 20 2003 Pixel <pixel@mandrakesoft.com> 0.5.1-1mdk
- new usb_class code (breaks compatibility!)

* Tue Aug 19 2003 Thierry Vignaud <tvignaud@mandrakesoft.com> 0.4.9-3mdk
- do full-probe by default

* Thu Jul 31 2003 Pixel <pixel@mandrakesoft.com> 0.4.9-2mdk
- detect ohci1394 & ehci-hcd based on the pci class
  (as done in RedHat's kudzu)

* Tue Apr 22 2003 Pixel <pixel@mandrakesoft.com> 0.4.9-1mdk
- Use read() instead of fread() to read from "/proc/bus/pci/%02x/%02x.%d".
  Thanks a lot to Tom Cox for finding this bug:

	  The proc.c module in the kernel source clearly states that
	  reading more than 64 bytes can cause problems. The pci.c
	  module in the ldetect library uses the buffered fread()
	  function. This function always reads in blocks, so when
	  run as root, the read always tried to read more than the
	  user requested amount.

  This should fix freezes when doing a full probe

* Mon Jan  6 2003 Thierry Vignaud <tvignaud@mandrakesoft.com> 0.4.8-1mdk
- require an usbutils recent enough to have working hub class
- fix hubs detection
- no error message when -p is not used and there is neither pci nor
  usb bus (pixel)

* Tue Oct 15 2002 Thierry Vignaud <tvignaud@mandrakesoft.com> 0.4.7-1mdk
- simplify pci configuration parsing
- build with newer usb ids

* Thu Sep  5 2002 Pixel <pixel@mandrakesoft.com> 0.4.6-6mdk
- fix ugly case for snd-usb-audio which should have made titi think that
something was broken. Really fixing the right way (this fixes automatic
detection of unknown usb controllers)

* Thu Aug 29 2002 Pixel <pixel@mandrakesoft.com> 0.4.6-5mdk
- fix getting the Product name in usb (occurs when there is no entry in usbtable)

* Mon Aug 26 2002 Thierry Vignaud <tvignaud@mandrakesoft.com> 0.4.6-4mdk
- kill last remaining lseek in pci configuration space to prevent
  buggy motherboard from freezing

* Thu Aug 22 2002 Thierry Vignaud <tvignaud@mandrakesoft.com> 0.4.6-3mdk
- prevent freeze on buggy motherboards

* Sat Aug 17 2002 Thierry Vignaud <tvignaud@mandrakesoft.com> 0.4.6-2mdk
- rpmlint fixes

* Sat Aug 17 2002 Thierry Vignaud <tvignaud@mandrakesoft.com> 0.4.6-1mdk
- homogenize pci and usb memory managment
- add hints for documentation
- usb audio devices can use new alsa modules snd-usb-audio (once alsa
  rc3 is in kernel)
- binary is 18% smaller

* Thu Aug  8 2002 Thierry Vignaud <tvignaud@mandrakesoft.com> 0.4.5-1mdk
- fix mis catchinf of "vendor dev" line in table when there's also a
  matching "vendor dev suvvendor subdev" line
  thus we don't depend of the order of the tables

* Tue Jul 30 2002 Pixel <pixel@mandrakesoft.com> 0.4.4-2mdk
- fill in pci_bus and pci_device for USB

* Thu Jul 25 2002 Thierry Vignaud <tvignaud@mandrakesoft.com> 0.4.4-1mdk
- fix "(null) description" bug: don't skip entries where module has
  already be set by pci.c workaround

* Wed Jul 24 2002 Thierry Vignaud <tvignaud@mandrakesoft.com> 0.4.3-1mdk
- lspcidrake.c: enhanced help
- don't die when missing /proc/bus/pci/devices
  (resp. /proc/bus/usb/devices), since on some boxes, this is
  *normal*! free error messages obtained via asprintf (pixel)
 - remove debugging message (pixel)

* Mon Jul 22 2002 Pixel <pixel@mandrakesoft.com> 0.4.2-3mdk
- don't die when missing /proc/bus/pci/devices (resp. /proc/bus/usb/devices), 
  since on some boxes, this is *normal*!
- free error messages obtained via asprintf
- remove debugging message "TOTO"

* Tue Jul 16 2002 Pixel <pixel@mandrakesoft.com> 0.4.2-2mdk
- pciusb.c: teach titi that !(a && !b) is not (!a && !b) but (!a || b)
  (the other solution is to teach him to *test*)
  (oh, remind me to teach him not to re-indent the whole code until he
   doesn't make stupid bugs)

* Tue Jul 16 2002 Thierry Vignaud <tvignaud@mandrakesoft.com> 0.4.2-1mdk
- pci.c:
	o move exception stuff from the fast path into the probe all patch
	o reduce memory usage, especially stack usage
	o add the ability to read pci devices list from a file instead of
	  /proc/bus/pci/devices

- usb.c:
	o add the ability to read usb devices list from a file instead of
	  /proc/usb/devices

- usb.c, pci.c, lspcidrake.c:
	o print error message if unable to open devices list
 	o make a few tests clearer

- lspcidrake.c: 
	o compacificazion
	o fix compilation with gcc-2.95.3 (reported by Ian White)
	o add -p option so that lspcidrake can read pci devices from a
	  file in order to understand what happened to remote testers
	o add -u option so that lspcidrake can read usb devices from a
	  file in order to understand what happened to remote testers
	o describe all options

* Thu Jul  4 2002 Thierry Vignaud <tvignaud@mandrakesoft.com> 0.4.1-1mdk
- let prevent useless copy
- stricter checking compilation and fix warnings
- make some code paths simpler mainly in pciusb.c
- remove useless {pci,usb}_find_modules() wrappers
- when multiples cards're identical, just return the cached description
  and text
- skip comments in {usb,pci}table
- remove uneeded test/free
- move some stuff outside fast paths into exception paths
- this result in:
	- a bug fix (regarding null description with -f) as a side effect
	- saving 9% of the binary & library size

* Wed Jun 26 2002 Gwenole Beauchesne <gbeauchesne@mandrakesoft.com> 0.4.0-2mdk
- sanitize specfile

* Mon Jun 10 2002 Pixel <pixel@mandrakesoft.com> 0.4.0-1mdk
- ensure the header file are C++ compliant (do not use "class" for struct field name)

* Fri Dec 28 2001 Pixel <pixel@mandrakesoft.com> 0.2.5-1mdk
- in probe_type=1, recognize usb controllers (is either usb-uhci or usb-ohci)

* Thu Sep 13 2001 Pixel <pixel@mandrakesoft.com> 0.2.4-2mdk
- use the sub-category for usb probing

* Tue Sep 11 2001 Guillaume Cottenceau <gc@mandrakesoft.com> 0.2.4-1mdk
- add "-v" and "-f" options to lspcidrake for (v)erbose mode and (f)ull
  probe

* Wed Aug 29 2001 Pixel <pixel@mandrakesoft.com> 0.2.3-14mdk
- fix when 2 similar devices are there

* Thu Apr 12 2001 Pixel <pixel@mandrakesoft.com> 0.2.3-13mdk
- close fdno's of the pipe which are unused or dup2'ed

* Wed Apr 11 2001 François Pons <fpons@mandrakesoft.com> 0.2.3-12mdk
- fixed to use LD_LOADER if defined.

* Thu Mar 29 2001 Pixel <pixel@mandrakesoft.com> 0.2.3-11mdk
- fix some memory leak and a few segfaults

* Sat Mar 24 2001 Pixel <pixel@mandrakesoft.com> 0.2.3-10mdk
- nasty C, fclose on popen'ed gets a segfault, in /some/ cases :-(

* Fri Mar 23 2001 Pixel <pixel@mandrakesoft.com> 0.2.3-9mdk
- handle gzip'ed pcitable/usbtable

* Wed Mar 21 2001 Pixel <pixel@mandrakesoft.com> 0.2.3-8mdk
- use subids if they are needed

* Thu Mar 15 2001 François Pons <fpons@mandrakesoft.com> 0.2.3-7mdk
- added pci_bus, pci_device and pci_function for DrakX
- added back Francis into cvs, please Francis do it yourself!

* Tue Mar 15 2001 Francis Galiegue <fg@mandrakesoft.com> 0.2.3-6mdk
- -fPIC in CFLAGS for ia64

* Tue Mar  6 2001 François Pons <fpons@mandrakesoft.com> 0.2.3-5mdk
- added support for SHARE_PATH
- add BuildRequires: usbutils

* Tue Feb 13 2001 Pixel <pixel@mandrakesoft.com> 0.2.3-4mdk
- fix ifree

* Tue Feb  6 2001 Pixel <pixel@mandrakesoft.com> 0.2.3-3mdk
- fix missing fclose's

* Fri Dec 22 2000 Guillaume Cottenceau <gc@mandrakesoft.com> 0.2.3-2mdk
- prettier printing for lspcidrake

* Sat Dec 16 2000 Pixel <pixel@mandrakesoft.com> 0.2.3-1mdk
- now detect usb

* Fri Dec 15 2000 Pixel <pixel@mandrakesoft.com> 0.2.2-1mdk
- fix pciprobe for subids

* Fri Dec 15 2000 Pixel <pixel@mandrakesoft.com> 0.2.1-1mdk
- try with linux/pci_ids.h to generate pciclass.c (kernel 2.4)

* Fri Dec 15 2000 Pixel <pixel@mandrakesoft.com> 0.2.0-2mdk
- add requires ldetect-lst

* Fri Dec 15 2000 Pixel <pixel@mandrakesoft.com> 0.2.0-1mdk
- first release
