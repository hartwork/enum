######## library
lib_LTLIBRARIES = libenum.la

libenum_la_SOURCES = generator.h \
		     generator.c \
		     utils.h \
		     utils.c \
		     assertion.h

include_HEADERS = enum.h

pkginclude_HEADERS = generator.h \
		     utils.h

libenum_la_LDFLAGS = -version-info @soversion@ \
	-lm

######## binary
bin_PROGRAMS = enum

enum_LDADD = -lm

enum_SOURCES = \
	assertion.h \
	main.c \
	info.c \
	info.h \
	utils.c \
	utils.h \
	parsing.c \
	parsing.h \
	printing.c \
	printing.h \
	generator.c \
	generator.h

if ! USE_SYSTEM_GETOPT
enum_CPPFLAGS = -I../thirdparty/glibc-2.11.1/posix
enum_SOURCES += \
	../thirdparty/glibc-2.11.1/posix/getopt.h \
	../thirdparty/glibc-2.11.1/posix/getopt.c
endif
