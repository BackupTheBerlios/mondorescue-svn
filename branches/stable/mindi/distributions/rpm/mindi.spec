#
# $Id$
#
%define mrel	RRR
%define tag		TTT

Summary:	Mindi creates emergency boot disks/CDs using your kernel, tools and modules
Name:		mindi
Version:	VVV
Release:	%mrel
License:	GPL
Group:		GRP
Url:		http://www.mondorescue.org
Source:		SSS
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(id -u -n)
Requires:	bzip2 >= 0.9, mkisofs, ncurses, binutils, gawk, dosfstools DDD
# Not on all systems
#Conflicts:	bonnie++

%description
Mindi takes your kernel, modules, tools and libraries, and puts them on N
bootable disks (or 1 bootable CD image). You may then boot from the disks/CD
and do system maintenance - e.g. format partitions, backup/restore data,
verify packages, etc.

%prep
%setup -n %name-%{version}-%{tag}

%build
%ifarch ia64
%{__make} -f Makefile.parted2fdisk clean
%{__make} -f Makefile.parted2fdisk
%endif

%install
%{__rm}  -rf $RPM_BUILD_ROOT
export DONT_RELINK=1

export PREFIX=${RPM_BUILD_ROOT}%{_exec_prefix}
export CONFDIR=${RPM_BUILD_ROOT}%{_sysconfdir}
export RPMBUILDMINDI="true"

./install.sh

%clean
%{__rm} -rf $RPM_BUILD_ROOT

%post
if [ -f /usr/local/sbin/mindi ]; then
	echo "WARNING: /usr/local/sbin/mindi exists. You should probably remove your manual mindi installation !"
fi

%files
%defattr(644,root,root,755)
%config(noreplace) %{_sysconfdir}/mindi
%config(noreplace) %{_sysconfdir}/mindi/deplist.txt 
%doc ChangeLog INSTALL COPYING README TODO README.ia64 README.pxe README.busybox svn.log
%{_mandir}/man8/*
%{_libdir}/mindi
%attr(755,root,root) %{_sbindir}/*
%attr(755,root,root) %{_libdir}/mindi/aux-tools/sbin/*
%attr(755,root,root) %{_libdir}/mindi/rootfs/bin/*
%attr(755,root,root) %{_libdir}/mindi/rootfs/sbin/*

%changelog
