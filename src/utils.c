/*
 * enum - seq- and jot-like enumerator
 *
 * Copyright (C) 2010, Jan Hauke Rahm <jhr@debian.org>
 * Copyright (C) 2010, Sebastian Pipping <sping@gentoo.org>
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

#include "assertion.h"
#include "utils.h"

#include <stdlib.h>  /* for malloc, strtod */
#include <string.h>  /* for strlen */

/** Simple union of float and int.
 *
 * @since 0.3
 */
typedef union _float_int {
	float float_data;  /**< float */
	long int int_data; /**< int */
} float_int;

/** Custom strdup function.
 *
 * Copies given string to newly allocated memory and adds trailing \\0 to
 * string.
 *
 * @param[in] text
 *
 * @return Pointer to copy of string or NULL if malloc failed
 *
 * @since 0.3
 */
char * enum_strdup(const char * text) {
	return enum_strndup(text, 0);
}

/** Custom strndup function.
 *
 * Copies first n characters of given string to newly allocated memory and adds
 * trailing \\0 to string. If n == 0, copy complete string (strdup).
 *
 * @param[in] text
 *
 * @return Pointer to copy of string or NULL if malloc failed
 *
 * @since 0.5
 */
char * enum_strndup(const char * text, unsigned int n) {
	unsigned int len = n;
	char * dup;

	if (n == 0)
		len = strlen(text);

	dup = (char *)malloc(len + 1);
	if (!dup)
		return NULL;
	strncpy(dup, text, len);
	dup[len] = '\0';
	return dup;
}

/** Custom check whether a float is NAN.
 *
 * Since strtod("NAN", NULL) != strtod("NAN", NULL), this function compares
 * bits of a given float to those of NAN and thus determines whether given
 * float it NAN or not.
 *
 * @param[in] value
 *
 * @return boolean meaning of 1 or 0
 *
 * @since 0.3
 */
int enum_is_nan_float(float value) {
	float_int fi_test;
	float_int fi_nan;

	assert(sizeof(long int) >= sizeof(float));

	fi_test.int_data = 0;
	fi_test.float_data = value;

	fi_nan.int_data = 0;
	fi_nan.float_data = (float)strtod("NAN", NULL);

	return fi_test.int_data == fi_nan.int_data;
}


/** Checks for hexadecimal characters ('0' to '9', 'a' to 'f', 'A' to 'F')
 *
 * @param[in] c Character to analyze
 *
 * @return 1 for a hexadecimal character, 0 else.
 *
 * @since 1.0
 */
static int hex_digit(char c) {
	return (((c >= '0') && (c <= '9'))
		|| ((c >= 'a') && (c <= 'f'))
		|| ((c >= 'A') && (c <= 'F')));
}


/** Extracts hexadecimal value (e.g. 'F' is 15)
 *
 * @param[in] c Character to analyze
 *
 * @return A number from 0 to 15
 *
 * @since 1.0
 */
static int hex_value(char c) {
	if ((c >= '0') && (c <= '9')) {
		return c - '0';
	} else if ((c >= 'a') && (c <= 'f')) {
		return c - 'a';
	} else {
		return c - 'A';
	}
}


/** Checks for octal characters ('0' to '7')
 *
 * @param[in] c Character to analyze
 *
 * @return 1 for an octal character, 0 else.
 *
 * @since 1.0
 */
static int oct_digit(char c) {
	return ((c >= '0') && (c <= '7'));
}


/** Extracts hexadecimal value (e.g. '7' is 7)
 *
 * @param[in] c Character to analyze
 *
 * @return A number from 0 to 7
 *
 * @since 1.0
 */
static int oct_value(char c) {
	return (c == 'o') ? 0 : (c - '0');
}


/** Resolve escape seqences
 *
 * @param[in,out] text String to process
 * @param[in] options Options to apply
 *
 * @return Length of the new string (excluding terminator)
 *
 * @since 1.0
 */
size_t enum_unescape(char * text, unescape_options options) {
	char const * read = text;
	char * write = text;

	char single[255] = { 0, };
	single[(unsigned char)'a'] = '\a';
	single[(unsigned char)'b'] = '\b';
	single[(unsigned char)'f'] = '\f';
	single[(unsigned char)'n'] = '\n';
	single[(unsigned char)'r'] = '\r';
	single[(unsigned char)'t'] = '\t';
	single[(unsigned char)'v'] = '\v';
	single[(unsigned char)'?'] = '\?';
	single[(unsigned char)'\\'] = '\\';

	while (read[0]) {
		const char replacement = single[(unsigned char)read[1]];

		if (read[0] != '\\') {
			write[0] = read[0];
			read++;
			write++;
			continue;
		}
		
		assert(read[0] == '\\');
		
		if (replacement) {
			write[0] = replacement;
			read += 2;
			write += 1;
			continue;
		}

		switch (read[1]) {
		case '\0':
			write[0] = '\\';
			write += 1;
			read += 1;
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case 'o':
			if ((read[2] != '\0') && oct_digit(read[2])) {
				if ((read[3] != '\0') && oct_digit(read[3])) {
					write[0] = 64 * oct_value(read[1]) + 8 * oct_value(read[2]) + oct_value(read[3]);
					read += 4;
				} else {
					write[0] = 8 * oct_value(read[1]) + oct_value(read[2]);
					read += 3;
				}
			} else {
				write[0] = oct_value(read[1]);
				read += 2;
			}

			if ((write[0] == '%') && ENUM_CHECK_FLAG(options, GUARD_PERCENT)) {
				write[1] = '%';
				write += 2;
			} else {
				write += 1;
			}
			break;

		case 'x':
			if ((read[2] != '\0') && hex_digit(read[2])) {
				if ((read[3] != '\0') && hex_digit(read[3])) {
					write[0] = 16 * hex_value(read[2]) + hex_value(read[3]);
					read += 4;
				} else {
					write[0] = hex_value(read[2]);
					read += 3;
				}
			} else {
				/* "\x" in C string gives GCC compile error. We make "x" from it. */
				write[0] = 'x';
				read += 2;
			}

			if ((write[0] == '%') && ENUM_CHECK_FLAG(options, GUARD_PERCENT)) {
				write[1] = '%';
				write += 2;
			} else {
				write += 1;
			}
			break;

		default:
			/* Drop backslash */
			write[0] = read[1];
			read += 2;
			write += 1;
		}
	}

	write[0] = '\0';

	return (write - text);
}
