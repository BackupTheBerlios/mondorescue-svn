#
# $Id$
#

%define mrel	RRR
%define tag		TTT
# if mandriva official build (rpm --with is_official)
%{?is_official:%define rel %{mkrel} %{mrel}}%{!?is_official:%define rel %{mrel}}
%define addreq	mindi >= 1.0.7, bzip2 >= 0.9, afio, mkisofs, binutils, newt >= 0.50, slang >= 1.4.1 DDD

Summary:	A program which a Linux user can utilize to create a rescue/restore CD/tape
Summary(fr):	Un programme pour les utilisateurs de Linux pour créer un CD/tape de sauvegarde/restauration
Summary(it):	Un programma per utenti Linux per creare un CD/tape di rescue
Summary(sp):	Un programa para los usuarios de Linux por crear una CD/cinta de restoracion/rescate

Name:		mondo
Version:	VVV
Release:	%{mrel}
License:	GPL
Group:		GRP
Url:		http://www.mondorescue.org
Source:		SSS
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(id -u -n)
BuildRequires:	newt-devel >= 0.50, slang-devel >= 1.4.1, gcc
OBS
%ifarch ia64
Requires:	%{addreq}, elilo, parted
%else
Requires:	%{addreq}, syslinux >= 1.52
%endif

%description
Mondo is a GPL disaster recovery solution to create backup media 
(CD, DVD, tape, network images) that can be used to redeploy the 
damaged system, as well as deploy similar or less similar systems.

%description -l fr
Objectif
""""""""
Mondo est une solution GPL de sauvegarde en cas de désastre pour 
créer des médias (CD, DVD, bande, images réseau) qui peuvent être 
utilisés pour redéployer le système endomangé, aussi bien que des 
systèmes similaires, ou moins similaires.

%description -l it
Scopo
"""""
Mondo e' un programma che permette a qualsiasi utente Linux 
di creare un cd di rescue/restore (o piu' cd qualora l'installazione 
dovesse occupare piu' di 2Gb circa). Funziona con gli azionamenti di
nastro, ed il NFS, anche.

%description -l sp
Objectivo
"""""""""
Mondo es un programa que permite cualquier usuario de Linux a crear una CD
de restoracion/rescate (o CDs, si su instalacion es >2GO aprox.).  Funciona 
con cintas y NFS, tambien.

%prep
%setup -q -n %name-%{version}

%build
%configure --program-prefix=%{?_program_prefix}
%{__make} VERSION=%{version}

%install
%{__rm} -rf $RPM_BUILD_ROOT
%makeinstall

%clean
%{__rm} -rf $RPM_BUILD_ROOT

%files
%defattr(644,root,root,755)
%doc ChangeLog svn.log
%doc INSTALL COPYING README TODO AUTHORS NEWS
%doc docs/en/mondorescue-howto.html docs/en/mondorescue-howto.pdf

%attr(755,root,root) %{_sbindir}/*
%attr(755,root,root) %{_datadir}/%{name}/restore-scripts/%{name}/*
%attr(755,root,root) %{_datadir}/%{name}/autorun
%attr(755,root,root) %{_datadir}/%{name}/post-nuke.sample/usr/bin/post-nuke
%{_datadir}/%{name}/*
%{_mandir}/man8/*

%changelog
