dnl This file contains all specificities for Mindi RedHat spec file build
dnl
dnl SSS is replaced by the source package format
define(`SSS', %{name}-%{version}.tgz)dnl
dnl DDD is replaced by the list of dependencies specific to that distro
define(`DDD', %{addreqb}, which, grep >= 2.5)dnl
dnl GRP is replaced by the RPM group of apps
define(`GRP', Archiving/Archiving)dnl
