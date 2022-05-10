#ifndef WWM_LIST_H



#include <stdint.h>



/* Appends to a list */
#define listAppend(list,value)				\
	(list) = _listAppend((List*)(list));	\
	(list)[listCount(list)] = (value)
/* Deletes a list */
#define listDelete(list)		(free(listMeta(list)))
/* Returns the length of the list */
#define listCount(list)			(listMeta(list)->count)
/* Returns the size of an element */
#define listSize(list)			(listMeta(list)->size)
/* Pointer to metadata of the list */
#define listMeta(list)			((List*)((void*)(list)-sizeof(List)))
/* Pointer to data (entries) of the list */
#define listData(list)			((List*)((void*)(list)+sizeof(List)))
/* Maximal amount of elements */
#define LIST_MAXELM				(2097151)
/* Maximal size of an element */
#define LIST_MAXSIZ				(4194304)



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



/* Initializes a new list*/
extern void* listNew(const uint32_t count, const uint32_t size);
/* Extends the list */
extern void* _listAppend(List* list);



#endif
