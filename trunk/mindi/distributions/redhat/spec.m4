dnl This file contains all specificities for Mindi RedHat spec file build
dnl
dnl SSS is replaced by the source package format
define(`SSS', `ftp://ftp.mondorescue.org/src/%{name}-%{version}-%{tag}.tar.gz')dnl
dnl DDD is replaced by the list of dependencies specific to that distro
define(`DDD', `, which, grep >= 2.5')dnl
dnl GRP is replaced by the RPM group of apps
define(`GRP', `Applications/Archiving')dnl
dnl OBS is replaced with what is being obsoleted
define(`OBS', )dnl
