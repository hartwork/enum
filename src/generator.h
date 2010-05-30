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

#ifndef GENERATOR_H
#define GENERATOR_H 1

#include "utils.h" /* for CHECK_FLAG */

/** @name Constants
 * Constants used by generator
 *
 * @since 0.3
 */
/*@{*/
#define MAX_POST_DOT_DIGITS  5
#define FLOAT_EQUAL_DELTA  0.0001f
/*@}*/

/** @name Macros to set scaffold values
 *
 * Set scaffold value according to macro name and at the same time set the
 * relevant flag to document this.
 *
 * @param[out] scaffold
 * @param[in] value
 *
 * @since 0.3
 */
/*@{*/
#define SET_LEFT(scaffold, _left)  \
	(scaffold).left = _left; \
	(scaffold).flags |= FLAG_LEFT_SET

#define SET_RIGHT(scaffold, _right)  \
	(scaffold).right = _right; \
	(scaffold).flags |= FLAG_RIGHT_SET

#define SET_STEP(scaffold, _step)  \
	(scaffold).step = _step; \
	(scaffold).flags |= FLAG_STEP_SET

#define SET_COUNT(scaffold, _count)  \
	(scaffold).count = _count; \
	(scaffold).flags |= FLAG_COUNT_SET
/*@}*/

/** Macro to increase precision.
 *
 * If a given value is higher than the precision already set in given scaffold,
 * increase it there.
 *
 * @param[in,out] scaffold
 * @param[in] _precision
 *
 * @since 0.3
 */
#define INCREASE_PRECISION(scaffold, _precision)  \
	if (_precision > (scaffold).auto_precision) { \
		(scaffold).auto_precision = _precision; \
	}

/** @name Scaffold flag checkers
 * Macros to check if scaffold values are set by using their respective flags.
 *
 * @param[in] scaffold
 *
 * @return boolean meaning of 1 or 0 depending on whether the flag is set
 *
 * @since 0.3
 */

/*@{*/
#define HAS_LEFT(scaffold)  CHECK_FLAG(scaffold->flags, FLAG_LEFT_SET)
#define HAS_RIGHT(scaffold)  CHECK_FLAG(scaffold->flags, FLAG_RIGHT_SET)
#define HAS_STEP(scaffold)  CHECK_FLAG(scaffold->flags, FLAG_STEP_SET)
#define HAS_COUNT(scaffold)  CHECK_FLAG(scaffold->flags, FLAG_COUNT_SET)
/*@}*/

/** Report about number of know values in scaffold.
 *
 * Most important values in scaffold are left, right, step, and count. Add 1
 * for each value set in scaffold and return that number.
 *
 * @param scaffold
 *
 * @return 0 to 4
 *
 * @since 0.3
 */
#define KNOWN(scaffold)  (HAS_LEFT(scaffold) + HAS_RIGHT(scaffold) \
	+ HAS_STEP(scaffold) + HAS_COUNT(scaffold))

/** Enumeration for flags set in scaffold->flags */
enum scaffolding_flags {
	FLAG_LEFT_SET = 1 << 0,
	FLAG_RIGHT_SET = 1 << 1,
	FLAG_STEP_SET = 1 << 2,
	FLAG_COUNT_SET = 1 << 3,

	FLAG_READY = 1 << 4,

	FLAG_RANDOM = 1 << 5,
	FLAG_USER_STEP = 1 << 6,
	FLAG_USER_PRECISION = 1 << 7,
	FLAG_EQUAL_WIDTH = 1 << 8,
	FLAG_USER_SEED = 1 << 9,
	FLAG_NULL_BYTES = 1 << 10,
	FLAG_REVERSE = 1 << 11
};

/** Enumeration of possible return states of enum_yield() */
typedef enum _yield_status {
	YIELD_MORE, /**< value calculated, more available */
	YIELD_LAST  /**< value calculated, no more available */
} yield_status;

/** Main data structure for output calculation.
 *
 * The most important source for information in order to calculate the output.
 * It's filled during parsing, then by calculation.
 */
typedef struct _scaffolding {
	int flags;              /**< store for flags to indicate set values */
	float left;             /**< lower border of return values */
	float right;            /**< upper border of return values */
	float step;             /**< step between values */
	unsigned int count;     /**< number of values to return */
	unsigned int position;  /**< current position while walking through values */
	unsigned int auto_precision; /**< derived number of decimal places for future output format */
	unsigned int user_precision; /**< number of decimal places for future output format specified by user */
	char * format;          /**< output format string */
	char * separator;       /**< separation string between output values (default: \n) */
	unsigned int seed;      /**< seed used to init random number generator */
	char * terminator;      /**< terminating string for output (default: \n) */
} scaffolding;

void complete_scaffold(scaffolding * scaffold);
yield_status enum_yield(scaffolding * scaffold, float * dest);
void initialize_scaffold(scaffolding * dest);

#endif /* GENERATOR_H */
