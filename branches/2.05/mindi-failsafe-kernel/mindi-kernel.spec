#
# $Id$
#
Summary:	A program that creates emergency boot disks/CDs using your kernel, tools and modules.
Name:		mindi-kernel
Version:	1.05
Release:	1
Copyright:	GPL
Group:		System/Kernel and hardware
Url:		http://www.mondorescue.org
Source:		%{name}-%{version}.tgz
BuildRoot:	/tmp/%{name}-%{version}
Requires:	binutils
Autoreq:	0

%description
Mindi-kernel is a library of kernel modules, a kernel, and other bits and bobs
used by Mindi.

%prep
%setup 
%build
%install
%{__rm} -rf $RPM_BUILD_ROOT
export PREFIX=${RPM_BUILD_ROOT}
./install.sh 

%clean
%{__rm} -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_libdir}/mindi

%changelog
* Wed Dec 14 2005 Bruno Cornec <bcornec@users.berlios.de> 1.05-1
- adapted to the new mindi location (in 1.05-r194 and upper)

* Mon May 19 2003 Hugo Rabson <hugorabson@msn.com> 1.0-1
- initial release
