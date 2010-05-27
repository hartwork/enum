SUBDIRS = src test

man1_MANS = \
	man/enum.1

EXTRA_DIST = \
	man/enum.1.txt


PACKAGE_VERSION = `fgrep AC_INIT configure.ac | sed 's|^.\+, \[\(.\+\)\],.\+$$|\1|'`
SNAPSHOT_VERSION = `git describe --tags | sed 's|-\([0-9]\+\)-g[0-9a-f]\+$$|.\1|'`

man/enum.1: man/enum.1.txt Makefile.am
	a2x --attribute="enumversion=$(PACKAGE_VERSION)" --format=manpage "$<"

snapshot:
	git diff --quiet || { echo 'STOP, you have uncommitted changes in the working directory' ; false ; }
	git diff --cached --quiet || { echo 'STOP, you have uncommitted changes in the index' ; false ; }
	sed 's|^\(AC_INIT(.\+, \[\)[^,]\+\(\], .\+\)$$|\1'"$(SNAPSHOT_VERSION)"'\2|' -i configure.ac
	autoreconf -i -f
	$(MAKE) dist
	git checkout HEAD configure.ac

dist-hook: man/enum.1
