#ifndef WWM_LIST_H



#include <stdint.h>



/* Appends to a list */
#define listAppend(list,value)							\
	(list) = listExpand((list), 1);						\
	listSet((list), listMeta(list)->count-1, (value))
#define listDelete(list)		(free(listMeta(list)))	/* Deletes a list */
#define listCount(list)			(listMeta(list)->count)	/* Returns the length of the list */
#define listSize(list)			(listMeta(list)->size)	/* Returns the size of an element */
#define listMeta(list)			((list)-sizeof(List))	/* Pointer to metadata of the list */
#define LIST_MAXELM				(2097151)				/* Maximal amount of elements */
#define LIST_MAXSIZ				(4194304)				/* Maximal size of an element */



/*	Lists are dynamically allocated arrays, which size can be changed */
typedef struct List {
	/* [Amount of initialized elements]
	 * Max element count: 2,097,152
	 */
	uint64_t count		:21;
	/* [Size of each element]
	 * Max element size: 4,194,304 bytes (4 MB)
	 * Size value of 0 means the element is 4,194,304 bytes in size
	 */
	uint64_t size		:22;
	/* [Amount of allocated elements]
	 * Max element count: 2,097,152
	 */
	uint64_t allocated	:21;
	/* void data[]; */
} List;



/* Uninitializes an element and shifts other elements over the gap */
extern List* listClear(List* list, const uint32_t index);
/* Initialize more elements */
extern List* listExpand(List* list, const uint32_t delta);
/* Retrives the value of a list element */
extern List* listGet(List* restrict const list, const uint32_t index);
/* Initializes a new list*/
extern List* listNew(const uint32_t count, const uint32_t size);
/* Writes the value to a list element */
extern List* listSet(List* restrict const list, const uint32_t index, List* value);

#endif
