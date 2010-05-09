#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([enum], [0.1], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/main.c])
# AC_CONFIG_HEADERS([config.h])

AC_ARG_ENABLE([pedantic],
  AS_HELP_STRING([--enable-pedantic], [be pedantic about warnings]), [
    CFLAGS="${CFLAGS} -Wall -Wextra -Wwrite-strings -std=c89 -pedantic"
], [])


# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	test/Makefile
])
AC_OUTPUT
