/*
 * enum - seq- and jot-like enumerator
 *
 * Copyright (C) 2010-2012, Jan Hauke Rahm <jhr@debian.org>
 * Copyright (C) 2010-2012, Sebastian Pipping <sping@gentoo.org>
 * All rights reserved.
 *
 * Redistribution  and use in source and binary forms, with or without
 * modification,  are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions   of  source  code  must  retain  the   above
 *       copyright  notice, this list of conditions and the  following
 *       disclaimer.
 *
 *     * Redistributions  in  binary  form must  reproduce  the  above
 *       copyright  notice, this list of conditions and the  following
 *       disclaimer   in  the  documentation  and/or  other  materials
 *       provided with the distribution.
 *
 *     * Neither  the name of the <ORGANIZATION> nor the names of  its
 *       contributors  may  be  used to endorse  or  promote  products
 *       derived  from  this software without specific  prior  written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT  NOT
 * LIMITED  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS
 * FOR  A  PARTICULAR  PURPOSE ARE DISCLAIMED. IN NO EVENT  SHALL  THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL,    SPECIAL,   EXEMPLARY,   OR   CONSEQUENTIAL   DAMAGES
 * (INCLUDING,  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES;  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT  LIABILITY,  OR  TORT (INCLUDING  NEGLIGENCE  OR  OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "info.h"
#include <stdlib.h>
#include <stdarg.h>

/** Return version of program.
 *
 * @since 0.2
 */
void dump_version() {
	puts(PACKAGE_VERSION);
}

/** Print usage of program.
 *
 * @param[in] file File stream to dump usage into
 *
 * @since 0.2
 */
void dump_usage(FILE * file) {
	fprintf(file,
		"Usage: \n"
		"  enum [ OPTIONS ] LEFT \"..\" COUNT\"x\" STEP \"..\" RIGHT\n"
		"\n"
		"  enum [ OPTIONS ] LEFT STEP RIGHT\n"
		"  enum [ OPTIONS ] LEFT RIGHT\n"
		"  enum [ OPTIONS ] RIGHT\n"
		"  ...\n"
		"\n");
	fprintf(file,
		"Options:\n"
		"  -V, --version         show program's version number and exit\n"
		"  -h, --help            show this help message and exit\n"
		"\n"
		"  -f, --format=FORMAT   adjust formatting of values\n"
		"  -s, --separator=TEXT  adjust separator (printed between values)\n"
		"  -t, --terminator=TEXT adjust terminator (printed after last value)\n"
		"  -n, --omit-newline    omit trailing terminator (default: newline)\n"
		"\n");
	fprintf(file,
		"  -c, --characters      print characters, not numbers\n"
		"  -p, --precision=COUNT adjust the number of decimal places printed\n"
		"  -e, --equal-width     equalize width by padding with leading zeroes.\n"
		"  -l, --line            use a space as separator, not a new line.\n"
		"  -x, --exclude=RANGE   exlude RANGE (one out of l, left, r, right) from output\n"
		"  -z, --zero, --null    use null byte as separator, not a new line.\n"
		"\n");
	fprintf(file,
		"  -r, --random          output random numbers, not sequential ones\n"
		"  -i, --seed=NUMBER     specify seed for random number generator\n"
		"\n"
		"  -w, --word=FORMAT     alias for --format\n"
		"  -b, --dumb=TEXT       use verbatim text for values"
		"\n");
}

/** Print proper message to stderr for warnings and errors
 *
 * @param[in] problem_type Type of given problem (see _problem_type)
 * @param[in] str Error string to print (printf format)
 * @param[in] ... variable list of arguments to complement error string
 *
 * @since 1.1
 */
void print_problem(int problem_type, ...) {
	static int usage_dumped = 0;
	va_list list;
	char * str;

	if (problem_type == USER_ERROR && usage_dumped == 0) {
		dump_usage(stderr);
		fprintf(stderr, "\n");
		usage_dumped = 1;
	}

	switch (problem_type) {
	case WARNING:
		fprintf(stderr, "WARNING: ");
		break;
	case USER_ERROR:
	case ERROR:
		fprintf(stderr, "ERROR: ");
		break;
	case OUTOFMEM_ERROR:
		fprintf(stderr, "ERROR: System too low on memory to continue.\n");
		return;
	}

	va_start(list, problem_type);
	str = va_arg(list, char *);
	vfprintf(stderr, str, list);
	va_end(list);

	fprintf(stderr, "\n");
}
