dnl This file contains all specificities for Mindi Mandriva spec file build
dnl
dnl SSS is replaced by the source package format
define(`SSS', `%{name}-%{version}.tar.bz2')dnl
dnl DDD is replaced by the list of dependencies specific to that distro
define(`DDD', `%{addreqb}')dnl
dnl GRP is replaced by the RPM group of apps
define(`GRP', `Archiving/Backup')dnl
dnl OBS is replaced vy what is being obsoleted
define(`OBS', Obsoletes:	libmondo <= 2.04)dnl
