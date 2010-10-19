#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])
AC_INIT([enum], [0.4], [gitenum-members@fedoraproject.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/main.c])
# AC_CONFIG_HEADERS([config.h])

AC_ARG_ENABLE([pedantic],
  AS_HELP_STRING([--enable-pedantic], [be pedantic about warnings]), [
    CFLAGS="${CFLAGS} -Wall -Wextra -Wwrite-strings -Wno-variadic-macros -std=c89 -pedantic"
], [])


# Checks for programs.
AC_PROG_CC
AC_PROG_SED  # for check against asciidoc


AC_ARG_ENABLE(doc-rebuild, [
===========================================================================

Extra options:
AS_HELP_STRING([--disable-doc-rebuild], [do not re-build shipped documentation])

===========================================================================
], [
	ENABLE_DOC=${enableval}
],[
	ENABLE_DOC=yes
])


# Check for asciidoc
if test ${ENABLE_DOC} = yes ; then
	ASCIIDOC_ERROR='

!!! Please install asciidoc 8.4.5 or later.
!!! Alternatively, you could run ./configure with option --disable-doc-rebuild.'  # see bug #44

	AC_MSG_CHECKING([for presence of asciidoc])
		if asciidoc --version >/dev/null 2>/dev/null ; then
			AC_MSG_RESULT([found])
		else
			AC_MSG_RESULT([missing])
			AC_MSG_ERROR([${ASCIIDOC_ERROR}])
		fi

	AC_MSG_CHECKING([version of asciidoc])
		ASCIIDOC_VERSION=`asciidoc --version | sed 's/^asciidoc \([[]0-9[]]\+\.[[]0-9[]]\+.*\)/\1/'`
		ASCIIDOC_MAJOR=`asciidoc --version | sed 's/^asciidoc \([[]0-9[]]\+\)\.[[]0-9[]]\+.*/\1/'`
		ASCIIDOC_MINOR=`asciidoc --version | sed 's/^asciidoc [[]0-9[]]\+\.\([[]0-9[]]\+\).*/\1/'`
		ASCIIDOC_RELEASE=`asciidoc --version | sed -e 's/^asciidoc [[]0-9[]]\+\.[[]0-9[]]\+$/\0.0/' -e 's/^asciidoc [[]0-9[]]\+\.[[]0-9[]]\+\.\([[]0-9[]]\+\)/\1/'`

		AC_MSG_RESULT([${ASCIIDOC_VERSION}])

		if test ${ASCIIDOC_MAJOR} -lt 8 ; then
			AC_MSG_ERROR([${ASCIIDOC_ERROR}])
		elif test ${ASCIIDOC_MAJOR} -eq 8 ; then
			if test ${ASCIIDOC_MINOR} -lt 4 ; then
				AC_MSG_ERROR([${ASCIIDOC_ERROR}])
			elif test ${ASCIIDOC_MINOR} -eq 4 ; then
				if test ${ASCIIDOC_RELEASE} -lt 5 ; then
					AC_MSG_ERROR([${ASCIIDOC_ERROR}])
				fi
			fi
		fi
fi


# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNC([getopt_long], [
	USE_SYSTEM_GETOPT=1
], [
	USE_SYSTEM_GETOPT=0
])

# Define automake conditionals
AM_CONDITIONAL([USE_SYSTEM_GETOPT], [test "${USE_SYSTEM_GETOPT}" = "1"])
AM_CONDITIONAL([ENABLE_DOC], [test "${ENABLE_DOC}" = "yes"])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	test/Makefile
])
AC_OUTPUT
