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
	test_harness_t * th = new_test_harness(16);

	add_test_description(th, 0, "Lookup Before Creation");
	add_expected_value_int(th, 0, -1);

	add_test_description(th, 1, "Alpha create");
	add_expected_value_int(th, 1, 0);
	add_test_description(th, 2, "Beta create");
	add_expected_value_int(th, 2, 1);
	add_test_description(th, 3, "Gamma create");
	add_expected_value_int(th, 3, 2);
	add_test_description(th, 4, "Sigma create");
	add_expected_value_int(th, 4, 3);
	add_test_description(th, 5, "Zeta create");
	add_expected_value_int(th, 5, 4);
	add_test_description(th, 6, "Tau create");
	add_expected_value_int(th, 6, 5);
	add_test_description(th, 7, "Pi create");
	add_expected_value_int(th, 7, -1);

	add_test_description(th, 8, "Alpha lookup");
	add_expected_value_int(th, 8, 0);
	add_test_description(th, 9, "Beta lookup");
	add_expected_value_int(th, 9, 1);
	add_test_description(th, 10, "Gamma lookup");
	add_expected_value_int(th, 10, 2);
	add_test_description(th, 11, "Sigma lookup");
	add_expected_value_int(th, 11, 3);
	add_test_description(th, 12, "Zeta lookup");
	add_expected_value_int(th, 12, 4);
	add_test_description(th, 13, "Tau lookup");
	add_expected_value_int(th, 13, 5);
	add_test_description(th, 14, "Pi lookup");
	add_expected_value_int(th, 14, -1);

	add_test_description(th, 15, "Alpha Create Again");
	add_expected_value_int(th, 15, -1);

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

	int64_t cur_test = 0;

	out = stinger_names_lookup_type(names, test_names[0]);
	test_expected_value_int(th, cur_test++, out);

	for (int i=0; i < 7; i++) {
		out = -1;
		status = 0;
		status = stinger_names_create_type(names, test_names[i], &out);
		test_expected_value_int(th, cur_test++, out);
	}

	for (int i=0; i < 7; i++) {
		out = -1;
		out = stinger_names_lookup_type(names, test_names[i]);
		test_expected_value_int(th, cur_test++, out);
	}

	status = stinger_names_create_type(names, test_names[0], &out);
	test_expected_value_int(th, cur_test++, out);

	fini_test_harness(th);
}



