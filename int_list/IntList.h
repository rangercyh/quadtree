#ifndef INT_LIST_H
#define INT_LIST_H

#ifdef __cplusplus
#define IL_FUNC extern "C"
#else
#define IL_FUNC
#endif

typedef struct IntList IntList;
enum { il_fixed_cap = 128 };

struct IntList {
    // Stores a fixed-size buffer in advance to avoid requiring
    // a heap allocation until we run out of space.
    int fixed[il_fixed_cap];

    // Points to the buffer used by the list. Initially this will
    // point to 'fixed'.
    int* data;

    // Stores how many integer fields each element has.
    int num_fields;

    // Stores the number of elements in the list.
    int num;

    // Stores the capacity of the array.
    int cap;

    // Stores an index to the free element or -1 if the free list
    // is empty.
    int free_element;
};

// ---------------------------------------------------------------------------------
// List Interface
// ---------------------------------------------------------------------------------
// Creates a new list of elements which each consist of integer fields.
// 'num_fields' specifies the number of integer fields each element has.
IL_FUNC void il_create(IntList* il, int num_fields);

// Destroys the specified list.
IL_FUNC void il_destroy(IntList* il);

// Returns the number of elements in the list.
IL_FUNC int il_size(const IntList* il);

// Returns the value of the specified field for the nth element.
IL_FUNC int il_get(const IntList* il, int n, int field);

// Sets the value of the specified field for the nth element.
IL_FUNC void il_set(IntList* il, int n, int field, int val);

// Clears the specified list, making it empty.
IL_FUNC void il_clear(IntList* il);

// ---------------------------------------------------------------------------------
// Stack Interface (do not mix with free list usage; use one or the other)
// ---------------------------------------------------------------------------------
// Inserts an element to the back of the list and returns an index to it.
IL_FUNC int il_push_back(IntList* il);

// Removes the element at the back of the list.
IL_FUNC void il_pop_back(IntList* il);

// ---------------------------------------------------------------------------------
// Free List Interface (do not mix with stack usage; use one or the other)
// ---------------------------------------------------------------------------------
// Inserts an element to a vacant position in the list and returns an index to it.
IL_FUNC int il_insert(IntList* il);

// Removes the nth element in the list.
IL_FUNC void il_erase(IntList* il, int n);

#endif
