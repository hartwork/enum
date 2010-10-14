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
#include "../src/assertion.h"

#include <stdio.h>
#include <math.h> /* for fabs */

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

#define TEST_CASE_INDENT "  "

const unsigned int XX = 999;

void pseudo_call(float left, unsigned int count, float step, float right) {
	printf("# enum  ");

	if (left != XX) {
		printf("%.1f .. ", left);
	} else {
		printf(".. ");
	}

	if (count != XX) {
		if (step != XX) {
			printf("%dx %.1f .. ", count, step);
		} else {
			printf("%dx .. ", count);
		}
	} else if (step != XX) {
		printf("%.1f .. ", step);
	}

	if (right != XX) {
		printf("%.1f", right);
	}

	puts("");
}

int towards_infinity(scaffolding const * scaffold) {
	return ((scaffold->flags & (FLAG_RIGHT_SET | FLAG_COUNT_SET)) != (FLAG_RIGHT_SET | FLAG_COUNT_SET));
}

int test_yield(float left, unsigned int count, float step, float right, const float * expected, unsigned int exp_len) {
	scaffolding scaffold;
	float dest;
	unsigned int i;
	int ret = 1;

	pseudo_call(left, count, step, right);

	initialize_scaffold(&scaffold);

	if (left != XX) {
		SET_LEFT(scaffold, left);
	}

	if (count != XX) {
		SET_COUNT(scaffold, count);
	}

	if (step != XX) {
		SET_STEP(scaffold, step);
		scaffold.flags |= FLAG_USER_STEP;
	}

	if (right != XX) {
		SET_RIGHT(scaffold, right);
	}

	complete_scaffold(&scaffold);

	puts(TEST_CASE_INDENT "    Received  Expected");
	puts(TEST_CASE_INDENT "----------------------");
	for (i = 0; i < exp_len; i++) {
		yield_status status = yield(&scaffold, &dest);

		printf(TEST_CASE_INDENT "%2d) %8.1f  %8.1f\n", i + 1, dest, expected[i]);

		if (status == YIELD_LAST) {
			if (i < (exp_len - 1)) {
				puts(TEST_CASE_INDENT "FAILURE (last too early, generator?)");
				ret = 0;
			}

			if (i == (exp_len - 1)) {
				if (towards_infinity(&scaffold)) {
					puts(TEST_CASE_INDENT "FAILURE (last despite infinity, generator?)");
					ret = 0;
				}
			}
		}

		if (status == YIELD_MORE) {
			if (i == (exp_len - 1)) {
				if (! towards_infinity(&scaffold)) {
					puts(TEST_CASE_INDENT "FAILURE (more instead of last, generator?)");
					ret = 0;
				}
			}
		}

		if (fabs(expected[i] - dest) > FLOAT_EQUAL_DELTA) {
			puts(TEST_CASE_INDENT "FAILURE (value mismatch)");
			ret = 0;
		}
	}

	if (! towards_infinity(&scaffold)) {
		if (scaffold.count != exp_len) {
			printf(TEST_CASE_INDENT "FAILURE (count miscalculated, expected: %d, calculated: %d)\n", exp_len, scaffold.count);
			ret = 0;
		}
	}

	if (ret) {
		puts(TEST_CASE_INDENT "Success\n");
	} else {
		puts("");
	}
	return ret;
}

int main() {
	unsigned int successes = 0;
	unsigned int failures = 0;

	TEST_CASE(successes, failures,  2,  4,  3, 11, ARRAY(2, 5, 8, 11))

	TEST_CASE(successes, failures, XX,  4,  3, 11, ARRAY(2, 5, 8, 11))
	TEST_CASE(successes, failures,  2, XX,  3, 11, ARRAY(2, 5, 8, 11))
	TEST_CASE(successes, failures,  2,  4, XX, 11, ARRAY(2, 5, 8, 11))
	TEST_CASE(successes, failures,  2,  4,  3, XX, ARRAY(2, 5, 8, 11))

	TEST_CASE(successes, failures, XX, XX,  3, 11, ARRAY(2, 5, 8, 11))
	TEST_CASE(successes, failures, XX,  4, XX, 11, ARRAY(8, 9, 10, 11))
	TEST_CASE(successes, failures, XX,  4,  3, XX, ARRAY(1, 4, 7, 10))
	TEST_CASE(successes, failures,  2, XX, XX, 11, ARRAY(2, 3, 4, 5, 6, 7, 8, 9, 10, 11))
	TEST_CASE(successes, failures,  2, XX,  3, XX, ARRAY(2, 5, 8, 11, 14, 17)) /* .. and more */
	TEST_CASE(successes, failures,  2,  4, XX, XX, ARRAY(2, 3, 4, 5))

	TEST_CASE(successes, failures,  2, XX, XX, XX, ARRAY(2, 3, 4, 5, 6, 7)) /* .. and more */
	TEST_CASE(successes, failures, XX,  4, XX, XX, ARRAY(1, 2, 3, 4))
	TEST_CASE(successes, failures, XX, XX,  3, XX, ARRAY(1, 4, 7, 10, 13, 16)) /* .. and more */
	TEST_CASE(successes, failures, XX, XX, XX, 11, ARRAY(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11))


	/* left > right */
	TEST_CASE(successes, failures,  11,  4,  3, 2, ARRAY(11, 8, 5, 2))
	TEST_CASE(successes, failures,  11,  4, -3, 2, ARRAY(11, 8, 5, 2))
	TEST_CASE(successes, failures,  11, XX,  3, 2, ARRAY(11, 8, 5, 2))
	TEST_CASE(successes, failures,  11, XX, -3, 2, ARRAY(11, 8, 5, 2))
	TEST_CASE(successes, failures,  11,  4, XX, 2, ARRAY(11, 8, 5, 2))
	TEST_CASE(successes, failures,  11, XX, XX, 2, ARRAY(11, 10, 9, 8, 7, 6, 5, 4, 3, 2))


	/* post dot digit precision */
	TEST_CASE(successes, failures, 0.3, XX, 0.1, 0.7, ARRAY(0.3, 0.4, 0.5, 0.6, 0.7))
	TEST_CASE(successes, failures, 0.7, XX, -0.1, 0.3, ARRAY(0.7, 0.6, 0.5, 0.4, 0.3))


	/* calculation of count */
	TEST_CASE(successes, failures, 1, XX, 2, 4, ARRAY(1, 3))
	TEST_CASE(successes, failures, 1, XX, 2, 5, ARRAY(1, 3, 5))


	assert(successes + failures > 0);
	printf(
		"Successes:   %2u  (%6.2f%%)\n"
		"Failures:    %2u  (%6.2f%%)\n"
		"--------------------------\n"
		"Test cases:  %2u\n"
		"\n",
		successes,
		(float)successes * 100 / (successes + failures),
		failures,
		(float)failures * 100 / (successes + failures),
		(successes + failures)
	);
	return failures;
}
