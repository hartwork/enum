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
#include "printing.h"
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
	PARAMETER_ERROR_HELP_NOT_ALONE,
	PARAMETER_ERROR_INVALID_SEED
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
	case PARAMETER_ERROR_INVALID_SEED:
		fatal("Seed must be a non-negative integer.");
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
 * Copying the new string to destination pointer.
 *
 * @param[in,out] dest
 * @param[in] new
 *
 * @return boolean value of 1 or 0
 *
 * @since 0.3
 */
static int set_format_strdup(char ** dest, const char * new) {
	assert(dest);
	assert(! *dest);

	*dest = enum_strdup(new);
	if (*dest == NULL)
		return 0;
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
	char * newformat = NULL;
	const size_t post_dot_bytes_needed = ((precision == 0) ? 0 : (size_t)log10(precision)) + 1;

	if (HAS_RIGHT(dest) && CHECK_FLAG(dest->flags, FLAG_EQUAL_WIDTH)) {
		const char * const equal_width_base = "%%0%u.%uf";
		const int left_len = (dest->left < 0) + (size_t)log10(fabs(dest->left)) + 1;
		const int right_len = (dest->right < 0) + (size_t)log10(fabs(dest->right)) + 1;
		const size_t pre_dot_digits_wanted = ENUM_MAX(left_len, right_len);
		const size_t total_chars_wanted = pre_dot_digits_wanted + (precision ? 1 + precision : 0);
		const size_t pre_dot_bytes_needed = (size_t)log10(total_chars_wanted) + 1;
		const size_t base_bytes = strlen(equal_width_base)
			- strlen("%")
			- strlen("%u") + pre_dot_bytes_needed
			- strlen("%u") + post_dot_bytes_needed
			+ 1;
		newformat = (char *)malloc(base_bytes);
		if (! newformat)
		    return 0;
		sprintf(newformat, equal_width_base, total_chars_wanted, precision);
	} else {
		const char * const default_base = "%%.%uf";
		const size_t base_bytes = strlen(default_base)
			- strlen("%")
			- strlen("%u") + post_dot_bytes_needed
			+ 1;
		newformat = (char *)malloc(base_bytes);
		if (! newformat)
		    return 0;
		sprintf(newformat, default_base, precision);
	}

	if (! set_format_strdup(&(dest->format), newformat)) {
		free(newformat);
		return 0;
	}

	free(newformat);
	return 1;
}

typedef enum _separator_change {
	APPLY_SEPARATOR = 1 << 0,
	APPLY_NULL_BYTES = 1 << 1
} separator_change;

/** Save given separator to scaffold.
 *
 * Save a given separator to scaffold, error out if malloc fails, warn if a
 * separator was already set (except the default separator '\n' which was set
 * in main.c already).
 *
 * @param[in,out] scaffold
 * @param[in] string
 * @param[in] action_to_take
 *
 * @return boolean meaing of 1 or 0
 *
 * @since 0.5
 */
static int set_separator(scaffolding * scaffold, const char * string,
		separator_change action_to_take) {
	if (CHECK_FLAG(scaffold->flags, FLAG_NULL_BYTES)) {
		if (action_to_take == APPLY_SEPARATOR) {
			fprintf(stderr,
				"WARNING: Discarding null byte separator "
				"in favor of \"%s\".\n", string);
		} else {
			assert(action_to_take == APPLY_NULL_BYTES);
			fprintf(stderr,
				"WARNING: Duplicate -z|--zero|--null detected.\n");
		}
	}

	if (scaffold->separator) {
		if (action_to_take == APPLY_SEPARATOR) {
			fprintf(stderr,
				"WARNING: Discarding previous separator "
				"\"%s\" in favor of \"%s\".\n",
				scaffold->separator, string
				);
		} else {
			assert(action_to_take == APPLY_NULL_BYTES);
			fprintf(stderr,
				"WARNING: Discarding previous separator "
				"\"%s\" in favor of null bytes.\n",
				scaffold->separator);
		}
		free(scaffold->separator);
		scaffold->separator = NULL;
	}

	switch (action_to_take) {
	case APPLY_SEPARATOR:
		scaffold->separator = enum_strdup(string);
		if (! scaffold->separator)
			return 0;

		/* remove null bytes flag */
		scaffold->flags &= ~FLAG_NULL_BYTES;
		break;

	case APPLY_NULL_BYTES:
		scaffold->flags |= FLAG_NULL_BYTES;
		break;

	default:
		assert(0);
	}

	return 1;
}

/** Append a string to an array of strings.
 *
 * The array is expected to have enough memory available.
 *
 * @param[out] new_argc
 * @param[out] new_argv
 * @param[in] token
 *
 * @return boolean meaning of 1 or 0
 *
 * @since 0.5
 */
static int save_new_token(unsigned int * new_argc, char ** new_argv, char * token) {
	char * newstr = enum_strdup(token);

	if (newstr == NULL)
		return 0;

	new_argv[*new_argc] = newstr;
	*new_argc += 1;
	return 1;
}

typedef enum _format_change {
	APPLY_FORMAT = 1 << 0,
	SAVE_PRECISION = 1 << 1,
	SAVE_EQUAL_WIDTH = 1 << 2
} format_change;

/** Erase formatting settings conflicting to upcoming changes and produce warnings as needed.
 *
 * @param[in] scaffold Settings to scan
 *
 * @since 0.5
 */
static void prepare_setting_format(scaffolding * scaffold, format_change expected_format_change) {
	assert(! ((CHECK_FLAG(scaffold->flags, FLAG_USER_PRECISION)
			|| CHECK_FLAG(scaffold->flags, FLAG_EQUAL_WIDTH))
		&& (scaffold->format != NULL)));

	/* Detect causes for warnings and make warning message as-needed */
	if ((CHECK_FLAG(scaffold->flags, FLAG_USER_PRECISION)
			&& (expected_format_change == SAVE_PRECISION))
		|| ((CHECK_FLAG(scaffold->flags, FLAG_USER_PRECISION)
				|| CHECK_FLAG(scaffold->flags, FLAG_EQUAL_WIDTH))
			&& (expected_format_change == APPLY_FORMAT))) {
		char const * format;

		if (CHECK_FLAG(scaffold->flags, FLAG_USER_PRECISION)) {
			if (CHECK_FLAG(scaffold->flags, FLAG_EQUAL_WIDTH)) {
				format = "WARNING: Discarding format previously "
				    "set by -e|--equal-width and -p|--precision %d.\n";
			} else {
				format = "WARNING: Discarding format previously "
				    "set by -p|--precision %d.\n";
			}
		} else {
			assert(CHECK_FLAG(scaffold->flags, FLAG_EQUAL_WIDTH));
			format = "WARNING: Discarding format previously "
			    "set by -e|--equal-width.\n";
		}
		fprintf(stderr, format, scaffold->user_precision);
	} else if (scaffold->format) {
		fprintf(stderr, "WARNING: Discarding previous format \"%s\".\n", scaffold->format);
	}

	/* Remove previous format */
	if (scaffold->format) {
		free(scaffold->format);
		scaffold->format = NULL;
	}

	/* Remove settings conflicting to upcoming changes */
	if (expected_format_change == APPLY_FORMAT) {
		/* remove user precision and equal width flags */
		scaffold->flags &= ~FLAG_USER_PRECISION;
		scaffold->flags &= ~FLAG_EQUAL_WIDTH;
	}
}

/** Wraps a call to is_valid_format and reports errors to stderr.
 *
 * @param[in] format Format to check
 *
 * @return 1 for valid, 0 for invalid
 *
 * @since 1.0
 */
static int analyze_format(const char * format) {
	custom_printf_return const res = is_valid_format(format);

	switch (res) {
	case CUSTOM_PRINTF_SUCCESS:
		return 1;

	case CUSTOM_PRINTF_INVALID_FORMAT_ENUM:
	case CUSTOM_PRINTF_INVALID_FORMAT_PRINTF:
		fprintf(stderr, "ERROR: Invalid format \"%s\".\n", format);
		return 0;

	case CUSTOM_PRINTF_OUT_OF_MEMORY:
		report_parameter_error(PARAMETER_ERROR_OUT_OF_MEMORY);
		return 1;

	default:
		assert(0);
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
			{"line",         no_argument,       0, 'l'},
			{"seed",         required_argument, 0, 'i'},
			{"format",       required_argument, 0, 'f'},
			{"word",         required_argument, 0, 'w'},
			{"dumb",         required_argument, 0, 'b'},
			{"separator",    required_argument, 0, 's'},
			{"precision",    required_argument, 0, 'p'},
			{"equal-width",  no_argument,       0, 'e'},
			{"null",         no_argument,       0, 'z'},
			{"zero",         no_argument,       0, 'z'},
			{0, 0, 0, 0}
		};

		c = getopt_long(original_argc, original_argv, "+b:cef:hi:lnp:rs:Vw:z", long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'b':
			{
				char * newformat;

				prepare_setting_format(dest, APPLY_FORMAT);

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
			prepare_setting_format(dest, APPLY_FORMAT);

			if (! set_format_strdup(&(dest->format), "%c")) {
				report_parameter_error(PARAMETER_ERROR_OUT_OF_MEMORY);
				success = 0;
			}
			break;

		case 'f':
		case 'w':
			prepare_setting_format(dest, APPLY_FORMAT);

			if (! analyze_format(optarg)) {
				success = 0;
				break;
			}

			if (! set_format_strdup(&(dest->format), optarg)) {
				report_parameter_error(PARAMETER_ERROR_OUT_OF_MEMORY);
				success = 0;
			}
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
			{
				unsigned int seed_candidate;
				char * end;

				if (CHECK_FLAG(dest->flags, FLAG_USER_SEED)) {
					fprintf(stderr,
						"WARNING: Discarding previously specified seed of %d.\n",
						dest->seed);
				}

				seed_candidate = strtoul(optarg, &end, 10);
				if (end - optarg != (int)strlen(optarg) || (strchr(optarg, '-') != NULL)) {
					report_parameter_error(PARAMETER_ERROR_INVALID_SEED);
					success = 0;
					break;
				}

				dest->flags |= FLAG_USER_SEED;
				dest->seed = seed_candidate;
			}
			break;

		case 'l':
			if (! set_separator(dest, " ", APPLY_SEPARATOR)) {
				report_parameter_error(PARAMETER_ERROR_OUT_OF_MEMORY);
				success = 0;
			}
			break;

		case 'n':
			/* remove newline flag */
			dest->flags &= ~FLAG_NEWLINE;
			break;

		case 'e':
			prepare_setting_format(dest, SAVE_EQUAL_WIDTH);

			dest->flags |= FLAG_EQUAL_WIDTH;
			break;

		case 'p':
			{
				unsigned int precision_candidate;
				char * end;

				prepare_setting_format(dest, SAVE_PRECISION);

				precision_candidate = strtoul(optarg, &end, 10);
				if (end - optarg != (int)strlen(optarg) || (strchr(optarg, '-') != NULL)) {
					report_parameter_error(PARAMETER_ERROR_INVALID_PRECISION);
					success = 0;
					break;
				}

				dest->flags |= FLAG_USER_PRECISION;
				dest->user_precision = precision_candidate;
			}
			break;

		case 'r':
			dest->flags |= FLAG_RANDOM;
			break;

		case 's':
			/* address of optarg in argv */
			if (! set_separator(dest, optarg, APPLY_SEPARATOR)) {
				report_parameter_error(PARAMETER_ERROR_OUT_OF_MEMORY);
				success = 0;
			}
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

		case 'z':
			set_separator(dest, NULL, APPLY_NULL_BYTES);
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

	/* Any paramaters or arguments given? */
	if (original_argc == 1) {
		fatal("No arguments given");
		success = 0;
		usage_needed = 1;
	}

	/* Seed given without random flag? */
	if (CHECK_FLAG(dest->flags, FLAG_USER_SEED)
			&& ! CHECK_FLAG(dest->flags, FLAG_RANDOM)) {
		fprintf(stderr, "ERROR: Parameter -i|--seed %u requires -r|--random.\n", dest->seed);
		success = 0;
		usage_needed = 1;
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

/** Preparsing of command line arguments to identify tokens that are not white
 *  space separated.
 *
 * In order to allow tokens not separated from each other by white space, we
 * need to preparse argv, separate them, and build a new argc and argv.
 *
 * @param[in] reduced_argc
 * @param[in] reduced_argv
 * @param[out] new_argc
 * @param[out] new_argv
 *
 * @return zero in case of failure, non-zero otherwise
 *
 * @since 0.5
 */
int preparse_args(unsigned int reduced_argc, char ** reduced_argv,
		unsigned int * new_argc, char *** new_argv) {
	unsigned int i;
	size_t argv_mem; /* How much memory we already allocated */

	/* Currently we support use cases with max 6 tokens. To avoid a few
	 * reallocs, allocate enough memory up front.
	 */
	argv_mem = 6 * sizeof(char *);
	*new_argc = 0;
	*new_argv = (char **)malloc(argv_mem);

	for (i = 0; i < reduced_argc; i++) {
		char * p = reduced_argv[i];

		token_type prev_type = TOKEN_ERROR;
		char * prev_str = NULL;

		/* Deny ambiguous cases like "enum 1...4" */
		if (strstr(p, "...")) {
			if (! save_new_token(new_argc, *new_argv, p))
				return 0;
			break;
		}

		while (*p != '\0') {
			unsigned int j;

			/* make sure to have enough memory */
			assert((*new_argc * sizeof(char *)) <= argv_mem);
			if ((*new_argc * sizeof(char *)) == argv_mem) {
				char ** tmp;
				argv_mem = (*new_argc + 1) * sizeof(char *);
				tmp = (char **)realloc(*new_argv, argv_mem);
				if (tmp == NULL) {
					free(*new_argv);
					return 0;
				}
				*new_argv = tmp;
			}

			for (j = strlen(p); j > 0; j--) {
				setter_value value;
				char * str = enum_strndup(p, j);
				token_type newtype = identify_token(str, &value);

				if (! IS_TOKEN_ERROR(newtype)) {
					/* Don't allow N. */
					if (newtype == TOKEN_FLOAT && str[j - 1] == '.' && j != strlen(p)) {
						free(str);
						continue;
					}

					/* found a token */
					if ((newtype == TOKEN_FLOAT) && (prev_type == TOKEN_FLOAT)) {
						j = 0;
						free(str);
						break;
					}

					if ((prev_type != TOKEN_ERROR) && (prev_str != NULL)) {
						if (! save_new_token(new_argc, *new_argv, prev_str)) {
							free(prev_str);
							free(str);
							return 0;
						}
						free(prev_str);
					}

					prev_type = newtype;
					prev_str = str;
					break;
				}
				free(str);
			}

			if ((prev_type != TOKEN_ERROR) && (prev_str != NULL)) {
				if (! save_new_token(new_argc, *new_argv, prev_str)) {
					free(prev_str);
					return 0;
				}
				free(prev_str);
				prev_str = NULL;
			}

			if (j == 0) {
				/* string ended but rest unusable, let's declare complete string unusable */
				if (! save_new_token(new_argc, *new_argv, reduced_argv[i]))
					return 0;
				break;
			}
			p += j;
		}
	}
	return 1;
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

	/* Any arguments given? */
	if (reduced_argc == 0) {
		fatal("No arguments given");
		return 0;
	}

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
