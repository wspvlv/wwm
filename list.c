#include "list.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>



/* Creates a new list by allocating `count` entries and specifying the list entry size */
void* listNew(const uint32_t count, const uint32_t size) {
	/* Allocate `count` entries and space for metadata */
	List* list = malloc(count*size + sizeof(List));
	/* Only proceed if the allocation didn't fail */
	if (list) {
		list->allocated = count;
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
