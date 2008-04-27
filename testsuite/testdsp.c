/*
 * testdsp.c: Unit Tests for dsp.c
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
#include "dsp.h"

static dsp_t* dsp = NULL;

void setup_dsp(void) {
	dsp = (dsp_t*)malloc(sizeof(dsp_t));
	assert(dsp != NULL);

	memset(dsp, 0x47, sizeof(dsp_t));
}

void teardown_dsp(void) {
	free(dsp);
	dsp = NULL;
}

/*
 * Test external dsp_get_volume()
 */
START_TEST(test__dsp_get_volume__0) {
	RESOURCE_GUARD_START();
	dsp->volume = 0.0;
	fail_unless(epsilon_cmp(dsp_get_volume(dsp), 0.123) == 0,
			"Error: Bad volume returned!");
	fail_unless(epsilon_cmp(dsp_get_volume(dsp), 0.0),
			"Error: Bad volume returned!");
	RESOURCE_GUARD_END();
}
END_TEST

/*
 * Test external dsp_get_volume()
 */
START_TEST(test__dsp_get_volume__0123) {
	RESOURCE_GUARD_START();
	dsp->volume = 0.123;
	fail_unless(epsilon_cmp(dsp_get_volume(dsp), 0.0) == 0,
			"Error: Bad volume returned!");
	fail_unless(epsilon_cmp(dsp_get_volume(dsp), 0.123),
			"Error: Bad volume returned!");
	RESOURCE_GUARD_END();
}
END_TEST

/*
 * Test external dsp_set_volume()
 */
START_TEST(test__dsp_set_volume__0) {
	RESOURCE_GUARD_START();
	dsp->running = 0; // TODO: only an init workaround
	dsp_set_volume(dsp, 0.0);
	fail_unless(epsilon_cmp(dsp->volume, 0.0),
			"Error: Bad volume set (%g)!", dsp->volume);
	RESOURCE_GUARD_END();
}
END_TEST

/*
 * Test external dsp_set_volume()
 */
START_TEST(test__dsp_set_volume__0765) {
	RESOURCE_GUARD_START();
	dsp->running = 0; // TODO: only an init workaround
	dsp_set_volume(dsp, 0.765);
	fail_unless(epsilon_cmp(dsp->volume, 0.765),
			"Error: Bad volume set (%g)!", dsp->volume);
	RESOURCE_GUARD_END();
}
END_TEST

Suite *test_suite(void) {
	Suite *s = suite_create("DSP");
	TCase *tc_extern = tcase_create("Extern Functions");

	tcase_add_checked_fixture(tc_extern, setup_dsp, teardown_dsp);
	tcase_add_test(tc_extern, test__dsp_get_volume__0);
	tcase_add_test(tc_extern, test__dsp_get_volume__0123);
	tcase_add_test(tc_extern, test__dsp_set_volume__0);
	tcase_add_test(tc_extern, test__dsp_set_volume__0765);
	suite_add_tcase(s, tc_extern);
	
	return s;
}

int main(int argc __attribute((unused)), char* argv[] __attribute((unused))) {
	return test_suite_run(test_suite());
}

