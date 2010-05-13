SUBDIRS = src test

EXTRA_DIST = \
	man/enum.1.ronn \
	man/enum.1


PACKAGE_VERSION = `fgrep AC_INIT configure.ac | sed 's|^.\+, \[\(.\+\)\],.\+$$|\1|'`

man/enum.1: man/enum.1.ronn Makefile.am
	ronn --manual='seq- and jot-like enumerator' \
		--manual="enum $(PACKAGE_VERSION)" \
		--organization="enum $(PACKAGE_VERSION)" \
		--date=`date +%Y-%m-%d` \
		< "$<" \
		> "$@"

dist-hook: man/enum.1
