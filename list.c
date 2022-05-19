#include "list.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>



/* Creates a new list by allocating `count` entries and specifying the list entry size */
void* listNew(const uint_fast32_t count, const uint_fast32_t size) {
	/* Allocate `count` entries and space for metadata */
	List* list = malloc(count*size + sizeof(List));
	/* Only proceed if the allocation didn't fail */
	if (list) {
		/* Let's keep it even to avoid any problems during shrinking */
		list->allocated = count%2 ? count+1 : count;
		list->size = size;
		list->count = 0;
		/* We need to return data */
		list = listData(list);
	}
	return list;
}

void* _listAppend(List* list) {
	/* Check whether the list exists */
	if (list) {
		/* Make `list` point to the metadata of the list */
		list = listMeta(list);
		/* By appending we increment the amount of entries */
		list->count++;
		/* If amount of entries more than the amount of entries allocated */
		if (list->count > list->allocated) {
			/* Update `allocated` */
			list->allocated *= 2;
			/* Extend to double the size to minimize the amount of future reallocations */
			list = realloc(list, list->allocated*list->size + sizeof(List));
		}
		/* We need to return data */
		list = listData(list);
	}
	/* Return (changed/unchanged) list pointer */
	return list;
}

void* _listClear(List* list, const uint_fast32_t index) {
	/* Check whether thelist list exists */
	if (list) {
		/* Make `list` point to the metadata of the list */
		list = listMeta(list);
		if (index < list->count) {
			/* Shift all entries by 1 to remove the entry*/
			memmove(
				listData(list) + index*list->size,
				listData(list) + (index+1)*list->size,
				(list->count-index-1)*list->size
			);
			list->count--;
			/* If less than half of allocated entries are initialized... */
			if (list->count < list->allocated/2) {
				list->allocated /= 2;
				list = realloc(list, list->allocated*list->size + sizeof(List));
			}
		}
		list = listData(list);
	}
	return list;
}
