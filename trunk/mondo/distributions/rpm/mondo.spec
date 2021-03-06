#
# $Id$
#

%define mrel	RRR
%define tag		TTT
# if mandriva official build (rpm --with is_official)
%{?is_official:%define rel %{mkrel} %{mrel}}%{!?is_official:%define rel %{mrel}}
%define addreq	mindi >= 1.0.7, bzip2 >= 0.9, afio, mkisofs, binutils, newt >= 0.50, DDD

Summary:	A program which a Linux user can utilize to create a rescue/restore CD/tape
Summary(fr):	Un programme pour les utilisateurs de Linux pour cr�er un CD/tape de sauvegarde/restauration
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
BuildRequires:	newt-devel >= 0.50, gcc
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
Mondo est une solution GPL de sauvegarde en cas de d�sastre pour 
cr�er des m�dias (CD, DVD, bande, images r�seau) qui peuvent �tre 
utilis�s pour red�ployer le syst�me endomang�, aussi bien que des 
syst�mes similaires, ou moins similaires.

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
%setup -q -n %name-%{version}-%{tag}

%build
%configure --program-prefix=%{?_program_prefix}
%{__make} VERSION=%{version}

%install
%{__rm} -rf $RPM_BUILD_ROOT
%makeinstall
%if %is_suse
%{__rm} -rf $RPM_BUILD_ROOT/%{_datadir}/doc/%name-%{version}
%endif

%clean
%{__rm} -rf $RPM_BUILD_ROOT

%files
%doc ChangeLog svn.log
%doc INSTALL COPYING README TODO AUTHORS NEWS
%doc docs/en/mondorescue-howto.html docs/en/mondorescue-howto.pdf

%{_datadir}/locale/*/LC_MESSAGES/mondo.mo
%{_sbindir}/*
%{_datadir}/%{name}
%{_mandir}/man8/*

%changelog
