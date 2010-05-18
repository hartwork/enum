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

#define ENUM_MIN(a, b)  (((a) <= (b)) ? (a) : (b))
#define ENUM_MAX(a, b)  (((a) >= (b)) ? (a) : (b))

#define HAS_LEFT(scaffold)  CHECK_FLAG(scaffold->flags, FLAG_LEFT_SET)
#define HAS_RIGHT(scaffold)  CHECK_FLAG(scaffold->flags, FLAG_RIGHT_SET)
#define HAS_STEP(scaffold)  CHECK_FLAG(scaffold->flags, FLAG_STEP_SET)
#define HAS_COUNT(scaffold)  CHECK_FLAG(scaffold->flags, FLAG_COUNT_SET)

void complete_scaffold(scaffolding * scaffold) {
	unsigned int precision = 0;
	unsigned int i;

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
			SET_STEP(*scaffold, 1.0f);
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
			SET_LEFT(*scaffold, scaffold->right - (scaffold->count - 1)
				* scaffold->step);
		} else if (! HAS_COUNT(scaffold)) {
			SET_COUNT(*scaffold, fabs(scaffold->right - scaffold->left)
				/ scaffold->step + 1);
		} else if (! HAS_STEP(scaffold)) {
			SET_STEP(*scaffold, (scaffold->right - scaffold->left)
					/ (scaffold->count - 1));
			/* correct precision if necessary */
			if (! CHECK_FLAG(scaffold->flags, FLAG_USER_PRECISION)) {
				unsigned int ptemp;
				ptemp = (scaffold->step - (int)scaffold->step)
					* pow(10, MAX_PD_DIGITS);
				if (ptemp != 0) {
					precision = MAX_PD_DIGITS;
					for (i = MAX_PD_DIGITS; i > 0; i--) {
						if (ptemp % (int)pow(10, i) != 0) {
							precision = MAX_PD_DIGITS - i + 1;
						}
					}
				}
				INCREASE_PRECISION(*scaffold, precision);
			}
		} else {
			assert(! HAS_RIGHT(scaffold));
			SET_RIGHT(*scaffold, scaffold->left + scaffold->step
				* (scaffold->count - 1));
		}
	}

	scaffold->flags |= FLAG_READY;
	assert(KNOWN(scaffold) == 4);

	{
		/* Ensure step direction aligns with relation between left and right */
		const int expected_direction = (scaffold->left <= scaffold->right) ? +1 : -1;
		const int step_direction = (scaffold->step >= 0) ? +1 : -1;
		if (expected_direction != step_direction) {
			scaffold->step = -scaffold->step;
		}
	}
}

float discrete_random_closed(float min, float max, float step_width) {
	const float distance = fabs(max - min) + 1;
	double zero_to_almost_one = 0;
	double zero_to_almost_distance;
	unsigned int depth;
	int unsigned u;

	assert(min <= max);
	assert(step_width > 0);

	/* Note: Using log(a)+log(b) for log(a*b) to push down overflow border */
	depth = ceil((log(distance) + log(step_width)) / log(RAND_MAX));

	/* Make random resolution at least on par with step_width */
	/* z = (r * RAND_MAX^0 + .. + r * RAND_MAX^(n-1)) / RAND_MAX^n */
	for (u = 0; u < depth; u++) {
		zero_to_almost_one += rand() * pow(RAND_MAX, u);
	}
	zero_to_almost_one /= pow(RAND_MAX, depth);

	zero_to_almost_distance = zero_to_almost_one * distance;
	return min + zero_to_almost_distance - fmod(zero_to_almost_distance, step_width);
}

yield_status yield(scaffolding * scaffold, float * dest) {
	float candidate;

	assert(CHECK_FLAG(scaffold->flags, FLAG_READY));

	if (CHECK_FLAG(scaffold->flags, FLAG_RANDOM)) {
		if (HAS_COUNT(scaffold) && (scaffold->position >= scaffold->count)) {
			*dest = 0.123456f;  /* Arbitrary magic value */
			return YIELD_NONE;
		}

		if (HAS_RIGHT(scaffold)) {
			if (((scaffold->left <= scaffold->right)
						&& (scaffold->left + scaffold->step > scaffold->right))
					|| ((scaffold->left >= scaffold->right)
						&& (scaffold->left + scaffold->step < scaffold->right))) {
				*dest = 0.123456f;  /* Arbitrary magic value */
				return YIELD_NONE;
			}

			*dest = discrete_random_closed(
				ENUM_MIN(scaffold->left, scaffold->right),
				ENUM_MAX(scaffold->left, scaffold->right),
				fabs(scaffold->step));
			scaffold->position++;
			assert(HAS_COUNT(scaffold));
			if (scaffold->position == scaffold->count) {
				return YIELD_LAST;
			} else {
				return YIELD_MORE;
			}
		} else {
			const float min = (scaffold->step >= 0) ? scaffold->left : -FLT_MAX;
			const float max = (scaffold->step >= 0) ? FLT_MAX : scaffold->left;
			*dest = discrete_random_closed(
				min,
				max,
				fabs(scaffold->step));
			return YIELD_MORE;
		}
	}

	assert(HAS_LEFT(scaffold) && HAS_STEP(scaffold));
	assert(! HAS_COUNT(scaffold) || (HAS_COUNT(scaffold) && (scaffold->count > 0)));

	/* Gone too far already? */
	if (HAS_COUNT(scaffold) && (scaffold->position >= scaffold->count)) {
		*dest = 0.123456f;  /* Arbitrary magic value */
		return YIELD_NONE;
	}

	candidate = scaffold->left + scaffold->step * scaffold->position;
	/* TODO check for float overflow, float imprecision */

	/* Gone too far now? */
	if (HAS_RIGHT(scaffold)
			&& (((scaffold->left <= scaffold->right) && (candidate > scaffold->right))
				|| ((scaffold->left >= scaffold->right) && (candidate < scaffold->right)))) {
		*dest = 0.123456f;  /* Arbitrary magic value */
		return YIELD_NONE;
	}

	*dest = candidate;
	scaffold->position++;

	/* One value only? */
	if (HAS_COUNT(scaffold) && (scaffold->count == 1)) {
		return YIELD_LAST;
	}

	/* Will there be more? */
	if ((HAS_COUNT(scaffold) && (scaffold->position == scaffold->count))
			|| (HAS_RIGHT(scaffold) && (*dest == scaffold->right))) {
		return YIELD_LAST;
	} else {
		return YIELD_MORE;
	}
}

void initialize_scaffold(scaffolding * dest) {
	dest->flags = 0;
	dest->position = 0;
	dest->precision = 0;
}
