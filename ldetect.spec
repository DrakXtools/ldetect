%define name ldetect
%define version 0.2.3
%define release 1mdk

Name: %{name}
Version: %{version}
Release: %{release}
Summary: Light hardware detection library
Source: %{name}.tar.bz2
Group: System/Libraries
BuildRoot: %{_tmppath}/%{name}-buildroot
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
%make

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
