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

#ifndef GENERATOR_H
#define GENERATOR_H 1

#define SET_LEFT(args, _left)  \
	args->left = _left; \
	args->flags |= FLAG_LEFT_SET

#define SET_RIGHT(args, _right)  \
	args->right = _right; \
	args->flags |= FLAG_RIGHT_SET

#define SET_STEP(args, num, denom)  \
	args->step_num = num; \
	args->step_denom = denom; \
	args->flags |= FLAG_STEP_SET

#define SET_COUNT(args, _count)  \
	args->count = _count; \
	args->flags |= FLAG_COUNT_SET

#define KNOWN(args)  (HAS_LEFT(args) + HAS_RIGHT(args) \
	+ HAS_STEP(args) + HAS_COUNT(args))

enum argument_flags {
	FLAG_LEFT_SET = 1 << 0,
	FLAG_RIGHT_SET = 1 << 1,
	FLAG_STEP_SET = 1 << 2,
	FLAG_COUNT_SET = 1 << 3,

	FLAG_READY = 1 << 4
};

typedef enum _yield_status {
	YIELD_MORE, /* value calculated, more available */
	YIELD_LAST, /* value calculated, no more available */
	YIELD_NONE  /* nothing could be calculated */
} yield_status;

typedef struct _arguments {
	int flags;
	float left;
	float right;
	float step_num;
	float step_denom;
	unsigned int count;
	unsigned int position;
} arguments;

void complete_args(arguments * args);
yield_status yield(arguments * args, float * dest);
void initialize_args(arguments * dest);

#endif /* GENERATOR_H */
