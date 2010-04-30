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

#include "parsing.h"
#include "generator.h"

#include <stdlib.h>
#include <string.h>

typedef enum _token_type {
  TOKEN_FLOAT,
  TOKEN_MULTIPLIER,
  TOKEN_DOTDOT,

  TOKEN_ERROR_EMPTY,
  TOKEN_ERROR_BAD_COUNT,
  TOKEN_ERROR_BAD_FLOAT,
  TOKEN_ERROR_PARSE
} token_type;

typedef union _setter_value {
	unsigned int uint_data;
	float float_data;
} setter_value;

typedef void (*setter_function_pointer)(arguments *, setter_value);

typedef struct _token_details {
	token_type type;
	setter_function_pointer setter;
} token_details;

void set_args_left(arguments * args, setter_value value) {
	args->flags |= FLAG_LEFT_SET;
	args->left = value.float_data;
}

void set_args_step(arguments * args, setter_value value) {
	args->flags |= FLAG_STEP_SET;
	args->step_num = value.float_data;
	args->step_denom = 1;
}

void set_args_right(arguments * args, setter_value value) {
	args->flags |= FLAG_RIGHT_SET;
	args->right = value.float_data;
}

void set_args_count(arguments * args, setter_value value) {
	args->flags |= FLAG_COUNT_SET;
	args->count = value.uint_data;
}

int ends_with_x(const char *str) {
	const int len = strlen(str);
	if (str[len - 1] == 'x')
		return 1;
	return 0;
}

int is_nan_or_inf(float f) {
	const float NAN = strtod("NAN", NULL);
	const float INF = strtod("INF", NULL);
	return ((f == NAN) || (f == INF)) ? 1 : 0;
}

token_type identify_token(char *arg, setter_value *value) {
	char *end;
	if (*arg == '\0')
		return TOKEN_ERROR_EMPTY;

	if (strcmp(arg, "..") == 0) {
		return TOKEN_DOTDOT;
	}

	if (ends_with_x(arg)) {
		const long int i = strtol(arg, &end, 10);
		if ((*end == 'x') && (end - arg == (int)strlen(arg) - 1)) {
			if (i < 1) {
				return TOKEN_ERROR_BAD_COUNT;
			}
			value->uint_data = i;
			return TOKEN_MULTIPLIER;
		}
	} else {
		const float f = strtod(arg, &end);
		if (end - arg == (int)strlen(arg)) {
			if (is_nan_or_inf(f)) {
				return TOKEN_ERROR_BAD_FLOAT;
			}
			value->float_data =f;
			return TOKEN_FLOAT;
		}
	}

	return TOKEN_ERROR_PARSE;
}

int parse_args(int arg_count, char **args, arguments *dest) {
	token_details const use_case_0[] = {{TOKEN_FLOAT, set_args_right}};
	token_details const use_case_1[] = {{TOKEN_FLOAT, set_args_left}, {TOKEN_DOTDOT, NULL}, {TOKEN_MULTIPLIER, set_args_count}, {TOKEN_FLOAT, set_args_step}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_args_right}};
	token_details const use_case_2[] = {{TOKEN_DOTDOT, NULL}, {TOKEN_MULTIPLIER, set_args_count}, {TOKEN_FLOAT, set_args_step}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_args_right}};
	token_details const use_case_3[] = {{TOKEN_FLOAT, set_args_left}, {TOKEN_DOTDOT, NULL}, {TOKEN_MULTIPLIER, set_args_count}, {TOKEN_FLOAT, set_args_step}, {TOKEN_DOTDOT, NULL}};
	token_details const use_case_4[] = {{TOKEN_FLOAT, set_args_left}, {TOKEN_DOTDOT, NULL}, {TOKEN_MULTIPLIER, set_args_count}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_args_right}};
	token_details const use_case_5[] = {{TOKEN_FLOAT, set_args_left}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_args_step}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_args_right}};
	token_details const use_case_6[] = {{TOKEN_DOTDOT, NULL}, {TOKEN_MULTIPLIER, set_args_count}, {TOKEN_FLOAT, set_args_step}, {TOKEN_DOTDOT, NULL}};
	token_details const use_case_7[] = {{TOKEN_DOTDOT, NULL}, {TOKEN_MULTIPLIER, set_args_count}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_args_right}};
	token_details const use_case_8[] = {{TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_args_step}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_args_right}};
	token_details const use_case_9[] = {{TOKEN_FLOAT, set_args_left}, {TOKEN_DOTDOT, NULL}, {TOKEN_MULTIPLIER, set_args_count}, {TOKEN_DOTDOT, NULL}};
	token_details const use_case_10[] = {{TOKEN_FLOAT, set_args_left}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_args_step}, {TOKEN_DOTDOT, NULL}};
	token_details const use_case_11[] = {{TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_args_step}, {TOKEN_DOTDOT, NULL}};
	token_details const use_case_12[] = {{TOKEN_DOTDOT, NULL}, {TOKEN_MULTIPLIER, set_args_count}, {TOKEN_DOTDOT, NULL}};
	token_details const use_case_13[] = {{TOKEN_FLOAT, set_args_left}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_args_right}};
	token_details const use_case_14[] = {{TOKEN_FLOAT, set_args_left}, {TOKEN_FLOAT, set_args_step}, {TOKEN_FLOAT, set_args_right}};
	token_details const use_case_15[] = {{TOKEN_FLOAT, set_args_left}, {TOKEN_FLOAT, set_args_right}};
	token_details const use_case_16[] = {{TOKEN_FLOAT, set_args_left}, {TOKEN_DOTDOT, NULL}};
	token_details const use_case_17[] = {{TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_args_right}};

	token_details const * table[18];
	table[0] = use_case_0;
	table[1] = use_case_1;
	table[2] = use_case_2;
	table[3] = use_case_3;
	table[4] = use_case_4;
	table[5] = use_case_5;
	table[6] = use_case_6;
	table[7] = use_case_7;
	table[8] = use_case_8;
	table[9] = use_case_9;
	table[10] = use_case_10;
	table[11] = use_case_11;
	table[12] = use_case_12;
	table[13] = use_case_13;
	table[14] = use_case_14;
	table[15] = use_case_15;
	table[16] = use_case_16;
	table[17] = use_case_17;

	return 0;
}
