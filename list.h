#ifndef WWM_LIST_H



#include <stdint.h>
#include <stdlib.h>



/* Appends to a list */
#define listAppend(list,value)	({			\
	(list) = _listAppend((List*)(list));	\
	(list)[listCount(list)-1] = (value);	\
})
/**/
#define listClear(list, index)	(list = _listClear((List*)(list), (index)))
/* Returns the length of the list */
#define listCount(list)			(listMeta(list)->count)
/* Pointer to data (entries) of the list */
#define listData(list)			((void*)((List*)list+1))
/* Deletes a list */
#define listDelete(list)		({ free(listMeta(list)); (list) = NULL; })
/* Find a value in list */
#define listFind(list, value) ({	\
	const void* const ptr = (void*)(memstr((list), listCount(list)*listSize(list), &(value), listSize(list)) - (void*)(list));	\
	ptr!=-list ? (uint_fast32_t)((uintptr_t)ptr/listSize(list)) : (uint_fast32_t)-1;	\
})
/* Find a pattern in an entry data */
#define listFindP(list, value)	({	\
	const void* const ptr = (void*)(memstr((list), listCount(list)*listSize(list), &(value), sizeof(value)) - (void*)(list));	\
	ptr!=(void*)(-(uintptr_t)list) ? (uint_fast32_t)((uintptr_t)ptr/listSize(list)) : (uint_fast32_t)-1;	\
})
/* Pointer to metadata of the list */
#define listMeta(list)			((List*)list-1)
/* Returns the size of an element */
#define listSize(list)			(listMeta(list)->size)

/* Maximal amount of elements */
#define LIST_MAXELM				(2097151)
/* Maximal size of an element */
#define LIST_MAXSIZ				(4194304)
/* Find bytes in memory */
#define memstr(haystack,hsize,needle,nsize)	_memstr((void*)(haystack),(hsize),(char*)(needle),(nsize))



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
extern void* listNew(const uint_fast32_t count, const uint_fast32_t size);
/* Extends the list */
extern void* _listAppend(List* list);
extern void* _listClear(List* list, const uint_fast32_t index);
void* _memstr(void* haystack, size_t hsize, char* restrict const needle, const size_t nsize);



#endif
