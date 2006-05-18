#
# $Id$
#

# if mandriva official build (rpm --with is_official)
%{?is_official:%define rel %{mkrel} %{mrel}}%{!?is_official:%define rel %{mrel}}
%define rel		RRR
%define tag		TTT

Summary:	Documentation for Mondo Rescue
Summary(fr):	Documentation pour Mondo Rescue

Name:		mondo-doc
Version:	VVV
Release:	%{rel}
License:	GPL
Group:		GRP
Url:		http://www.mondorescue.org
Source:		SSS
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(id -u -n)
BuildRequires:	docbook-utils
BuildArch: noarch

%description
Documentation for Mondo Rescue

%description -l fr
Documentation pour Mondo Rescue

%prep
%setup -q -n %name-%{version}-%{tag}

%build
%{__make} -f Makefile.man VERSION=%{version}
%{__make} -f Makefile.howto VERSION=%{version}

%install
%{__rm} -rf $RPM_BUILD_ROOT
%{__make} -f Makefile.man install INSTALLDIR=$RPM_BUILDROOT/$RPM_DOC_DIR/%name-%{version}
%{__make} -f Makefile.howto install INSTALLDIR=$RPM_BUILDROOT/$RPM_DOC_DIR/%name-%{version}

%clean
%{__rm} -rf $RPM_BUILD_ROOT

%files
%defattr(644,root,root,755)
%doc svn.log
%doc 

%changelog
