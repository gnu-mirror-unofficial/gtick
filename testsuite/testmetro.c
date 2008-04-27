/*
 * testmetro.c: Unit Tests for metro.c
 *
 * This file is part of GTick
 *
 *
 * Copyright (c) 2008 Roland Stigge <stigge@antcom.de>
 *
 * GTick is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GTick is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GTick; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <string.h>

/* Unit Test common code */
#include "common.h"

/* Include from code under test */
#include "metro.h"
#include "dsp.h"

static dsp_t* dsp = NULL;

/* TODO: Adjust setup/teardown */
void setup_meter(void) {
	dsp = (dsp_t*)malloc(sizeof(dsp_t));
	assert(dsp != NULL);

	memset(dsp, 0x47, sizeof(dsp_t));
}

void teardown_meter(void) {
	free(dsp);
	dsp = NULL;
}

/*
 * Test external set_meter()
 */
START_TEST(test__set_meter__0) {
	RESOURCE_GUARD_START();
	fail_unless(set_meter(NULL, NULL, NULL) == -1,
			"Error expected on set_meter(NULL, NULL, NULL)");
	RESOURCE_GUARD_END();
}
END_TEST

Suite *test_suite(void) {
	Suite *s = suite_create("Metro");
	TCase *tc_extern = tcase_create("Extern Functions");

	tcase_add_checked_fixture(tc_extern, setup_meter, teardown_meter);
	tcase_add_test(tc_extern, test__set_meter__0);
	suite_add_tcase(s, tc_extern);
	
	return s;
}

int main(int argc __attribute((unused)), char* argv[] __attribute((unused))) {
	return test_suite_run(test_suite());
}

