bin_PROGRAMS = enum

enum_LDFLAGS = -lm

enum_SOURCES = \
	main.c \
	parsing.c \
	parsing.h \
	generator.c \
	generator.h

if ! USE_SYSTEM_GETOPT
enum_CPPFLAGS = -Iglibc-2.11.1/posix
enum_SOURCES += \
	glibc-2.11.1/posix/getopt.h \
	glibc-2.11.1/posix/getopt.c
endif
