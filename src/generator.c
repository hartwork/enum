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

#define CHECK_FLAG(bitfield, flag)  (((bitfield) & (flag)) == (flag))

#define HAS_LEFT(args)  CHECK_FLAG(args->flags, FLAG_LEFT_SET)
#define HAS_RIGHT(args)  CHECK_FLAG(args->flags, FLAG_RIGHT_SET)
#define HAS_STEP(args)  CHECK_FLAG(args->flags, FLAG_STEP_SET)
#define HAS_COUNT(args)  CHECK_FLAG(args->flags, FLAG_COUNT_SET)

void calculator(arguments * args) {
	/* we need a step, no matter what */
	if (! HAS_STEP(args)) {
		if (HAS_LEFT(args) && HAS_RIGHT(args) && HAS_COUNT(args)) {
			assert(args->count != 0);
			SET_STEP(*args, (args->right - args->left), (args->count - 1));
		} else {
			SET_STEP(*args, 1.0f, 1.0f);
		}
	}
	assert(HAS_STEP(args));

	/* and we need left */
	if (! HAS_LEFT(args)) {
		if (HAS_RIGHT(args) && HAS_COUNT(args)) {
			/* count and right set */
			SET_LEFT(*args, (args->right - ((args->count - 1) * args->step_num / args->step_denom)));
		} else if (HAS_RIGHT(args)) {
			/* right but no count */
			SET_LEFT(*args, ((int)((args->right - args->left) / (args->step_num / args->step_denom)) + 1));
		} else {
			/* no right, count may be there */
			SET_LEFT(*args, 1.0f);
		}
	}
	assert(HAS_LEFT(args));

	/* and a count */
	if (! HAS_COUNT(args)) {
		if (HAS_RIGHT(args)) {
			/* right is set */
			SET_COUNT(*args, (args->right - args->left) * args->step_denom / args->step_num + 1);
		} else {
			/* no right -> INFINITY */
			/* TODO INFINITY */
			SET_COUNT(*args, strtod("INF", NULL));
		}
	}
	assert(HAS_COUNT(args));

	if (! HAS_RIGHT(args)) {
		/* TODO INFINITY */
		/* TODO possibly negative INFINITY in reverted mode */
		SET_RIGHT(*args, strtod("INF", NULL));
	}
	assert(HAS_RIGHT(args));

	args->flags |= FLAG_READY;
}

yield_status yield(arguments * args, float * dest) {
	if (! CHECK_FLAG(args->flags, FLAG_READY)) {
		calculator(args);
	}

	if (args->count == 1) {
		/* TODO return first and only value */
		return YIELD_LAST;
	}

	*dest = args->left + (args->step_num / args->step_denom) * args->position;
	/* TODO check for float overflow, float imprecision */

	if (args->position >= (args->count - 1)) {
		return YIELD_LAST;
	}

	if (args->right == *dest)
		return YIELD_LAST;

	if (args->right < *dest)
		return YIELD_NONE;

	args->position++;
	return YIELD_MORE;
}
