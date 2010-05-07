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

#include "../src/generator.h"

#include <stdio.h>
#include <assert.h>

#define ARRAY(numbers...)  { numbers }

#define TEST_YIELD(left, count, step, right, expected) \
	test_yield(left, count, step, right, expected, (sizeof(expected) / sizeof(float)))

#define TEST_CASE(successes, failures, left, count, step, right, expected_data) \
	{ \
		const float expected[] = expected_data; \
		if (TEST_YIELD(left, count, step, right, expected)) { \
			successes++; \
		} else { \
			failures++; \
		} \
	}

const int NO_VALUE = 999;

void pseudo_call(float left, unsigned int count, float step, float right) {
	printf("# enum  ");

	if (left != NO_VALUE) {
		printf("%.1f .. ", left);
	} else {
		printf(".. ");
	}

	if (count != NO_VALUE) {
		if (step != NO_VALUE) {
			printf("%dx %.1f .. ", count, step);
		} else {
			printf("%dx .. ", count);
		}
	} else if (step != NO_VALUE) {
		printf("%.1f .. ", step);
	}

	if (right != NO_VALUE) {
		printf("%.1f", right);
	}

	puts("");
}

int test_yield(float left, unsigned int count, float step, float right, const float * expected, unsigned int exp_len) {
	arguments args;
	float dest;
	int i;

	pseudo_call(left, count, step, right);

	initialize_args(&args);

	if (left != NO_VALUE) {
		SET_LEFT(args, left);
	}

	if (count != NO_VALUE) {
		SET_COUNT(args, count);
	}

	if (step != NO_VALUE) {
		SET_STEP(args, step, 1);
	}

	if (right != NO_VALUE) {
		SET_RIGHT(args, right);
	}

	complete_args(&args);

	puts("  received, expected");
	for (i = 0; i < exp_len; i++) {
		if (yield(&args, &dest) == YIELD_NONE) {
			puts("  failure\n");
			return 0;
		}
		printf("  %f, %f\n", dest, expected[i]);
		if (expected[i] != dest) {
			puts("  failure\n");
			return 0;
		}
	}
	puts("  success\n");
	return 1;
}

int main() {
	unsigned int successes = 0;
	unsigned int failures = 0;

	TEST_CASE(successes, failures, 2, 4, 3, 11, ARRAY(2, 5, 8, 11))
	TEST_CASE(successes, failures, NO_VALUE, 4, 3, 11, ARRAY(2, 5, 8, 11))
	TEST_CASE(successes, failures, 2, NO_VALUE, 3, 11, ARRAY(2, 5, 8, 11))
	TEST_CASE(successes, failures, 2, 4, NO_VALUE, 11, ARRAY(2, 5, 8, 11))
	TEST_CASE(successes, failures, 2, 4, 3, NO_VALUE, ARRAY(2, 5, 8, 11))

	TEST_CASE(successes, failures, NO_VALUE, NO_VALUE, 3, 11, ARRAY(2, 5, 8, 11))
	TEST_CASE(successes, failures, NO_VALUE, 4, NO_VALUE, 11, ARRAY(8, 9, 10, 11))
	TEST_CASE(successes, failures, NO_VALUE, 4, 3, NO_VALUE, ARRAY(1, 4, 7, 10))
	TEST_CASE(successes, failures, 2, NO_VALUE, NO_VALUE, 11, ARRAY(2, 3, 4, 5, 6, 7, 8, 9, 10, 11))
	TEST_CASE(successes, failures, 2, NO_VALUE, 3, NO_VALUE, ARRAY(2, 5, 8, 11, 14, 17))
	TEST_CASE(successes, failures, 2, 4, NO_VALUE, NO_VALUE, ARRAY(2, 3, 4, 5))

	TEST_CASE(successes, failures, 2, NO_VALUE, NO_VALUE, NO_VALUE, ARRAY(2, 3, 4, 5, 6, 7, 8, 9, 10, 11))
	TEST_CASE(successes, failures, NO_VALUE, 4, NO_VALUE, NO_VALUE, ARRAY(1, 2, 3, 4))
	TEST_CASE(successes, failures, NO_VALUE, NO_VALUE, 3, NO_VALUE, ARRAY(1, 4, 7, 10, 13, 16))
	TEST_CASE(successes, failures, NO_VALUE, NO_VALUE, NO_VALUE, 11, ARRAY(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11))

	assert(successes + failures > 0);
	printf(
		"Successes:   %u (%.2f%%)\n"
		"Failures:    %u (%.2f%%)\n"
		"-------------------------\n"
		"Test cases:  %u\n"
		"\n",
		successes,
		(float)successes * 100 / (successes + failures),
		failures,
		(float)failures * 100 / (successes + failures),
		(successes + failures)
	);
	return failures;
}
