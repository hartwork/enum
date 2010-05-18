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

git-describe-hook:
	git diff --quiet || { echo 'STOP, you have uncommitted changes around' ; false ; }
	sed 's|^\(AC_INIT(.\+, \[\)[^,]\+\(\], .\+\)$$|\1'`git describe`'\2|' -i configure.ac
	autoreconf -i -f
	git checkout HEAD configure.ac

dist-hook: git-describe-hook man/enum.1
