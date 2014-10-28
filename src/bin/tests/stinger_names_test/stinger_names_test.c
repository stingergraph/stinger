/*
 * stinger_names_test.c
 *
 *  Created on: Oct 28, 2014
 *      Author: jason
 */

#include "stinger_names_test.h"

int
main (const int argc, char *argv[])
{
	stinger_names_t * names = stinger_names_new(5);

	stinger_names_init(names,5);

	char * test_names [] = {
			"Alpha",
			"Beta",
			"Gamma",
			"Sigma",
			"Zeta",
			"Tau",
			"Pi"
	};

	int64_t out = 0;
	int64_t status = 0;

	for (int j=0; j < 3; j++) {
		for (int i=0; i < 7; i++) {
			out = -1;
			status = 0;
			status = stinger_names_create_type(names, test_names[i], &out);
			printf("CREATE:\n\tString: %s\n\tStatus: %lld\n\tIndex: %lld\n", test_names[i], status, out);

			out = stinger_names_lookup_type(names, test_names[i]);
			printf("LOOKUP:\n\tString: %s\n\tIndex: %lld\n", test_names[i], out);
		}
	}
}



