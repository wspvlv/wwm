#include "list.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>



/* Uninitializes an element and shifts other elements over the gap */
List* listClear(List* list, const uint32_t index) {
	/* Obviously, a list should exist */
	if (list) {
		/* Get the metadate of the list */
		List* meta = listMeta(list);
		/* Can only clear initialized elements */
		if (index <= (uint32_t)(meta->count-1)) {
			/* If an element is not the last, there will be a gap, which will need to be filled */
			if (index != (uint32_t)(meta->count-1)) {
				/* Shift all elements one element left */
				memmove(list+(index*meta->size), list+((index+1)*meta->size), meta->size-(index+1));
			}
			meta->count--;
			/* If less than half of allocated elements are initialize, shrink the list in half */
			if (meta->count <= meta->allocated/2)
				list = realloc(meta, meta->allocated/2*meta->size + sizeof(List));
		}
		/* Nothing to do */
	}
	return list;
}

/* Initialize more elements */
List* listExpand(List* list, const uint32_t delta) {
	/* Obviously, a list should exist */
	if (list) {
		/* Get the metadate of the list */
		List* meta = listMeta(list);
		/* Make sure we are not expanding beyond the limit */
		if (meta->count + delta <= LIST_MAXELM) {
			meta->count += delta;
			/* Reallocate when expanding beyond allocated size */
			if (meta->count > meta->allocated) {
				list = realloc(meta, 2*meta->allocated*meta->size + sizeof(List));
				/* Offset the pointer from metadata */
				list += sizeof(List);
			}
		}
	}
	return list;
}

/* Retrives the value of a list element */
List* listGet(List* restrict const list, const uint32_t index) {
	/* Obviously, a list should exist */
	if (list) {
		/* Get the metadate of the list */
		List* meta = listMeta(list);
		
		/* Check if we're trying to access an initialized element */
		if (index < meta->count)
			return list + index * meta->size;
		/* Otherwise, return NULL */
	}
	return NULL;
}

/* Initializes a new list*/
List* listNew(const uint32_t count, const uint32_t size) {
	/* Make sure element count and size are acceptable */
	if (count <= LIST_MAXELM && size <= LIST_MAXSIZ && size != 0) {
		/* Allocate memory for meta and `count` elements */
		List* list = malloc(count*size + sizeof(List));
		/* Zero elements have been initialized yet */
		list->count = 0;
		list->allocated = count;
		/* "Size value of 0 means the element is 4,194,304 bytes in size" */
		if (size != LIST_MAXSIZ)
			list->size = size;
		else
			list->size = 0;
		/* Offset the pointer from metadata */
		return list + sizeof(List);
	}
	return NULL;
}

/* Writes the value to a list element */
List* listSet(List* restrict const list, const uint32_t index, List* value) {
	/* Obviously, a list should exist */
	if (list) {
		/* Get the metadate of the list */
		List* meta = listMeta(list);
		
		/* Check if we're writing to the initialized elements */
		if (index < meta->count)
			memcpy(list + index * meta->size, value, meta->size);
		/* If we're setting values at the index, it means we are appending, therefore we need to update `count`*/
		else if (index == meta->count) {
			meta->count++;
			memcpy(list + index * meta->size, value, meta->size);
		}
		/* Otherwise, return NULL */
	}
	return NULL;
}
