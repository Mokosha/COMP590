/**
 *  Copyright 2013 Mike Reed
 *
 *  COMP 590 -- Fall 2013
 */

#ifndef GTypes_DEFINED
#define GTypes_DEFINED

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 *  This header should be (directly or indirectly) included by all other
 *  headers and cpp files for COMP 590.
 */

/**
 *  Call g_crash() when you want to halt execution
 */
static inline void g_crash() {
    fprintf(stderr, "g_crash called\n");
    *(int*)0x50FF8001 = 12345;
}

#ifdef NDEBUG
    #define GRELEASE
    #define GASSERT(pred)
    #define GDEBUGCODE(code)
#else
    #define GDEBUG
    #define GASSERT(pred)       do { if (!(pred)) g_crash(); } while (0)
    #define GDEBUGCODE(code)    code
#endif

/**
 *  Given an array (not a pointer), this macro will return the number of
 *  elements declared in that array.
 */
#define GARRAY_COUNT(array) (int)(sizeof(array) / sizeof(array[0]))

///////////////////////////////////////////////////////////////////////////////

template <typename T> void GSwap(T& a, T& b) {
    T c(a);
    a = b;
    b = c;
}

template <typename T> const T& GMax(const T& a, const T& b) {
    return a > b ? a : b;
}

template <typename T> const T& GMin(const T& a, const T& b) {
    return a < b ? a : b;
}

template <typename T> class GAutoDelete {
    public:
    GAutoDelete(T* obj) : fObj(obj) {}
    ~GAutoDelete() { delete fObj; }
    
    T* get() const { return fObj; }
    operator T*() { return fObj; }
    T* operator->() { return fObj; }
    
    // Return the contained object, transfering ownership to the caller.
    // The internal pointer will be set to NULL and therefore not deleted
    // when this goes out of scope.
    T* detach() {
        T* obj = fObj;
        fObj = NULL;
        return obj;
    }
    
    private:
    T*  fObj;
};

template <typename T> class GAutoArray {
public:
    GAutoArray(int N) : fArray(new T[N]) {}
    ~GAutoArray() { delete[] fArray; }
    
    T* get() const { return fArray; }
    operator T*() { return fArray; }
    T* operator->() { return fArray; }
    
private:
    T*  fArray;
};

template <typename T, size_t N> class GAutoSArray {
public:
    GAutoSArray(int count) {
        if (count > N) {
            fArray = new T[count];
        } else {
            fArray = (T*)fStorage;
        }
    }
    ~GAutoSArray() {
        if (fArray != (T*)fStorage) {
            delete[] fArray;
        }
    }
    
    T* get() const { return fArray; }
    operator T*() { return fArray; }
    T* operator->() { return fArray; }
    
private:
    T*  fArray;
    intptr_t fStorage[(N + 1) * sizeof(T) / sizeof(intptr_t)];
};

///////////////////////////////////////////////////////////////////////////////

static inline float GPinToUnitFloat(float x) {
    return GMax<float>(0, GMin<float>(1, x));
}

#endif
