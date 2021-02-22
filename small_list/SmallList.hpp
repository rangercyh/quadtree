// ************************************************************************************
// SmallList.hpp
// ************************************************************************************
#ifndef SMALL_LIST_HPP
#define SMALL_LIST_HPP

#include <cstdlib>
#include <cstring>
#include <cassert>

// Stores a random-access sequence of elements similar to vector, but avoids
// heap allocations for small lists. T must be trivially constructible and
// destructible.
template <class T>
class SmallList
{
public:
    // Creates an empty list.
    SmallList();

    // Creates a copy of the specified list.
    SmallList(const SmallList& other);

    // Copies the specified list.
    SmallList& operator=(const SmallList& other);

    // Destroys the list.
    ~SmallList();

    // Returns the number of agents in the list.
    int size() const;

    // Returns the nth element.
    T& operator[](int n);

    // Returns the nth element in the list.
    const T& operator[](int n) const;

    // Returns an index to a matching element in the list or -1
    // if the element is not found.
    int find_index(const T& element) const;

    // Clears the list.
    void clear();

    // Reserves space for n elements.
    void reserve(int n);

    // Inserts an element to the back of the list.
    void push_back(const T& element);

    /// Pops an element off the back of the list.
    T pop_back();

    // Swaps the contents of this list with the other.
    void swap(SmallList& other);

    // Returns a pointer to the underlying buffer.
    T* data();

    // Returns a pointer to the underlying buffer.
    const T* data() const;

private:
    enum { fixed_cap = 256 };
    struct ListData {
        ListData();
        T buf[fixed_cap];
        T* data;
        int num;
        int cap;
    };
    ListData ld;
};

/// Provides an indexed free list with constant-time removals from anywhere
/// in the list without invalidating indices. T must be trivially constructible
/// and destructible.
template <class T>
class FreeList
{
public:
    /// Creates a new free list.
    FreeList();

    /// Inserts an element to the free list and returns an index to it.
    int insert(const T& element);

    // Removes the nth element from the free list.
    void erase(int n);

    // Removes all elements from the free list.
    void clear();

    // Returns the range of valid indices.
    int range() const;

    // Returns the nth element.
    T& operator[](int n);

    // Returns the nth element.
    const T& operator[](int n) const;

    // Reserves space for n elements.
    void reserve(int n);

    // Swaps the contents of the two lists.
    void swap(FreeList& other);

private:
    union FreeElement {
        T element;
        int next;
    };
    SmallList<FreeElement> data;
    int first_free;
};

// ---------------------------------------------------------------------------------
// SmallList Implementation
// ---------------------------------------------------------------------------------
template <class T>
SmallList<T>::ListData::ListData(): data(buf), num(0), cap(fixed_cap) {
}

template <class T>
SmallList<T>::SmallList() {
}

template <class T>
SmallList<T>::SmallList(const SmallList& other)
{
    if (other.ld.cap == fixed_cap) {
        ld = other.ld;
        ld.data = ld.buf;
    } else {
        reserve(other.ld.num);
        for (int j=0; j < other.size(); ++j) {
            ld.data[j] = other.ld.data[j];
        }
        ld.num = other.ld.num;
        ld.cap = other.ld.cap;
    }
}

template <class T>
SmallList<T>& SmallList<T>::operator=(const SmallList<T>& other) {
    SmallList(other).swap(*this);
    return *this;
}

template <class T>
SmallList<T>::~SmallList() {
    if (ld.data != ld.buf) {
        free(ld.data);
    }
}

template <class T>
int SmallList<T>::size() const {
    return ld.num;
}

template <class T>
T& SmallList<T>::operator[](int n) {
    assert(n >= 0 && n < ld.num);
    return ld.data[n];
}

template <class T>
const T& SmallList<T>::operator[](int n) const {
    assert(n >= 0 && n < ld.num);
    return ld.data[n];
}

template <class T>
int SmallList<T>::find_index(const T& element) const {
    for (int j = 0; j < ld.num; ++j) {
        if (ld.data[j] == element) {
            return j;
        }
    }
    return -1;
}

template <class T>
void SmallList<T>::clear() {
    ld.num = 0;
}

template <class T>
void SmallList<T>::reserve(int n) {
    enum { type_size = sizeof(T) };
    if (n > ld.cap) {
        if (ld.cap == fixed_cap) {
            ld.data = static_cast<T*>(malloc(n * type_size));
            memcpy(ld.data, ld.buf, sizeof(ld.buf));
        } else {
            ld.data = static_cast<T*>(realloc(ld.data, n * type_size));
        }
        ld.cap = n;
    }
}

template <class T>
void SmallList<T>::push_back(const T& element) {
    if (ld.num >= ld.cap) {
        reserve(ld.cap * 2);
    }
    ld.data[ld.num++] = element;
}

template <class T>
T SmallList<T>::pop_back() {
    return ld.data[--ld.num];
}

template <class T>
void SmallList<T>::swap(SmallList& other) {
    ListData& ld1 = ld;
    ListData& ld2 = other.ld;

    const int use_fixed1 = ld1.data == ld1.buf;
    const int use_fixed2 = ld2.data == ld2.buf;

    const ListData temp = ld1;
    ld1 = ld2;
    ld2 = temp;

    if (use_fixed1) {
        ld2.data = ld2.buf;
    }
    if (use_fixed2) {
        ld1.data = ld1.buf;
    }
}

template <class T>
T* SmallList<T>::data() {
    return ld.data;
}

template <class T>
const T* SmallList<T>::data() const {
    return ld.data;
}

// ---------------------------------------------------------------------------------
// FreeList Implementation
// ---------------------------------------------------------------------------------
template <class T>
FreeList<T>::FreeList(): first_free(-1) {
}

template <class T>
int FreeList<T>::insert(const T& element) {
    if (first_free != -1) {
        const int index = first_free;
        first_free = data[first_free].next;
        data[index].element = element;
        return index;
    } else {
        FreeElement fe;
        fe.element = element;
        data.push_back(fe);
        return data.size() - 1;
    }
}

template <class T>
void FreeList<T>::erase(int n) {
    assert(n >= 0 && n < data.size());
    data[n].next = first_free;
    first_free = n;
}

template <class T>
void FreeList<T>::clear() {
    data.clear();
    first_free = -1;
}

template <class T>
int FreeList<T>::range() const {
    return data.size();
}

template <class T>
T& FreeList<T>::operator[](int n) {
    return data[n].element;
}

template <class T>
const T& FreeList<T>::operator[](int n) const {
    return data[n].element;
}

template <class T>
void FreeList<T>::reserve(int n) {
    data.reserve(n);
}

template <class T>
void FreeList<T>::swap(FreeList& other) {
    const int temp = first_free;
    data.swap(other.data);
    first_free = other.first_free;
    other.first_free = temp;
}

#endif
