check_PROGRAMS = enum_test

TESTS = enum_test

enum_test_LDADD = -lm

enum_test_CPPFLAGS = -I$(srcdir)/../src/

enum_test_SOURCES = \
	main.c \
	\
	../src/utils.c

if ENABLE_INTERNAL_LINKING
enum_test_LDFLAGS = -L$(srcdir)/../src/.libs
enum_test_LDADD += -lenum
else
enum_test_SOURCES += ../src/generator.c
endif
