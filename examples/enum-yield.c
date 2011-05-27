#include <stdio.h>
#include <stdlib.h>
#include <enum.h>

/* This example equals `enum 1 5' or `seq 1 5' */
int main(int argc, char **argv) {
	scaffolding sc;
	yield_status s;
	float out;

	enum_initialize_scaffold(&sc);
	SET_LEFT(sc, 1);
	SET_RIGHT(sc, 5);
	if (! enum_complete_scaffold(&sc))
		exit(1);

	while(1) {
		s = enum_yield(&sc, &out);
		printf("%.0f\n", out);
		if (s != YIELD_MORE)
			break;
	}

	exit(0);
}
