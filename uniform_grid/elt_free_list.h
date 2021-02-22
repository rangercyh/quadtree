#ifndef ELT_FREE_LIST_H
#define ELT_FREE_LIST_H

#ifdef __cplusplus
#define ELT_FL_FUNC extern "C"
#else
#define ELT_FL_FUNC
#endif

typedef struct Elt Elt;
typedef struct EltFreeList EltFreeList;

struct Elt {
    // Stores the next element in the cell.
    int next;

    // Stores the ID of the element. This can be used to associate external
    // data to the element.
    int id;

    // Stores the center position of the uniformly-sized element.
    float mx, my;
};

enum { fixed_cap = 128 };

struct EltFreeList {
    // Stores a fixed-size buffer in advance to avoid requiring
    // a heap allocation until we run out of space.
    Elt fixed[fixed_cap];

    // Points to the buffer used by the list. Initially this will
    // point to 'fixed'.
    Elt* data;

    // Stores the max number of elements in the list.
    int num;

    // Stores the capacity of the list.
    int cap;

    // Stores an index to the free element or -1 if the free list
    // is empty.
    int free_element;
};

// Creates a new free list.
ELT_FL_FUNC EltFreeList* efl_create();

// Destroys the specified list.
ELT_FL_FUNC void efl_destroy(EltFreeList* fl);

// Inserts an element to the free list and returns an index to it.
ELT_FL_FUNC int efl_insert(EltFreeList* fl, int id, float mx, float my, int next);

// Removes the nth element from the free list.
ELT_FL_FUNC void efl_remove(EltFreeList* fl, int n);

// Returns the nth element.
ELT_FL_FUNC Elt* efl_get(EltFreeList* fl, int n);

// Reserves space for n elements.
ELT_FL_FUNC void efl_reserve(EltFreeList* fl, int n);

#endif
