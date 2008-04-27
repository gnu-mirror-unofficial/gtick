/*
 * testg711.c: Unit Tests for g711.c
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
#include "g711.h"

/*
 * Test external linear2ulaw()
 */
START_TEST(test__linear2ulaw__0) {
	unsigned char x;

	RESOURCE_GUARD_START();
	x = linear2ulaw(0);
	fail_unless(x == 0xFF, "Wrong ulaw value (0x%02x)", x);
	RESOURCE_GUARD_END();
}
END_TEST

Suite *test_suite(void) {
	Suite *s = suite_create("G.711");
	TCase *tc_extern = tcase_create("Extern Functions");

	tcase_add_test(tc_extern, test__linear2ulaw__0);
	suite_add_tcase(s, tc_extern);
	
	return s;
}

int main(int argc __attribute((unused)), char* argv[] __attribute((unused))) {
	return test_suite_run(test_suite());
}

