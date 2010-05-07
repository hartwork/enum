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
#include <assert.h>
#include <stdlib.h>
#include <math.h>  /* for fabs(double) */

#define CHECK_FLAG(bitfield, flag)  (((bitfield) & (flag)) == (flag))

#define ENUM_FLOOR(f)  ((float)(int)(f))

void calculator(arguments * args) {
	/* we need a step, no matter what */
	if (! CHECK_FLAG(args->flags, FLAG_STEP_SET)) {
		if (CHECK_FLAG(args->flags, FLAG_LEFT_SET)
				&& CHECK_FLAG(args->flags, FLAG_RIGHT_SET)
				&& CHECK_FLAG(args->flags, FLAG_COUNT_SET)) {
			assert(args->count != 0);
			args->step_num = (args->right - args->left);
			args->step_denom = (args->count - 1);
		} else {
			args->step_num = 1.0f;
			args->step_denom = 1.0f;
		}
		args->flags |= FLAG_STEP_SET;
	}
	assert(CHECK_FLAG(args->flags, FLAG_STEP_SET));

	/* and we need left */
	if (! CHECK_FLAG(args->flags, FLAG_LEFT_SET)) {
		if (CHECK_FLAG(args->flags, FLAG_RIGHT_SET) &&
				CHECK_FLAG(args->flags, FLAG_COUNT_SET)) {
			/* count and right set */
			args->left = args->right - ((args->count - 1) * args->step_num / args->step_denom);
		} else if (CHECK_FLAG(args->flags, FLAG_RIGHT_SET)) {
			/* right but no count -> use right as left */
			/* TODO this is reverted mode! */
			/* TODO this doesn't work for shortcut 'enum 10' */
			args->left = args->right;
			args->flags &= ~FLAG_RIGHT_SET;
		} else {
			/* no right, count may be there */
			args->left = 1.0f;
		}
		args->flags |= FLAG_LEFT_SET;
	}
	assert(CHECK_FLAG(args->flags, FLAG_LEFT_SET));

	/* and a count */
	if (! CHECK_FLAG(args->flags, FLAG_COUNT_SET)) {
		if (CHECK_FLAG(args->flags, FLAG_RIGHT_SET)) {
			/* right is set */
			args->count = (args->right - args->left) * args->step_denom / args->step_num + 1;
		} else {
			/* no right -> INFINITY */
			/* TODO INFINITY */
			args->count = strtod("INF", NULL);
		}
		args->flags |= FLAG_COUNT_SET;
	}
	assert(CHECK_FLAG(args->flags, FLAG_COUNT_SET));

	if (! CHECK_FLAG(args->flags, FLAG_RIGHT_SET)) {
		/* TODO INFINITY */
		args->count = strtod("INF", NULL);
		/* TODO possibly negative INFINITY in reverted mode */
		args->flags |= FLAG_RIGHT_SET;
	}
	assert(CHECK_FLAG(args->flags, FLAG_RIGHT_SET));

	args->flags |= FLAG_READY;
}


#define HAS_LEFT(args)  CHECK_FLAG(args->flags, FLAG_LEFT_SET)
#define HAS_RIGHT(args)  CHECK_FLAG(args->flags, FLAG_RIGHT_SET)
#define HAS_STEP(args)  CHECK_FLAG(args->flags, FLAG_STEP_SET)
#define HAS_COUNT(args)  CHECK_FLAG(args->flags, FLAG_COUNT_SET)


void complete_args(arguments * args) {
	assert(KNOWN(args) >= 0);

	if (KNOWN(args) == 1) {
		if (! HAS_LEFT(args)) {
			SET_LEFT(*args, 1.0f);
		} else {
			SET_STEP(*args, 1.0f, 1.0f);
		}
		assert(KNOWN(args) == 2);
	}

	assert(KNOWN(args) >= 2);
	if (KNOWN(args) == 2) {
		if (HAS_LEFT(args) && HAS_STEP(args)) {
			/* running to infinity */
			args->flags |= FLAG_READY;
			assert(! HAS_COUNT(args) && ! HAS_RIGHT(args));
			assert(KNOWN(args) == 2);
			return;
		}

		/* NOTE: Step has higher precedence */
		if (! HAS_STEP(args)) {
			SET_STEP(*args, 1.0f, 1.0f);
		} else {
			if (HAS_RIGHT(args)) {
				assert(HAS_STEP(args));
				SET_LEFT(*args, args->right
					- (args->step_num / args->step_denom) * ENUM_FLOOR(
					args->right * args->step_denom / args->step_num));
			} else {
				SET_LEFT(*args, 1.0f);
			}
		}
		assert(KNOWN(args) == 3);
	}

	assert(KNOWN(args) >= 3);
	if (KNOWN(args) == 3) {
		if (! HAS_LEFT(args)) {
			SET_LEFT(*args, args->right - (args->count - 1)
				* (args->step_num / args->step_denom));
		} else if (! HAS_COUNT(args)) {
			SET_COUNT(*args, fabs(args->right - args->left)
				/ (args->step_num / args->step_denom) + 1);
		} else if (! HAS_STEP(args)) {
			SET_STEP(*args, args->right - args->left, args->count - 1);
		} else {
			assert(! HAS_RIGHT(args));
			SET_RIGHT(*args, args->left + (args->step_num / args->step_denom)
				* (args->count - 1));
		}
	}

	args->flags |= FLAG_READY;
	assert(KNOWN(args) == 4);
}

yield_status yield(arguments * args, float * dest) {
	assert(CHECK_FLAG(args->flags, FLAG_READY));
	assert(HAS_LEFT(args) && HAS_STEP(args));

	/* Gone too far already? */
	if (HAS_COUNT(args) && (args->position >= args->count)) {
		*dest = 0.123456f;  /* Arbitrary magic value */
		return YIELD_NONE;
	}

	/* One value only? */
	if (HAS_COUNT(args) && (args->count == 1)) {
		*dest = args->left;
		return YIELD_LAST;
	} else {
		*dest = args->left + (args->step_num / args->step_denom) * args->position;
		/* TODO check for float overflow, float imprecision */
	}

	/* Gone too far now? */
	if (HAS_RIGHT(args) && (*dest > args->right)) {
		*dest = 0.123456f;  /* Arbitrary magic value */
		return YIELD_NONE;
	}

	/* Will there be more? */
	if ((HAS_COUNT(args) && (args->position == args->count - 1))
			|| (HAS_RIGHT(args) && (*dest == args->right))) {
		args->position++;
		return YIELD_LAST;
	} else {
		args->position++;
		return YIELD_MORE;
	}
}

void initialize_args(arguments * dest) {
	dest->flags = 0;
	dest->position = 0;
}
