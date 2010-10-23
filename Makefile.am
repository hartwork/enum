SUBDIRS = src test

man1_MANS = \
	man/enum.1

EXTRA_DIST = \
	man/asciidoc.conf \
	man/enum.1 \
	man/enum.1.txt


PACKAGE_VERSION = `fgrep AC_INIT "$(abs_top_srcdir)"/configure.ac | sed 's|^.\+, \[\(.\+\)\],.\+$$|\1|'`
SNAPSHOT_VERSION = `git describe --tags | sed -e 's|^\([0-9]\+\.[0-9]\+\)-|\1.0-|' -e 's|-\([0-9]\+\)-g[0-9a-f]\+$$|.\1|'`


if ENABLE_DOC

$(builddir)/man/enum.1: man/enum.1.txt man/asciidoc.conf configure.ac Makefile.am
	mkdir -p "$(builddir)"/man
	a2x --conf-file=man/asciidoc.conf --attribute="enumversion=$(PACKAGE_VERSION)" \
		--format=manpage -D "$(builddir)"/man "$<"

clean-local:
	rm -f "$(builddir)"/man/enum.1

else

$(builddir)/man/enum.1:

endif


snapshot:
	git diff --quiet || { echo 'STOP, you have uncommitted changes in the working directory' ; false ; }
	git diff --cached --quiet || { echo 'STOP, you have uncommitted changes in the index' ; false ; }
	sed 's|^\(AC_INIT(.\+, \[\)[^,]\+\(\], .\+\)$$|\1'"$(SNAPSHOT_VERSION)"'\2|' -i configure.ac
	autoreconf -i -f
	$(MAKE) distcheck
	git checkout HEAD configure.ac

dist-hook: man/enum.1
