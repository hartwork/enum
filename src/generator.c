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

#define CHECK_FLAG(bitfield, flag)  (((bitfield) & (flag)) == (flag))

/**
 * return values:
 * 0: value calculated, more available
 * 1: value calculated, no more available
 * 2: nothing could be calculated
 */
int yield(arguments * args, float * dest) {
	if (CHECK_FLAG(args->flags, FLAG_READY)) {
		/* TODO make ready */
	}

	if (CHECK_FLAG(args->flags, FLAG_COUNT_SET)
			&& (args->count == 1)) {
		/* TODO return first and only value */
		return 1;
	}

	if (! CHECK_FLAG(args->flags, FLAG_STEP_SET)) {
		if (CHECK_FLAG(args->flags, FLAG_MIN_SET)
				&& CHECK_FLAG(args->flags, FLAG_MAX_SET)
				&& CHECK_FLAG(args->flags, FLAG_COUNT_SET)) {
			assert(args->count != 0);
			args->step_num = (args->max - args->min);
			args->step_denom = (args->count - 1);
		} else {
			args->step_num = 1.0f;
			args->step_denom = 1.0f;
		}
		args->flags |= FLAG_STEP_SET;
	}
	assert(CHECK_FLAG(args->flags, FLAG_STEP_SET));
	return 0;
}
