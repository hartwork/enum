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

#include "generator.h"
#include "assertion.h"
#include <stdlib.h>  /* for rand */
#include <math.h>  /* for fabs, ceil, floor, fmod, log, pow, rand */
#include <float.h>  /* for FLT_MAX */

/** @name Min/Max macros
 * Macros to find the smaller/larger number.
 *
 * @param[in] a
 * @param[in] b
 *
 * @return a or b respectively, depending on which is smaller
 *
 * @since 0.3
 */

/*@{*/
#define ENUM_MIN(a, b)  (((a) <= (b)) ? (a) : (b))
#define ENUM_MAX(a, b)  (((a) >= (b)) ? (a) : (b))
/*@}*/

/** Calculate precision of float.
 *
 * Precision, i.e. number of decimal places, is calculated by counting the
 * sensible number of decimal places of the given float.
 *
 * @param[in] element floating point number to calculate the precision from
 *
 * @return number of decimal places of given float
 *
 * @since 0.3
 */
static unsigned int calc_precision(float element) {
	unsigned int precision = 0;
	unsigned int ptemp;
	unsigned int i;

	ptemp = (element - (int)element) * pow(10, MAX_PD_DIGITS);
	if (ptemp != 0) {
		precision = MAX_PD_DIGITS;
		for (i = MAX_PD_DIGITS; i > 0; i--) {
			if (ptemp % (int)pow(10, i) != 0) {
				precision = MAX_PD_DIGITS - i + 1;
			}
		}
	}

	return precision;
}

/** Calculate a default step altered by precision.
 *
 * Default step is 1. If precision, i.e. number of decimal places, is higher
 * than 1, the step needs to be adjusted to reflect that precision.
 *
 * @param[in] precision
 *
 * @return step with possibly higher precision (float)
 *
 * @since 0.3
 */
static float precision_to_step(unsigned int precision) {
	return 1.0f / pow(10, precision);
}


/** Correct sign of step in necessary.
 *
 * Ensure step direction aligns with relation between left and right.
 *
 * @param[in,out] scaffold
 *
 * @since 0.3
 */
static void ensure_proper_step_sign(scaffolding * scaffold) {
	const int expected_direction = (scaffold->left <= scaffold->right) ? +1 : -1;
	const int step_direction = (scaffold->step >= 0) ? +1 : -1;
	if (expected_direction != step_direction) {
		scaffold->step = -scaffold->step;
	}
}

/** Calculate values in scaffold not given by user input.
 *
 * Main function to produce a usable scaffold for output calculation. All
 * values unknown after user input should be calculated here, i.e. scaffold is
 * output usable after calling this function.
 *
 * @param[in,out] scaffold
 *
 * @since 0.3
 */
void complete_scaffold(scaffolding * scaffold) {
	assert(KNOWN(scaffold) >= 0);

	if (KNOWN(scaffold) == 1) {
		if (! HAS_LEFT(scaffold)) {
			SET_LEFT(*scaffold, 1.0f);
		} else {
			SET_STEP(*scaffold, 1.0f);
		}
		assert(KNOWN(scaffold) == 2);
	}

	assert(KNOWN(scaffold) >= 2);
	if (KNOWN(scaffold) == 2) {
		if (HAS_LEFT(scaffold) && HAS_STEP(scaffold)) {
			/* running to infinity */
			scaffold->flags |= FLAG_READY;
			assert(! HAS_COUNT(scaffold) && ! HAS_RIGHT(scaffold));
			assert(KNOWN(scaffold) == 2);
			return;
		}

		/* NOTE: Step has higher precedence */
		if (! HAS_STEP(scaffold)) {
			if (HAS_LEFT(scaffold) && HAS_RIGHT(scaffold)) {
				/* Special case for decimal place precision */
				SET_STEP(*scaffold, precision_to_step(scaffold->precision));
			} else {
				SET_STEP(*scaffold, 1.0f);
			}
		} else {
			if (HAS_RIGHT(scaffold)) {
				assert(HAS_STEP(scaffold));
				SET_LEFT(*scaffold, scaffold->right
					- scaffold->step * floor(
					scaffold->right / scaffold->step));
			} else {
				SET_LEFT(*scaffold, 1.0f);
			}
		}
		assert(KNOWN(scaffold) == 3);
	}

	assert(KNOWN(scaffold) >= 3);
	if (KNOWN(scaffold) == 3) {
		if (! HAS_LEFT(scaffold)) {
			if (CHECK_FLAG(scaffold->flags, FLAG_RANDOM)) {
				SET_LEFT(*scaffold, 1.0);
			} else {
				SET_LEFT(*scaffold, scaffold->right - (scaffold->count - 1)
					* scaffold->step);
			}
		} else if (! HAS_COUNT(scaffold)) {
			if (CHECK_FLAG(scaffold->flags, FLAG_RANDOM)) {
				SET_COUNT(*scaffold, 1);
			} else {
				assert(HAS_STEP(scaffold));
				assert(HAS_LEFT(scaffold));
				assert(HAS_RIGHT(scaffold));

				ensure_proper_step_sign(scaffold);
				{
					int count_candidate =
						ceil(fabs(scaffold->right - scaffold->left)
						/ fabs(scaffold->step) + 1);

					/* Will values be running over right? */
					/* For instance for "enum 1 .. 2 .. 4" count must be 2, not 3. */
					const float last = scaffold->left
						+ scaffold->step * (count_candidate - 1);
					const float over_by = (scaffold->step > 0)
						? last - scaffold->right
						: scaffold->right - last;
					if (over_by > FLOAT_EQUAL_DELTA) {
						count_candidate--;
					}

					assert(count_candidate > 0);
					SET_COUNT(*scaffold, (unsigned int)count_candidate);
				}
			}
		} else if (! HAS_STEP(scaffold)) {
			if (CHECK_FLAG(scaffold->flags, FLAG_RANDOM)) {
				SET_STEP(*scaffold, precision_to_step(scaffold->precision));
			} else {
				assert(scaffold->count - 1 != 0);
				SET_STEP(*scaffold, (scaffold->right - scaffold->left)
						/ (scaffold->count - 1));
				/* correct precision */
				INCREASE_PRECISION(*scaffold, calc_precision(scaffold->step));
			}
		} else {
			assert(! HAS_RIGHT(scaffold));
			assert(scaffold->count - 1 != 0);
			SET_RIGHT(*scaffold, scaffold->left + scaffold->step
				* (scaffold->count - 1));
		}
	}

	scaffold->flags |= FLAG_READY;
	assert(KNOWN(scaffold) == 4);

	ensure_proper_step_sign(scaffold);
}

/** Calculate a random value out of possible output values.
 *
 * Left and right borders given as well as the step, a random value is "picked"
 * out of the list of possible output values and returned.
 *
 * @param[in] min
 * @param[in] max
 * @param[in] step_width
 *
 * @return A random value within given range considering step
 *
 * @since 0.3
 */
static float discrete_random_closed(float min, float max, float step_width) {
	const float distance = fabs(max - min) + step_width;
	double zero_to_almost_one = 0;
	double zero_to_almost_distance;
	unsigned int depth;
	int unsigned u;

	if (min == max) {
		return min;
	}
	assert(min < max);
	assert(step_width > 0);

	/* Note: Using log(a)+log(b) for log(a*b) to push down overflow border */
	depth = ENUM_MAX(1, ceil((log(distance) + log(step_width)) / log(RAND_MAX)));

	/* Make random resolution at least on par with step_width */
	/* z = (r * RAND_MAX^0 + .. + r * RAND_MAX^(n-1)) / RAND_MAX^n */
	for (u = 0; u < depth; u++) {
		zero_to_almost_one += rand() * pow(RAND_MAX, u);
	}
	zero_to_almost_one /= pow(RAND_MAX, depth);

	zero_to_almost_distance = zero_to_almost_one * distance;
	return min + zero_to_almost_distance - fmod(zero_to_almost_distance, step_width);
}

/** Calculate a candidate for output.
 *
 * Based on known values, calculate a possible candidate for yield to place
 * into scaffold or check if there will be another valid one.
 *
 * @param[in] scaffold
 *
 * @return A calculated candidate for yield to place into scaffold
 *
 * @since 0.5
 */
static float calc_candidate(scaffolding const * scaffold) {
	if (! CHECK_FLAG(scaffold->flags, FLAG_USER_STEP)
			&& HAS_RIGHT(scaffold) && HAS_COUNT(scaffold)) {
		return  scaffold->left + (scaffold->right - scaffold->left)
			/ (scaffold->count - 1) * scaffold->position;
	} else {
		return scaffold->left + scaffold->step * scaffold->position;
	}
	/* TODO check for float overflow, float imprecision */
}

/** Checks a candidate for validity.
 *
 * @param[in] scaffold Settings to apply
 *
 * @return Validity bool (1 means valid, 0 mean invalid)
 *
 * @since 0.5
 */
static int check_candidate(scaffolding const * scaffold, float candidate) {
	if (HAS_RIGHT(scaffold)
			&& (((scaffold->left <= scaffold->right) && ((candidate - scaffold->right) > FLOAT_EQUAL_DELTA))
				|| ((scaffold->left >= scaffold->right) && (scaffold->right - candidate) > FLOAT_EQUAL_DELTA))) {
		return 0;
	}
	return 1;
}

/** Main output function.
 *
 * Calculate next value based on given scaffold and write it to dest. Assuming
 * scaffold was filled correctly and not altered inbetween calls, yield will
 * find the next value to be printed.
 *
 * In order to determine its success, yield returns a yield_status indicating
 * whether a value has been written to dest or not, and even if this might be
 * the last one yield would be able to offer.
 *
 * @param[in] scaffold
 * @param[out] dest
 *
 * @return yield
 *
 * @since 0.3
 */
yield_status yield(scaffolding * scaffold, float * dest) {
	float candidate;

	assert(CHECK_FLAG(scaffold->flags, FLAG_READY));

	if (CHECK_FLAG(scaffold->flags, FLAG_RANDOM)) {
		assert(HAS_COUNT(scaffold) && (scaffold->position < scaffold->count));

		assert(HAS_RIGHT(scaffold));

		*dest = discrete_random_closed(
			ENUM_MIN(scaffold->left, scaffold->right),
			ENUM_MAX(scaffold->left, scaffold->right),
			fabs(scaffold->step));
		scaffold->position++;
		if (scaffold->position == scaffold->count) {
			return YIELD_LAST;
		} else {
			return YIELD_MORE;
		}
	}

	assert(HAS_LEFT(scaffold) && HAS_STEP(scaffold));
	assert(! HAS_COUNT(scaffold) || (HAS_COUNT(scaffold) && (scaffold->count > 0)));

	/* Gone too far already? */
	if (HAS_COUNT(scaffold))
		assert(scaffold->position < scaffold->count);

	/* One value only? */
	if (HAS_COUNT(scaffold) && (scaffold->count == 1)) {
		*dest = scaffold->left;
		return YIELD_LAST;
	}

	candidate = calc_candidate(scaffold);

	/* Gone too far now? */
	assert(check_candidate(scaffold, candidate));

	*dest = candidate;
	scaffold->position++;

	/* Will there be more? */
	if ((HAS_COUNT(scaffold) && (scaffold->position == scaffold->count))
			|| (HAS_RIGHT(scaffold) && (*dest == scaffold->right))) {
		return YIELD_LAST;
	} else {
		/* Going too far next time? (position has already been increased) */
		const float future_candidate = calc_candidate(scaffold);
		if (! check_candidate(scaffold, future_candidate)) {
			return YIELD_LAST;
		}
		return YIELD_MORE;
	}
}

/** Initialization of scaffold.
 *
 * In order to have usable defaults in at least some basic scaffold members,
 * initialize it here.
 *
 * @param[out] dest (scaffold)
 *
 * @since 0.3
 */
void initialize_scaffold(scaffolding * dest) {
	dest->flags = 0;
	dest->position = 0;
	dest->precision = 0;
	dest->format = NULL;
	dest->separator = NULL;
}
