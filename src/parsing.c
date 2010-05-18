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
#include "assertion.h"

#include <stdlib.h>
#include <string.h>

#define IS_TOKEN_ERROR(type)  ((type) >= TOKEN_ERROR)

typedef enum _token_type {
  TOKEN_FLOAT,
  TOKEN_MULTIPLIER,
  TOKEN_DOTDOT,

  /* Only errors from here */
  TOKEN_ERROR,
  TOKEN_ERROR_EMPTY,
  TOKEN_ERROR_BAD_COUNT,
  TOKEN_ERROR_BAD_FLOAT,
  TOKEN_ERROR_PARSE
} token_type;

typedef union _setter_value {
	unsigned int uint_data;
	float float_data;
} setter_value;

typedef int (*setter_function_pointer)(scaffolding *, setter_value);

typedef struct _token_details {
	token_type type;
	setter_function_pointer setter;
} token_details;

typedef struct _use_case {
	token_details const *details;
	unsigned int length;
} use_case;

int set_scaffold_left(scaffolding * scaffold, setter_value value) {
	scaffold->flags |= FLAG_LEFT_SET;
	scaffold->left = value.float_data;
	return 1;
}

int set_scaffold_step(scaffolding * scaffold, setter_value value) {
	if (value.float_data == 0.0f) {
		/* error code handled below */
		return 0;
	}
	scaffold->flags |= FLAG_STEP_SET;
	scaffold->step = value.float_data;
	return 1;
}

int set_scaffold_right(scaffolding * scaffold, setter_value value) {
	scaffold->flags |= FLAG_RIGHT_SET;
	scaffold->right = value.float_data;
	return 1;
}

int set_scaffold_count(scaffolding * scaffold, setter_value value) {
	scaffold->flags |= FLAG_COUNT_SET;
	scaffold->count = value.uint_data;
	return 1;
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

token_type identify_token(const char *arg, setter_value *value) {
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

int parse_args(unsigned int args_len, char **args, scaffolding *dest) {
	unsigned int i;
	unsigned int j;
	unsigned int k;
	unsigned int l;
	unsigned int m;
	use_case const *valid_case = NULL;

	token_details const use_case_0[] = {{TOKEN_FLOAT, set_scaffold_right}};
	token_details const use_case_1[] = {{TOKEN_FLOAT, set_scaffold_left}, {TOKEN_DOTDOT, NULL}, {TOKEN_MULTIPLIER, set_scaffold_count}, {TOKEN_FLOAT, set_scaffold_step}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_scaffold_right}};
	token_details const use_case_2[] = {{TOKEN_DOTDOT, NULL}, {TOKEN_MULTIPLIER, set_scaffold_count}, {TOKEN_FLOAT, set_scaffold_step}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_scaffold_right}};
	token_details const use_case_3[] = {{TOKEN_FLOAT, set_scaffold_left}, {TOKEN_DOTDOT, NULL}, {TOKEN_MULTIPLIER, set_scaffold_count}, {TOKEN_FLOAT, set_scaffold_step}, {TOKEN_DOTDOT, NULL}};
	token_details const use_case_4[] = {{TOKEN_FLOAT, set_scaffold_left}, {TOKEN_DOTDOT, NULL}, {TOKEN_MULTIPLIER, set_scaffold_count}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_scaffold_right}};
	token_details const use_case_5[] = {{TOKEN_FLOAT, set_scaffold_left}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_scaffold_step}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_scaffold_right}};
	token_details const use_case_6[] = {{TOKEN_DOTDOT, NULL}, {TOKEN_MULTIPLIER, set_scaffold_count}, {TOKEN_FLOAT, set_scaffold_step}, {TOKEN_DOTDOT, NULL}};
	token_details const use_case_7[] = {{TOKEN_DOTDOT, NULL}, {TOKEN_MULTIPLIER, set_scaffold_count}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_scaffold_right}};
	token_details const use_case_8[] = {{TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_scaffold_step}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_scaffold_right}};
	token_details const use_case_9[] = {{TOKEN_FLOAT, set_scaffold_left}, {TOKEN_DOTDOT, NULL}, {TOKEN_MULTIPLIER, set_scaffold_count}, {TOKEN_DOTDOT, NULL}};
	token_details const use_case_10[] = {{TOKEN_FLOAT, set_scaffold_left}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_scaffold_step}, {TOKEN_DOTDOT, NULL}};
	token_details const use_case_11[] = {{TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_scaffold_step}, {TOKEN_DOTDOT, NULL}};
	token_details const use_case_12[] = {{TOKEN_DOTDOT, NULL}, {TOKEN_MULTIPLIER, set_scaffold_count}, {TOKEN_DOTDOT, NULL}};
	token_details const use_case_13[] = {{TOKEN_FLOAT, set_scaffold_left}, {TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_scaffold_right}};
	token_details const use_case_14[] = {{TOKEN_FLOAT, set_scaffold_left}, {TOKEN_FLOAT, set_scaffold_step}, {TOKEN_FLOAT, set_scaffold_right}};
	token_details const use_case_15[] = {{TOKEN_FLOAT, set_scaffold_left}, {TOKEN_FLOAT, set_scaffold_right}};
	token_details const use_case_16[] = {{TOKEN_FLOAT, set_scaffold_left}, {TOKEN_DOTDOT, NULL}};
	token_details const use_case_17[] = {{TOKEN_DOTDOT, NULL}, {TOKEN_FLOAT, set_scaffold_right}};
	token_details const use_case_18[] = {{TOKEN_MULTIPLIER, set_scaffold_count}};

	use_case table[19];
	table[0].details = use_case_0;
	table[0].length = sizeof(use_case_0) / sizeof(token_details);
	table[1].details = use_case_1;
	table[1].length = sizeof(use_case_1) / sizeof(token_details);
	table[2].details = use_case_2;
	table[2].length = sizeof(use_case_2) / sizeof(token_details);
	table[3].details = use_case_3;
	table[3].length = sizeof(use_case_3) / sizeof(token_details);
	table[4].details = use_case_4;
	table[4].length = sizeof(use_case_4) / sizeof(token_details);
	table[5].details = use_case_5;
	table[5].length = sizeof(use_case_5) / sizeof(token_details);
	table[6].details = use_case_6;
	table[6].length = sizeof(use_case_6) / sizeof(token_details);
	table[7].details = use_case_7;
	table[7].length = sizeof(use_case_7) / sizeof(token_details);
	table[8].details = use_case_8;
	table[8].length = sizeof(use_case_8) / sizeof(token_details);
	table[9].details = use_case_9;
	table[9].length = sizeof(use_case_9) / sizeof(token_details);
	table[10].details = use_case_10;
	table[10].length = sizeof(use_case_10) / sizeof(token_details);
	table[11].details = use_case_11;
	table[11].length = sizeof(use_case_11) / sizeof(token_details);
	table[12].details = use_case_12;
	table[12].length = sizeof(use_case_12) / sizeof(token_details);
	table[13].details = use_case_13;
	table[13].length = sizeof(use_case_13) / sizeof(token_details);
	table[14].details = use_case_14;
	table[14].length = sizeof(use_case_14) / sizeof(token_details);
	table[15].details = use_case_15;
	table[15].length = sizeof(use_case_15) / sizeof(token_details);
	table[16].details = use_case_16;
	table[16].length = sizeof(use_case_16) / sizeof(token_details);
	table[17].details = use_case_17;
	table[17].length = sizeof(use_case_17) / sizeof(token_details);
	table[18].details = use_case_18;
	table[18].length = sizeof(use_case_18) / sizeof(token_details);

	for (i = 0; i < args_len; i++) {
		setter_value value;
		token_type type = identify_token(args[i], &value);

		if (! IS_TOKEN_ERROR(type)) {
			for (j = 0; j < (sizeof(table) / sizeof(use_case)); j++) {
				if (table[j].length == 0) {
					continue;
				}

				if (table[j].length <= i) {
					table[j].length = 0;
					continue;
				}

				if (table[j].details[i].type != type) {
					table[j].length = 0;
					continue;
				}
			}
		} else {
			return PARSE_ERROR_UNKNOWN_TYPE;
		}
	}

	for (k = 0; k < (sizeof(table) / sizeof(use_case)); k++) {
		if (table[k].length == 0)
			continue;

		if (table[k].length != args_len)
			continue;

		/* valid case left */
		assert(valid_case == NULL);
		valid_case = table + k;
	}

	if (valid_case) {
		setter_value value;
		for (l = 0; l < valid_case->length; l++) {
			token_type type = identify_token(args[l], &value);
			assert(type == valid_case->details[l].type);

			if (type != TOKEN_DOTDOT) {
				int success = valid_case->details[l].setter(dest, value);
				if (! success) {
					/* only case for no success: step == 0 */
					return PARSE_ERROR_ZERO_STEP;
				}
				/* auto-detect precision */
				if (type == TOKEN_FLOAT) {
					if (! CHECK_FLAG(dest->flags, FLAG_USER_PRECISION)) {
						unsigned int flen = strlen(args[l]);
						for (m = 0; m < flen; m++) {
							if (args[l][m] == '.') {
								INCREASE_PRECISION(*dest, flen - m - 1);
								break;
							}
						}
					}
				}
			}
		}

		/* Reject left == right */
		if (((dest->flags & (FLAG_LEFT_SET | FLAG_RIGHT_SET))
					== (FLAG_LEFT_SET | FLAG_RIGHT_SET))
				&& (dest->left == dest->right)) {
			return PARSE_ERROR_ZERO_RANGE;
		}

		return PARSE_SUCCESS;
	} else {
		return PARSE_ERROR_INVALID_INPUT;
	}
}
