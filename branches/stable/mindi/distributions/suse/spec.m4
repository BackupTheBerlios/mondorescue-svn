dnl This file contains all specificities for Mindi SuSE spec file build
dnl
dnl SSS is replaced by the source package format
define(`SSS', %{name}-%{version}.tgz)dnl
dnl DDD is replaced by the list of dependencies specific to that distro
define(`DDD', %{addreqb}, syslinux)dnl
dnl GRP is replaced by the RPM group of apps
define(`GRP', Productivity/Archiving/Backup)dnl
