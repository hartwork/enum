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
#include <math.h>   /* for log10 */

/** Macro to have a boolean kind of answer about if a token is invalid somehow
 * @since 0.3
 */
#define IS_TOKEN_ERROR(type)  ((type) >= TOKEN_ERROR)

/** @name Shortcuts for setting up the use_case table
 * @since 0.3
 */
/*@{*/
#define LEFT	{TOKEN_FLOAT, set_scaffold_left}
#define RIGHT	{TOKEN_FLOAT, set_scaffold_right}
#define STEP	{TOKEN_FLOAT, set_scaffold_step}
#define COUNT	{TOKEN_MULTIPLIER, set_scaffold_count}
#define DOTDOT	{TOKEN_DOTDOT, NULL}

#define INSTALL_USE_CASE(UC, RAND) \
	assert(sizeof(table) / sizeof(use_case) > UC); \
	table[UC].details = use_case_##UC; \
	table[UC].length = sizeof(use_case_##UC) / sizeof(token_details); \
	table[UC].random = RAND;
/*@}*/

/** Enumeration for token types.
 *
 * What a user gives us as command line argument is considered a token. There
 * are a few valid types that we can use, the rest are error cases.
 *
 * Note that all types after (and including) TOKEN_ERROR are considered to be
 * errors.
 *
 * @see IS_TOKEN_ERROR
 *
 * @since 0.3
 */
typedef enum _token_type {
  TOKEN_FLOAT,            /**< what is considered to be a floating point number */
  TOKEN_MULTIPLIER,       /**< number followed by an x */
  TOKEN_DOTDOT,           /**< two dots */

  /* Only errors from here */
  TOKEN_ERROR,            /**< general token error (unused) */
  TOKEN_ERROR_EMPTY,      /**< token is empty */
  TOKEN_ERROR_BAD_COUNT,  /**< looking like a count value can't be one */
  TOKEN_ERROR_BAD_FLOAT,  /**< should be float but isn't, like nan/inf */
  TOKEN_ERROR_PARSE       /**< token not recognizable at all */
} token_type;

/** union used in function_pointer
 * @see setter_function_pointer
 *
 * @since 0.3
 */
typedef union _setter_value {
	unsigned int uint_data; /**< unsigned int */
	float float_data;       /**< float */
} setter_value;

/** function_pointer to install a value into scaffold
 * @since 0.3
 */
typedef int (*setter_function_pointer)(scaffolding *, setter_value);

/** structure to keep function_pointer and token type together
 * @since 0.3
 */
typedef struct _token_details {
	token_type type;                /**< type of current token */
	setter_function_pointer setter; /**< function to set token */
} token_details;

/** use case structure.
 *
 * All possible combinations of command line arguments are stored as use cases.
 * This structure keeps the important details about such use case.
 *
 * @since 0.3
 */
typedef struct _use_case {
	token_details const *details; /**< combination of token type and its setter function */
	unsigned int length;          /**< length of use case */
	unsigned int random;          /**< bool for random usefulness */
} use_case;

/** from getopt */
extern int opterr;

/** @name Setter functions
 * Functions to be used through function_pointer.
 *
 * For each value possibly given by command line arguments a setter function is
 * implemented and called by a function_pointer.
 *
 * @see setter_function_pointer
 *
 * @param[out] scaffold
 * @param[in] value
 *
 * @return boolean meaning of 1 or 0
 *
 * @since 0.3
 */

/*@{*/
static int set_scaffold_left(scaffolding * scaffold, setter_value value) {
	scaffold->flags |= FLAG_LEFT_SET;
	scaffold->left = value.float_data;
	return 1;
}

static int set_scaffold_step(scaffolding * scaffold, setter_value value) {
	if (value.float_data == 0.0f) {
		/* error code handled below */
		return 0;
	}
	scaffold->flags |= FLAG_STEP_SET;
	scaffold->flags |= FLAG_USER_STEP;
	scaffold->step = value.float_data;
	return 1;
}

static int set_scaffold_right(scaffolding * scaffold, setter_value value) {
	scaffold->flags |= FLAG_RIGHT_SET;
	scaffold->right = value.float_data;
	return 1;
}

static int set_scaffold_count(scaffolding * scaffold, setter_value value) {
	scaffold->flags |= FLAG_COUNT_SET;
	scaffold->count = value.uint_data;
	return 1;
}
/*@}*/

/** Figure out if a string ends on an x.
 *
 * @param[in] str
 *
 * @return boolean meaning of 1 or 0
 *
 * @since 0.3
 */
static int ends_with_x(const char *str) {
	const int len = strlen(str);
	if (str[len - 1] == 'x')
		return 1;
	return 0;
}

/** Figure out if a float is not a number or infinity.
 *
 * @param[in] f
 *
 * @return boolean meaning of 1 or 0
 *
 * @since 0.3
 */
static int is_nan_or_inf(float f) {
	const float INF = strtod("INF", NULL);
	return (enum_is_nan_float(f) || (f == INF) || (f == -INF)) ? 1 : 0;
}

/** Identify type of a token.
 *
 * As defined by token_type every token is of a specific type, possibly an
 * error type, obviously.
 *
 * The token type is important for command line parsing as not every type can
 * be at every position on the command line, and also for the type of number
 * since count must be unsigned int for example.
 *
 * @param[in] arg
 * @param[in] value
 *
 * @return type of token as defined above.
 *
 * @see token_type
 *
 * @since 0.3
 */
static token_type identify_token(const char *arg, setter_value *value) {
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

/** Escape a string by given escape char and copy new string.
 *
 * Escape the string str by escape character esc and write new string to dest.
 *
 * Example: '5§f' escaped by '§' leads to '5§§f'.
 *
 * @param[in] str
 * @param[in] esc
 * @param[out] dest
 *
 * @return boolean meaning of 1 or 0
 *
 * @since 0.3
 */
static int escape_strdup(const char *str, const char esc, char **dest) {
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
	if (newstr == NULL) {
		free(newstr);
		return 0;
	}

	*dest = newstr;
	return 1;
}

/** @name Error handling
 * Enumerations and functions for error handling during parsing.
 */

/*@{*/
/** Errors during parsing of parameters.
 *
 * Parameters are to be considered what others call switches, i.e. command line
 * arguments with one or two leading dashes.
 *
 * @since 0.3
 */
typedef enum _parameter_error {
	PARAMETER_ERROR_OUT_OF_MEMORY,
	PARAMETER_ERROR_INVALID_PRECISION,
	PARAMETER_ERROR_VERSION_NOT_ALONE,
	PARAMETER_ERROR_HELP_NOT_ALONE
} parameter_error;

/** Errors during parsing of arguments.
 *
 * Arguments, in contrast to parameters, are given on the command line without
 * leading dashes. They provide the basic information needed to fill a
 * scaffold.
 *
 * @since 0.3
 */
typedef enum _parse_return {
	PARSE_ERROR_UNKNOWN_TYPE,  /* token error: type of argument not known */
	PARSE_ERROR_ZERO_STEP,     /* step == 0 */
	PARSE_ERROR_INVALID_INPUT, /* generic parsing error */
	PARSE_ERROR_INVALID_RANDOM /* useless arguments for random mode */
} parse_return;

/** Print error message to stderr.
 *
 * @param[in] message
 *
 * @since 0.3
 */
static void fatal(const char * message) {
	fprintf(stderr, "ERROR: %s\n", message);
}

/** Report error during parameter parsing.
 *
 * @param[in] code
 *
 * @since 0.3
 */
static void report_parameter_error(int code) {
	switch (code) {
	case PARAMETER_ERROR_OUT_OF_MEMORY:
		fatal("System too low on memory to continue.");
		break;
	case PARAMETER_ERROR_INVALID_PRECISION:
		fatal("Precision must be a non-negative integer.");
		break;
	case PARAMETER_ERROR_VERSION_NOT_ALONE:
		fatal("-V and --version must come alone.");
		break;
	case PARAMETER_ERROR_HELP_NOT_ALONE:
		fatal("-h and --help must come alone.");
		break;
	default:
		assert(0);
	}
}

/** Report error during argument parsing.
 *
 * In contrast to errors during parameter parsing, argument parsing errors also
 * provide information about which argument failed to be parsed.
 *
 * @param[in] code
 * @param[in] myargc
 * @param[in] myargv
 *
 * @since 0.3
 */
static void report_parse_error(int code, int myargc, char **myargv) {
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
	case PARSE_ERROR_INVALID_RANDOM:
		fatal("Combining random and infinity not supported.");
		break;
	}

	if (myargc > 0) {
		assert(myargv != NULL);
		for (i = 0; i < myargc; i++) {
			fprintf(stderr, "%s ", myargv[i]);
		}
		fprintf(stderr, "\n");
	}
}
/*@}*/

/** Set new format string.
 *
 * Copying the new string to destination pointer, issuing a warning to stderr
 * if an earlier format string thereby is overwritten.
 *
 * @param[in,out] dest
 * @param[in] new
 *
 * @return boolean value of 1 or 0
 *
 * @since 0.3
 */
static int set_format_strdup(char ** dest, const char * new) {
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

/** Store a default format string to scaffold.
 *
 * A default format string, consisting of a floating point number with decimal
 * places according to given precision, is created in allocated memory and then
 * stored into a scaffold's format key, scaffold->format.
 *
 * @param[in,out] dest
 * @param[in] precision
 *
 * @return boolean meaning of 1 or 0
 *
 * @since 0.4
 */
int make_default_format_string(scaffolding * dest, unsigned int precision) {
	const char base[] = "%%.%uf";
	const size_t min_length = sizeof(base) - 2;

	const size_t precision_digits = ((precision == 0) ? 0 : (size_t)log10(precision)) + 1;

	char * const newformat = (char *)malloc(min_length + precision_digits + 1);

	if (newformat != NULL) {
		sprintf(newformat, base, precision);
		if (! set_format_strdup(&(dest->format), newformat)) {
			free(newformat);
			return 0;
		}
		free(newformat);
		return 1;
	}
	return 0;
}

/** @name Command line parsing */

/*@{*/
/** Parsing of command line parameters.
 *
 * Parameters are all parts of given command line args that start with one or
 * two dashes, i.e. whatever getopt can handle.
 *
 * @param[in] original_argc
 * @param[in] original_argv
 * @param[out] dest
 *
 * @return zero in case of failure, non-zero otherwise
 *
 * @since 0.3
 */
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
				if (end - optarg != (int)strlen(optarg) || (strchr(optarg, '-') != NULL)) {
					report_parameter_error(PARAMETER_ERROR_INVALID_PRECISION);
					success = 0;
					break;
				}
				make_default_format_string(dest, precision);
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

/** Parsing of command line arguments that are not considered parameters.
 *
 * Knowing about all possible combinations of command line arguments, a table
 * of use cases is built and compared to what was given by the user.
 *
 * @param[in] reduced_argc
 * @param[in] reduced_argv
 * @param[out] dest
 *
 * @return zero in case of failure, non-zero otherwise
 *
 * @since 0.3
 */
int parse_args(unsigned int reduced_argc, char **reduced_argv, scaffolding *dest) {
	unsigned int i;
	unsigned int j;
	unsigned int k;
	unsigned int l;
	unsigned int m;
	use_case const *valid_case = NULL;

	token_details const use_case_0[]  = {RIGHT}; /* seq compatibility #1 */
	token_details const use_case_1[]  = {LEFT, DOTDOT, COUNT, STEP, DOTDOT, RIGHT};
	token_details const use_case_2[]  = {DOTDOT, COUNT, STEP, DOTDOT, RIGHT};
	token_details const use_case_3[]  = {LEFT, DOTDOT, COUNT, STEP, DOTDOT};
	token_details const use_case_4[]  = {LEFT, DOTDOT, COUNT, DOTDOT, RIGHT};
	token_details const use_case_5[]  = {LEFT, DOTDOT, STEP, DOTDOT, RIGHT};
	token_details const use_case_6[]  = {DOTDOT, COUNT, STEP, DOTDOT};
	token_details const use_case_7[]  = {DOTDOT, COUNT, DOTDOT, RIGHT};
	token_details const use_case_8[]  = {DOTDOT, STEP, DOTDOT, RIGHT};
	token_details const use_case_9[]  = {LEFT, DOTDOT, COUNT, DOTDOT};
	token_details const use_case_10[] = {LEFT, DOTDOT, STEP, DOTDOT};
	token_details const use_case_11[] = {DOTDOT, STEP, DOTDOT};
	token_details const use_case_12[] = {DOTDOT, COUNT, DOTDOT};
	token_details const use_case_13[] = {LEFT, DOTDOT, RIGHT};
	token_details const use_case_14[] = {LEFT, STEP, RIGHT}; /* seq compatibility #3 */
	token_details const use_case_15[] = {LEFT, RIGHT}; /* seq compatibility #2 */
	token_details const use_case_16[] = {LEFT, DOTDOT};
	token_details const use_case_17[] = {DOTDOT, RIGHT};
	token_details const use_case_18[] = {COUNT};
	token_details const use_case_19[] = {LEFT, COUNT, RIGHT};

	use_case table[20];
	/* incrementing use_case_NN, random support (bool) */
	INSTALL_USE_CASE( 0, 1)
	INSTALL_USE_CASE( 1, 1)
	INSTALL_USE_CASE( 2, 1)
	INSTALL_USE_CASE( 3, 0)
	INSTALL_USE_CASE( 4, 1)
	INSTALL_USE_CASE( 5, 1)
	INSTALL_USE_CASE( 6, 0)
	INSTALL_USE_CASE( 7, 1)
	INSTALL_USE_CASE( 8, 1)
	INSTALL_USE_CASE( 9, 0)
	INSTALL_USE_CASE(10, 0)
	INSTALL_USE_CASE(11, 0)
	INSTALL_USE_CASE(12, 0)
	INSTALL_USE_CASE(13, 1)
	INSTALL_USE_CASE(14, 1)
	INSTALL_USE_CASE(15, 1)
	INSTALL_USE_CASE(16, 0)
	INSTALL_USE_CASE(17, 1)
	INSTALL_USE_CASE(18, 0)
	INSTALL_USE_CASE(19, 1)

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
			report_parse_error(PARSE_ERROR_INVALID_RANDOM, 0, NULL);
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
/*@}*/
