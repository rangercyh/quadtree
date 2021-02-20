#include "elt_free_list.h"
#include <stdlib.h>
#include <string.h>

EltFreeList* efl_create() {
    EltFreeList* fl = (EltFreeList *)malloc(sizeof(EltFreeList));
    fl->data = fl->fixed;
    fl->num = 0;
    fl->cap = fixed_cap;
    fl->free_element = -1;
    return fl;
}

void efl_destroy(EltFreeList* fl) {
    // Free the buffer only if it was heap allocated.
    if (fl->data != fl->fixed) {
        free(fl->data);
    }
}

int efl_insert(EltFreeList* fl, int id, float mx, float my, int next) {
    int index;
    if (fl->free_element != -1) {
        index = fl->free_element;
        fl->free_element = fl->data[index].next;
    } else {
        if (fl->num >= fl->cap) {
            efl_reserve(fl, fl->cap * 2);
        }
        index = fl->num++;
    }
    fl->data[index].id = id;
    fl->data[index].mx = mx;
    fl->data[index].my = my;
    fl->data[index].next = next;
    return index;
}

void efl_remove(EltFreeList* fl, int n) {
    if (n >= 0 && n < fl->num) {
        fl->data[n].next = fl->free_element;
        fl->free_element = n;
    }
}

Elt* efl_get(EltFreeList* fl, int n) {
    if (n >= 0 && n < fl->num) {
        return &fl->data[n];
    }
    return NULL;
}

void efl_reserve(EltFreeList* fl, int n) {
    if (n > fl->cap) {
        if (fl->cap == fixed_cap) {
            fl->data = (Elt *)malloc(n * sizeof(Elt));
            memcpy(fl->data, fl->fixed, sizeof(fl->fixed));
        } else {
            fl->data = realloc(fl->data, n * sizeof(Elt));
        }
        fl->cap = n;
    }
}
