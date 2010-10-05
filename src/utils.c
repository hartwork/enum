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
 * @since 0.3
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
