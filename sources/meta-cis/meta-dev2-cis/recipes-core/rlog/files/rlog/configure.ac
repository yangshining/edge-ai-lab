AC_PREREQ([2.0])
AC_INIT([rlog],[1.0],[@])
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects -Werror])
AM_PROG_AR
AC_PROG_CC
AC_PROG_LIBTOOL
LT_INIT

AM_CFLAGS="$AM_CFLAGS -Wall -g -std=gnu99 -Werror -pedantic -Wmissing-declarations -Wextra"
AC_SUBST([AM_CFLAGS])


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
