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

#include "printing.h"
#include "assertion.h"
#include "utils.h"

#include <stdlib.h>  /* for malloc */
#include <stdio.h>  /* for FILE*, fopen, fclose */
#include <string.h>  /* for strncpy */


#define CASE_INT_LIKE_SPECIFIER  \
	case 'd': \
	case 'i': \
	case 'o': \
	case 'u': \
	case 'x': \
	case 'X': \
	case 'c':

#define CASE_FLOAT_LIKE_SPECIFIER  \
	case 'e': \
	case 'f': \
	case 'g': \
	case 'E': \
	case 'F': \
	case 'G':

#define CASE_FLAG_CHARACTER  \
	case '#': \
	case '0': \
	case ' ': \
	case '\'': \
	case '+': \
	case '-':

#define CASE_NON_ZERO_DIGIT  \
	case '1': \
	case '2': \
	case '3': \
	case '4': \
	case '5': \
	case '6': \
	case '7': \
	case '8': \
	case '9':


typedef enum _format_parse_state {
	STATE_AT_FIRST,
	STATE_INSIDE_FLAGS,
	STATE_INSIDE_PRE_DOT,
	STATE_AT_DOT,
	STATE_INSIDE_POST_DOT,
	STATE_AT_LAST,
	STATE_OUTSIDE
} format_parse_state;


/*
 * Helper function called by <multi_printf>.
 * Runs printf on the format from <start> to <after_last> (exclusively)
 * passing a casted instanced <value> as needed.
 * With <pretend> != 0 no printing will be done; instead,
 * the given format will be checked for validity.
 */
static custom_printf_return single_cast_printf(const char *start, const char *after_last, char specifier, float value, int pretend) {
	const int len = after_last - start;
	char * const subformat = malloc(len + 1);
	static const char * const safety_pointer = "Should never be printed";
	int res;
	FILE * file = stdout;

	if (!subformat) {
		return CUSTOM_PRINTF_OUT_OF_MEMORY;
	}

	if (pretend) {
		file = fopen("/dev/null", "w");
		assert(file);
	}

	strncpy(subformat, start, len);
	subformat[len] = '\0';

	switch (specifier) {
	case '\0':
	case '%':
		res = fprintf(file, subformat, safety_pointer, safety_pointer, safety_pointer);
		break;
	CASE_INT_LIKE_SPECIFIER
		res = fprintf(file, subformat, (int)value, safety_pointer, safety_pointer);
		break;
	CASE_FLOAT_LIKE_SPECIFIER
		res = fprintf(file, subformat, value, safety_pointer, safety_pointer);
		break;
	default:
		assert(0);
	}

	if (pretend) {
		fclose(file);
	}

	free(subformat);

	if ((res < 0) || ((res == 0) && (len > 0))) {
		return CUSTOM_PRINTF_INVALID_FORMAT_PRINTF;
	}

	return CUSTOM_PRINTF_SUCCESS;
}


/*
 * Parses <format> for points of interpolation (e.g. "%3i")
 * and calls <single_cast_printf> on substrings containing
 * on point of interpolation at most.
 * With <pretend> != 0 no printing will be done; instead,
 * <format> will be checked for validity.
 */
static custom_printf_return multi_printf_internal(const char * format, float value, int pretend) {
	const char * start = format;
	const char * walker = format;
	char specifier = '\0';
	format_parse_state state = STATE_OUTSIDE;
	custom_printf_return res = CUSTOM_PRINTF_SUCCESS;

	/* Supported patterns:
	 * "%[#0 '+-]?([1-9][0-9]+)?[diouxXc]"
	 * "%[#0 '+-]?([1-9][0-9]+(.[0-9]+)?)?[efgEFG]"
	 * "%%"
	 */
	while (walker[0] != '\0') {
		switch (state) {
		case STATE_OUTSIDE:
		case STATE_AT_LAST:
			if (walker[0] == '%') {
				if ((walker > start) && (specifier != '%')) {
					res = single_cast_printf(start, walker, specifier, value, pretend);
					if (res != CUSTOM_PRINTF_SUCCESS) {
						return res;
					}
					start = walker;
				}
				state = STATE_AT_FIRST;
			} else {
				state = STATE_OUTSIDE;
			}
			break;
		case STATE_AT_FIRST:
			switch (walker[0]) {
			CASE_FLAG_CHARACTER
				state = STATE_INSIDE_FLAGS;
				break;
			CASE_NON_ZERO_DIGIT
				state = STATE_INSIDE_PRE_DOT;
				break;
			case '.':
				state = STATE_AT_DOT;
				break;
			CASE_INT_LIKE_SPECIFIER
			CASE_FLOAT_LIKE_SPECIFIER
				specifier = walker[0];
				state = STATE_AT_LAST;
				break;
			case '%':
				specifier = walker[0];
				state = STATE_AT_LAST;
				break;
			default:
				return CUSTOM_PRINTF_INVALID_FORMAT_ENUM;
			}
			break;
		case STATE_INSIDE_FLAGS:
			switch (walker[0]) {
			CASE_FLAG_CHARACTER
				break;
			CASE_NON_ZERO_DIGIT
				state = STATE_INSIDE_PRE_DOT;
				break;
			case '.':
				state = STATE_AT_DOT;
				break;
			CASE_INT_LIKE_SPECIFIER
			CASE_FLOAT_LIKE_SPECIFIER
				specifier = walker[0];
				state = STATE_AT_LAST;
				break;
			default:
				return CUSTOM_PRINTF_INVALID_FORMAT_ENUM;
			}
			break;
		case STATE_INSIDE_PRE_DOT:
			switch (walker[0]) {
			CASE_NON_ZERO_DIGIT
				break;
			case '.':
				state = STATE_AT_DOT;
				break;
			CASE_INT_LIKE_SPECIFIER
			CASE_FLOAT_LIKE_SPECIFIER
				specifier = walker[0];
				state = STATE_AT_LAST;
				break;
			default:
				return CUSTOM_PRINTF_INVALID_FORMAT_ENUM;
			}
			break;
		case STATE_AT_DOT:
			switch (walker[0]) {
			case '0':
			CASE_NON_ZERO_DIGIT
				state = STATE_INSIDE_POST_DOT;
				break;
			CASE_FLOAT_LIKE_SPECIFIER
				specifier = walker[0];
				state = STATE_AT_LAST;
				break;
			default:
				return CUSTOM_PRINTF_INVALID_FORMAT_ENUM;
			}
			break;
		case STATE_INSIDE_POST_DOT:
			switch (walker[0]) {
			case '0':
			CASE_NON_ZERO_DIGIT
				break;
			CASE_FLOAT_LIKE_SPECIFIER
				specifier = walker[0];
				state = STATE_AT_LAST;
				break;
			default:
				return CUSTOM_PRINTF_INVALID_FORMAT_ENUM;
			}
			break;
		default:
			assert(0);
		}
		walker++;
	}

	if (walker > start) {
		res = single_cast_printf(start, walker, specifier, value, pretend);
	}

	return res;
}


/*
 * Parses <format> for points of interpolation (e.g. "%3i")
 * and calls <single_cast_printf> on substrings containing
 * on point of interpolation at most.
 */
custom_printf_return multi_printf(const char * format, float value) {
	return multi_printf_internal(format, value, 0);
}


/*
 * Checks <format> for validity.
 */
custom_printf_return is_valid_format(const char * format) {
	return multi_printf_internal(format, 1.23456f, 1);
}
