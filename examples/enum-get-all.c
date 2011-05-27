#include <stdio.h>
#include <stdlib.h>
#include <enum.h>

void my_print(float out) {
	printf("%.0f\n", out);
}

/* This example equals `enum 1 5' or `seq 1 5' */
int main(int argc, char **argv) {
	scaffolding sc;

	enum_initialize_scaffold(&sc);
	SET_LEFT(sc, 1);
	SET_RIGHT(sc, 5);
	enum_complete_scaffold(&sc);
	if (! ENUM_IS_READY(&sc))
		exit(1);

	enum_get_all(&sc, &my_print);

	exit(0);
}
