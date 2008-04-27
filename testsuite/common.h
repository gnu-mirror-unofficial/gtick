/*
 * common.h: Common Code for Unit Tests
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

#ifndef _COMMON_H
#define _COMMON_H

#include <config.h>

#if (WITH_CHECK != 1)
#error "You need Check for building and running the test suite."
#endif

#include <check.h>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif /* DMALLOC */

/* Memory checking functions */
#ifdef HAVE_DMALLOC_H
struct track_list_t {
	struct track_list_t *next;

        const char *file;
	unsigned int line;
	int func_id;
	DMALLOC_SIZE byte_size;
        DMALLOC_SIZE alignment;
        const DMALLOC_PNT addr;
};

extern int get_track_list_length(struct track_list_t *l);
extern struct track_list_t* track_list;
extern int guard_level;
extern void memory_track(const char *file, const unsigned int line,
                                 const int func_id,
                                 const DMALLOC_SIZE byte_size,
                                 const DMALLOC_SIZE alignment,
                                 const DMALLOC_PNT old_addr,
                                 const DMALLOC_PNT new_addr);

#define RESOURCE_GUARD_START() do { \
	fail_unless(guard_level == 0, "Nested RESOURCE_GUARD_START() not supported"); \
	guard_level ++; \
	dmalloc_debug_setup("debug=0x4e48503"); \
	track_list = NULL; \
	dmalloc_track(memory_track); \
} while (0)

#define RESOURCE_GUARD_END() do { \
	fail_unless(guard_level == 1, "RESOURCE_GUARD_END() without RESOURCE_GUARD_START()"); \
	guard_level --; \
	dmalloc_track(NULL); \
	if (track_list != NULL && track_list->line != 0) /* TODO: externally managed chunks? */ \
		fail("%d memory leak(s) detected. First leak is %ld bytes allocated at %s:%d", \
			get_track_list_length(track_list), \
			track_list->byte_size, \
			track_list->line ? track_list->file : "<unknown file>", \
			track_list->line); \
	dmalloc_debug(0); \
} while (0)

#else
#define RESOURCE_GUARD_START()
#define RESOURCE_GUARD_END()
#endif /* HAVE_DMALLOC_H */

/* default epsilon */
extern double epsilon;
/* returns 1 if a is close enough to b considering epsilon */
extern int epsilon_cmp(double a, double b);

extern int test_suite_run(Suite* s);
#endif /* _COMMON_H */
