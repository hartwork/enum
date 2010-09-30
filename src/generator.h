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

#define MAX_PD_DIGITS  5
#define FLOAT_EQUAL_DELTA  0.0001f

#define CHECK_FLAG(bitfield, flag)  (((bitfield) & (flag)) == (flag))

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

#define INCREASE_PRECISION(scaffold, _precision)  \
	if (_precision > (scaffold).precision) { \
		(scaffold).precision = _precision; \
	}

#define HAS_LEFT(scaffold)  CHECK_FLAG(scaffold->flags, FLAG_LEFT_SET)
#define HAS_RIGHT(scaffold)  CHECK_FLAG(scaffold->flags, FLAG_RIGHT_SET)
#define HAS_STEP(scaffold)  CHECK_FLAG(scaffold->flags, FLAG_STEP_SET)
#define HAS_COUNT(scaffold)  CHECK_FLAG(scaffold->flags, FLAG_COUNT_SET)

#define KNOWN(scaffold)  (HAS_LEFT(scaffold) + HAS_RIGHT(scaffold) \
	+ HAS_STEP(scaffold) + HAS_COUNT(scaffold))

enum scaffolding_flags {
	FLAG_LEFT_SET = 1 << 0,
	FLAG_RIGHT_SET = 1 << 1,
	FLAG_STEP_SET = 1 << 2,
	FLAG_COUNT_SET = 1 << 3,

	FLAG_READY = 1 << 4,

	FLAG_RANDOM = 1 << 5,
	FLAG_USER_STEP = 1 << 6,
	FLAG_NEWLINE = 1 << 7
};

typedef enum _yield_status {
	YIELD_MORE, /* value calculated, more available */
	YIELD_LAST, /* value calculated, no more available */
	YIELD_NONE  /* nothing could be calculated */
} yield_status;

typedef struct _scaffolding {
	int flags;
	float left;
	float right;
	float step;
	unsigned int count;
	unsigned int position;
	unsigned int precision;
	char * format;
	char * separator;
} scaffolding;

void complete_scaffold(scaffolding * scaffold);
yield_status yield(scaffolding * scaffold, float * dest);
void initialize_scaffold(scaffolding * dest);

#endif /* GENERATOR_H */
