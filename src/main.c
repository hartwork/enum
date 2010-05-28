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

#include "parsing.h"
#include "generator.h"
#include "assertion.h"
#include "printing.h"
#include "utils.h"

#include <stdlib.h>  /* for srand */
#include <time.h>  /* for time */

int main(int argc, char **argv) {
	int argpos;
	scaffolding dest;
	float out;
	int ret;
	int i = 0;

	initialize_scaffold(&dest);
	dest.flags |= FLAG_NEWLINE;

	dest.separator = enum_strdup("\n");

	argpos = parse_parameters(argc, argv, &dest);
	switch (argpos) {
	case 0:
		/* usage or version shown, no numbers wanted */
		exit(0);
	case -1:
		/* errors reported already */
		exit(1);
	default:
		/* normal run with numbers */
		break;
	}

	if (! parse_args(argc - argpos, argv + argpos, &dest)) {
		fprintf(stderr, "Command line parsing error\n");
		return 1;
	}

	complete_scaffold(&dest);

	if (!dest.format) {
		assert(dest.precision < 100);
		dest.format = malloc(6); /* TODO */
		sprintf(dest.format, "%%.%uf", dest.precision);
	}

	if (CHECK_FLAG(dest.flags, FLAG_RANDOM))
		srand(time(NULL));

	while (1) {
		ret = yield(&dest, &out);

		if (i != 0)
			printf("%s", dest.separator);

		if (ret != YIELD_NONE)
			multi_printf(dest.format, out);

		if (ret != YIELD_MORE)
			break;

		i++;
	}

	if (CHECK_FLAG(dest.flags, FLAG_NEWLINE))
		printf("\n");

	free(dest.format);
	free(dest.separator);

	return 0;
}
