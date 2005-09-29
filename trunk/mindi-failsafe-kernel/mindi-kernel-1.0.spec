#
# $Id$
#
%define _prefix /usr/local
Summary:	A program that creates emergency boot disks/CDs using your kernel, tools and modules.
Name:		mindi-kernel
Version:	1.0
Release:	1
Copyright:	GPL
Group:		System/Kernel and hardware
Url:		http://www.mondorescue.org
Source:		%{name}-%{version}.tgz
BuildRoot:	/tmp/%{name}-%{version}
Requires:	binutils
Prefix:		/usr/local
Autoreq:	0

%description
Mindi-kernel is a library of kernel modules, a kernel, and other bits and bobs
used by Mindi.

%prep
%setup 
%build
%install
%{__rm} -rf $RPM_BUILD_ROOT
MINDIDIR=$RPM_BUILD_ROOT%{_datadir}/mindi
%{__mkdir_p} $MINDIDIR $RPM_BUILD_ROOT%{_bindir} $RPM_BUILD_ROOT%{_sysconfdir}/mindi
%{__cp} -a * $MINDIDIR
#

%clean
%{__rm} -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_datadir}/mindi

%changelog
* Mon May 19 2003 Hugo Rabson <hugorabson@msn.com> 1.0-1
- initial release
