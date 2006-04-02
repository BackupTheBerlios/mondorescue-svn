#
# $Id$
#
%define name	mindi
%define version	VVV
%define mrel	RRR
%define src		SSS
%define grp		GRP
%define addreqb	bzip2 >= 0.9, mkisofs, ncurses, binutils, gawk, dosfstools
%define addreq	DDD
%define rel		%{mrel}

Summary:	Mindi creates emergency boot disks/CDs using your kernel, tools and modules
Name:		%name
Version:	%version
Release:	%rel
License:	GPL
Group:		%{grp}
Url:		http://mondorescue.berlios.de
Source:		%{src}
BuildRoot:	%{_tmppath}/%{name}-%{version}
Requires:	%{addreq}
Epoch:		%(echo EEE | cut -d- -f1 | sed "s~M~~")
# Not on all systems
#Conflicts:	bonnie++

%description
Mindi takes your kernel, modules, tools and libraries, and puts them on N
bootable disks (or 1 bootable CD image). You may then boot from the disks/CD
and do system maintenance - e.g. format partitions, backup/restore data,
verify packages, etc.

%prep
%{__rm}  -rf $RPM_BUILD_ROOT
%setup -n %name-%{version}

%build
%ifarch ia64
%{__make} -f Makefile.parted2fdisk clean
%{__make} -f Makefile.parted2fdisk
%endif

%install
export DONT_RELINK=1

%{__rm} -rf $RPM_BUILD_ROOT
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
%config(noreplace) %{_sysconfdir}/mindi/deplist.txt 
%config(noreplace) %{_sysconfdir}/mindi/deplist.d/* 
%config(noreplace) %{_sysconfdir}/mindi/mindi.conf
%doc ChangeLog INSTALL COPYING README TODO README.ia64 README.pxe README.busybox svn.log
%{_mandir}/man8/*
%{_libdir}/mindi
%attr(755,root,root) %{_sysconfdir}/mindi/mindi.conf
%attr(755,root,root) %{_sbindir}/*
%attr(755,root,root) %{_libdir}/mindi/aux-tools/sbin/*
%attr(755,root,root) %{_libdir}/mindi/rootfs/bin/*
%attr(755,root,root) %{_libdir}/mindi/rootfs/sbin/*

%changelog
