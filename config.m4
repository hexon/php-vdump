dnl
dnl $ Id: $
dnl

PHP_ARG_ENABLE(vdump, whether to enable vdump functions,
[  --enable-vdump         Enable vdump support])

if test "$PHP_VDUMP" != "no"; then
dnl   export OLD_CPPFLAGS="$CPPFLAGS"
dnl   export CPPFLAGS="$CPPFLAGS $INCLUDES -DHAVE_VDUMP"

dnl   AC_MSG_CHECKING(PHP version)
dnl   AC_TRY_COMPILE([#include <php_version.h>], [
dnl #if PHP_VERSION_ID < 50000
dnl #error  this extension requires at least PHP version 5.0.0
dnl #endif
dnl ],
dnl [AC_MSG_RESULT(ok)],
dnl [AC_MSG_ERROR([need at least PHP 5.0.0])])
dnl 
dnl   export CPPFLAGS="$OLD_CPPFLAGS"


  PHP_SUBST(VDUMP_SHARED_LIBADD)
  AC_DEFINE(HAVE_VDUMP, 1, [ ])

  PHP_NEW_EXTENSION(vdump, vdump.c , $ext_shared)

fi

