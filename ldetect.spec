# !! DON'T MODIFY HERE, MODIFY IN THE CVS !!
%define name ldetect
%define version 0.2.3
%define release 11mdk

Name: %{name}
Version: %{version}
Release: %{release}
Summary: Light hardware detection library
Source: %{name}.tar.bz2
Group: System/Libraries
BuildRoot: %{_tmppath}/%{name}-buildroot
BuildRequires: usbutils
Requires: ldetect-lst
Copyright: GPL
Prefix: %{_prefix}

%package devel
Summary: Development package for ldetect
Group: Development/C

%description
The hardware device lists provided by this package are used as lookup 
table to get hardware autodetection

%description devel
see %{name}

%prep
%setup -n %{name}

%build
%ifnarch ia64
%make
%else
%make CFLAGS="$CFLAGS -fPIC"
%endif

%install
rm -rf $RPM_BUILD_ROOT
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_bindir}/*

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_libdir}/*

%changelog
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
