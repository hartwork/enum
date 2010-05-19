SUBDIRS = src test

man1_MANS = \
	man/enum.1

EXTRA_DIST = \
	man/enum.1.ronn


PACKAGE_VERSION = `fgrep AC_INIT configure.ac | sed 's|^.\+, \[\(.\+\)\],.\+$$|\1|'`

man/enum.1: man/enum.1.ronn Makefile.am
	ronn --manual='seq- and jot-like enumerator' \
		--manual="enum $(PACKAGE_VERSION)" \
		--organization="enum $(PACKAGE_VERSION)" \
		--date=`date +%Y-%m-%d` \
		< "$<" \
		> "$@"

snapshot:
	git diff --quiet || { echo 'STOP, you have uncommitted changes around' ; false ; }
	sed 's|^\(AC_INIT(.\+, \[\)[^,]\+\(\], .\+\)$$|\1'`git describe`'\2|' -i configure.ac
	autoreconf -i -f
	$(MAKE) dist
	git checkout HEAD configure.ac

dist-hook: man/enum.1
