/*
 * common.c: Common Code for Unit Tests
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

#include "common.h"

#ifdef HAVE_DMALLOC_H
/* Returns number of elements in specified list */
int get_track_list_length(struct track_list_t *l) {
	int ret = 0;
	while (l) {
		l = l->next;
		ret++;
	}
	return ret;
}

struct track_list_t* track_list;
int guard_level = 0;

void memory_track(const char *file, const unsigned int line,
                                 const int func_id,
                                 const DMALLOC_SIZE byte_size,
                                 const DMALLOC_SIZE alignment,
                                 const DMALLOC_PNT old_addr,
                                 const DMALLOC_PNT new_addr)
{
	dmalloc_track(NULL);
	switch (func_id) {
		case DMALLOC_FUNC_MALLOC:
		case DMALLOC_FUNC_CALLOC:
		case DMALLOC_FUNC_MEMALIGN:
		case DMALLOC_FUNC_VALLOC:
		case DMALLOC_FUNC_STRDUP:
			/* Allocation: Add entry to list */
			{
				struct track_list_t** temp = &track_list;

				while (*temp)
					temp = &((*temp)->next);

				*temp = (struct track_list_t*) malloc(sizeof(struct track_list_t));
				(*temp)->next = NULL;

				(*temp)->file = file;
				(*temp)->line = line;
				(*temp)->func_id = func_id;
				(*temp)->byte_size = byte_size;
				(*temp)->alignment = alignment;
				(*temp)->addr = new_addr;
			}
			break;
		case DMALLOC_FUNC_REALLOC:
		case DMALLOC_FUNC_RECALLOC:
			/* Re-Allocation: Change entry in list */
			{
				struct track_list_t** temp = &track_list;

				while (*temp && (*temp)->addr != old_addr)
					temp = &((*temp)->next);

				fail_unless(*temp != NULL,
					"Reallocation of unknown memory chunk (%d bytes) at %s:%d",
					byte_size, line ? file : "<unknown file>", line);

				assert((*temp)->addr == old_addr);
				(*temp)->addr = new_addr;
			}
			break;
		case DMALLOC_FUNC_FREE:
		case DMALLOC_FUNC_CFREE:
			/* De-Allocation: Remove entry from list */
			{
				struct track_list_t** temp = &track_list;
				struct track_list_t* free_elem = NULL;

				while (*temp && (*temp)->addr != old_addr)
					temp = &((*temp)->next);
#if 0
				fail_unless(*temp != NULL,
					"De-Allocation of unknown memory chunk (%d bytes) at %s:%d",
					byte_size, line ? file : "<unknown file>", line);
#endif
				if (*temp == NULL) break; /* TODO: Some external entity did it... */

				assert((*temp)->addr == old_addr);

				free_elem = *temp;
				*temp = (*temp)->next;
				free(free_elem);
			}
			break;
#if 0 /* C++ Functions unsupported */
		case DMALLOC_FUNC_NEW:
		case DMALLOC_FUNC_NEW_ARRAY:
		case DMALLOC_FUNC_DELETE:
		case DMALLOC_FUNC_DELETE_ARRAY:
#endif
		default:
			fail("Unsupported dmalloc func_id: %d", func_id);
	}
	dmalloc_track(memory_track);
}
#endif /* HAVE_DMALLOC_H */

/* default epsilon */
double epsilon = 0.000001;
/* returns 1 if a is close enough to b considering epsilon */
int epsilon_cmp(double a, double b) {
	return fabs(a - b) < epsilon;
}

int test_suite_run(Suite* s) {
	int number_failed;
	SRunner *sr;

#ifdef HAVE_DMALLOC_H
	dmalloc_debug(0);
#else
	printf("WARNING: No memory leak checks done due to missing dmalloc.\n");
#endif

	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

