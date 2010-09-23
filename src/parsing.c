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
#include "info.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#define IS_TOKEN_ERROR(type)  ((type) >= TOKEN_ERROR)

#define INSTALL_USE_CASE(UC, RAND) \
	assert(sizeof(table) / sizeof(use_case) > UC); \
	table[UC].details = use_case_##UC; \
	table[UC].length = sizeof(use_case_##UC) / sizeof(token_details); \
	table[UC].random = RAND;

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
	unsigned int random;          /* bool for random usefulness */
} use_case;

extern int opterr;

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
	scaffold->flags |= FLAG_USER_STEP;
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
	const float INF = strtod("INF", NULL);
	return (enum_is_nan_float(f) || (f == INF) || (f == -INF)) ? 1 : 0;
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

int escape_strdup(const char *str, const char esc, char **dest) {
	unsigned int len;
	unsigned int rpos;
	unsigned int wpos;
	char *newstr;

	len = strlen(str);
	newstr = (char *)malloc(len * 2 + 1);
	if (newstr == NULL)
		return 0;

	for (rpos = wpos = 0; rpos <= len; rpos++, wpos++) {
		newstr[wpos] = str[rpos];
		if (str[rpos] == esc) {
			wpos++;
			newstr[wpos] = esc;
		}
	}
	newstr = (char *)realloc(newstr, strlen(newstr));
	if (newstr == NULL)
		return 0;

	*dest = newstr;
	return 1;
}

typedef enum _parameter_error {
	PARAMETER_ERROR_OUT_OF_MEMORY,
	PARAMETER_ERROR_INVALID_PRECISION,
	PARAMETER_ERROR_VERSION_NOT_ALONE,
	PARAMETER_ERROR_HELP_NOT_ALONE,
	PARAMETER_ERROR_INVALID_RANDOM     /* useless arguments for random mode */
} parameter_error;

typedef enum _parse_return {
	PARSE_ERROR_UNKNOWN_TYPE,  /* token error: type of argument not known */
	PARSE_ERROR_ZERO_STEP,     /* step == 0 */
	PARSE_ERROR_INVALID_INPUT  /* generic parsing error */
} parse_return;

void fatal(const char * message) {
	fprintf(stderr, "ERROR: %s\n", message);
}

void report_parameter_error(int code) {
	switch (code) {
	case PARAMETER_ERROR_OUT_OF_MEMORY:
		fatal("System too low on memory to continue.");
		break;
	case PARAMETER_ERROR_INVALID_PRECISION:
		fatal("Precision must be an integer between 0 and 99.");
		break;
	case PARAMETER_ERROR_VERSION_NOT_ALONE:
		fatal("-V and --version must come alone.");
		break;
	case PARAMETER_ERROR_HELP_NOT_ALONE:
		fatal("-h and --help must come alone.");
		break;
	case PARAMETER_ERROR_INVALID_RANDOM:
		fatal("Command line arguments not useful in random mode");
		break;
	default:
		assert(0);
	}
}

void report_parse_error(int code, int myargc, char **myargv) {
	int i;

	switch (code) {
	case PARSE_ERROR_ZERO_STEP:
		fatal("A step of 0 (zero) is invalid.");
		break;
	case PARSE_ERROR_UNKNOWN_TYPE:
		fatal("Unidentified token:");
		break;
	case PARSE_ERROR_INVALID_INPUT:
		fatal("Combination of command line arguments could not be parsed:");
		break;
	}

	if (myargc > 0) {
		for (i = 0; i < myargc; i++) {
			fprintf(stderr, "%s ", myargv[i]);
		}
		fprintf(stderr, "\n");
	}
}

int set_format_strdup(char ** dest, const char * new) {
	char * newformat;

	if (*dest) {
		newformat = enum_strdup(new);
		if (newformat == NULL)
			return 0;
		fprintf(stderr,
				"WARNING: Discarding previous format "
				"\"%s\" in favor of \"%s\"!\n",
				*dest, newformat
				);
		*dest = newformat;
	} else {
		*dest = enum_strdup(new);
		if (*dest == NULL)
			return 0;
	}
	return 1;
}

int parse_parameters(unsigned int original_argc, char **original_argv, scaffolding *dest) {
	int c;
	int option_index = 0;
	unsigned int precision = 0;

	int success = 1;
	int usage_needed = 0;
	int quit = 0;

	/* Inhibit getopt's own error message for unrecognized options */
	opterr = 0;

	while (1) {
		struct option long_options[] = {
			{"help",         no_argument,       0, 'h'},
			{"version",      no_argument,       0, 'V'},
			{"random",       no_argument,       0, 'r'},
			{"characters",   no_argument,       0, 'c'},
			{"omit-newline", no_argument,       0, 'n'},
			{"seed",         required_argument, 0, 'i'},
			{"format",       required_argument, 0, 'f'},
			{"word",         required_argument, 0, 'w'},
			{"dumb",         required_argument, 0, 'b'},
			{"separator",    required_argument, 0, 's'},
			{"precision",    required_argument, 0, 'p'},
			{0, 0, 0, 0}
		};

		c = getopt_long(original_argc, original_argv, "+b:cf:hi:np:rs:Vw:", long_options, &option_index);

		if (c == -1) {
			/* TODO Move outside while loop */
			if (original_argc == 1) {
				fatal("No arguments given");
				success = 0;
				usage_needed = 1;
			}
			break;
		}

		switch (c) {
		case 'b':
			{
				char * newformat;
				escape_strdup(optarg, '%', &newformat);
				if (newformat == NULL) {
					report_parameter_error(PARAMETER_ERROR_OUT_OF_MEMORY);
					success = 0;
				} else {
					if (! set_format_strdup(&(dest->format), newformat)) {
						report_parameter_error(PARAMETER_ERROR_OUT_OF_MEMORY);
						success = 0;
					}
					free(newformat);
				}
			}
			break;

		case 'c':
			if (! set_format_strdup(&(dest->format), "%c")) {
				report_parameter_error(PARAMETER_ERROR_OUT_OF_MEMORY);
				success = 0;
			}
			break;

		case 'f':
		case 'w':
			if (! set_format_strdup(&(dest->format), optarg)) {
				report_parameter_error(PARAMETER_ERROR_OUT_OF_MEMORY);
				success = 0;
			}
			/* TODO look for %f or similar and error out unless found */
			break;

		case 'h':
			if (original_argc != 2) {
				report_parameter_error(PARAMETER_ERROR_HELP_NOT_ALONE);
				success = 0;
			} else {
				dump_usage(stdout);
				quit = 1;
			}
			break;

		case 'i':
			break;

		case 'n':
			/* remove newline flag */
			dest->flags &= ~FLAG_NEWLINE;
			break;

		case 'p':
			{
				char * end;
				precision = strtoul(optarg, &end, 10);
				if (end - optarg != (int)strlen(optarg)) {
					precision = -1;  /* Triggers range error */
					success = 0;
				}
			}
			if (precision > 99) {
				report_parameter_error(PARAMETER_ERROR_INVALID_PRECISION);
				success = 0;
			} else {
				char * newformat = (char *)malloc(6);
				if (! newformat) {
					report_parameter_error(PARAMETER_ERROR_OUT_OF_MEMORY);
					success = 0;
				} else {
					sprintf(newformat, "%%.%uf", precision);
					if (! set_format_strdup(&(dest->format), newformat)) {
						report_parameter_error(PARAMETER_ERROR_OUT_OF_MEMORY);
						success = 0;
					}
				}
				free(newformat);
			}
			break;

		case 'r':
			dest->flags |= FLAG_RANDOM;
			break;

		case 's':
			/* address of optarg in argv */
			dest->separator = enum_strdup(optarg);
			break;

		case 'V':
			if (original_argc != 2) {
				report_parameter_error(PARAMETER_ERROR_VERSION_NOT_ALONE);
				success = 0;
			} else {
				dump_version();
				quit = 1;
			}
			break;

		case '?':
			fprintf(stderr, "ERROR: Unrecognized option \"%s\"\n", original_argv[optind - 1]);
			success = 0;
			usage_needed = 1;
			break;

		default:
			assert(0);
		}
	}

	if (! success && usage_needed) {
		fprintf(stderr, "\n");
		dump_usage(stderr);
	}

	return success
		? (quit
			? 0
			: optind)
		: -1;
}

int parse_args(unsigned int reduced_argc, char **reduced_argv, scaffolding *dest) {
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
	INSTALL_USE_CASE(0, 1)
	INSTALL_USE_CASE(1, 1)
	INSTALL_USE_CASE(2, 1)
	INSTALL_USE_CASE(3, 1)
	INSTALL_USE_CASE(4, 1)
	INSTALL_USE_CASE(5, 1)
	INSTALL_USE_CASE(6, 1)
	INSTALL_USE_CASE(7, 1)
	INSTALL_USE_CASE(8, 1)
	INSTALL_USE_CASE(9, 1)
	INSTALL_USE_CASE(10, 0)
	INSTALL_USE_CASE(11, 0)
	INSTALL_USE_CASE(12, 1)
	INSTALL_USE_CASE(13, 1)
	INSTALL_USE_CASE(14, 1)
	INSTALL_USE_CASE(15, 1)
	INSTALL_USE_CASE(16, 0)
	INSTALL_USE_CASE(17, 1)
	INSTALL_USE_CASE(18, 1)

	for (i = 0; i < reduced_argc; i++) {
		setter_value value;
		token_type type = identify_token(reduced_argv[i], &value);

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
			report_parse_error(PARSE_ERROR_UNKNOWN_TYPE, 1, &reduced_argv[i]);
			return 0;
		}
	}

	for (k = 0; k < (sizeof(table) / sizeof(use_case)); k++) {
		if (table[k].length == 0)
			continue;

		if (table[k].length != reduced_argc)
			continue;

		if (CHECK_FLAG(dest->flags, FLAG_RANDOM) && (! table[k].random)) {
			report_parameter_error(PARAMETER_ERROR_INVALID_RANDOM);
			return 0;
		}

		/* valid case left */
		assert(valid_case == NULL);
		valid_case = table + k;
	}

	if (valid_case) {
		setter_value value;
		for (l = 0; l < valid_case->length; l++) {
			token_type type = identify_token(reduced_argv[l], &value);
			assert(type == valid_case->details[l].type);

			if (type != TOKEN_DOTDOT) {
				int success = valid_case->details[l].setter(dest, value);
				if (! success) {
					/* only case for no success: step == 0 */
					report_parse_error(PARSE_ERROR_ZERO_STEP, 0, NULL);
					return 0;
				}
				/* auto-detect precision */
				if (type == TOKEN_FLOAT) {
					unsigned int flen = strlen(reduced_argv[l]);
					for (m = 0; m < flen; m++) {
						if (reduced_argv[l][m] == '.') {
							INCREASE_PRECISION(*dest, flen - m - 1);
							break;
						}
					}
				}
			}
		}

		return 1;
	} else {
		report_parse_error(PARSE_ERROR_INVALID_INPUT, reduced_argc, reduced_argv);
		return 0;
	}
}
