#ifndef __ATOMIC_COMPAT_H__
#define __ATOMIC_COMPAT_H__


/* 
Provides compatability of atomic variables in C so that Cyares doesn't have to move to C++.
It isn't because C++ is bad it's just more annoying than C.

It made sense to just provide backwards compatability for atomic types 
and borrow from CPython for an easy implementation, if you see an appache 2.0 license update on cyares,
you now will know as to why that is.

*/

#include <stdint.h>

#ifdef _WIN32 

#ifdef __cplusplus
extern "C" {
#endif

#include <intrin.h>
// XXX: Stdatomic has problems on windows still so we need backups
// inplace for c, this will go first in the order of priority

// we use cpython's implementation to workaround problems.

static inline uint8_t
atomic_load_uint8(const uint8_t *obj)
{
#if defined(_M_X64) || defined(_M_IX86)
    return *(volatile uint8_t *)obj;
#elif defined(_M_ARM64)
    return (uint8_t)__ldar8((unsigned __uint8 volatile *)obj);
#else
#  error "no implementation of atomic_load_uuint8"
#endif
}

uint8_t atomic_add_uint8(uint8_t* obj, uint8_t value){
    return (uint8_t)_InterlockedExchangeAdd8((volatile char*)obj, (char)value);
}

static inline int 
atomic_compare_exchange_uint8(uint8_t *obj, uint8_t *expected, uint8_t value){
    uint8_t initial = (uint8_t)_InterlockedCompareExchange8(
                                       (volatile char *)obj,
                                       (char)value,
                                       (char)*expected);
    if (initial == *expected) {
        return 1;
    }
    *expected = initial;
    return 0;
}

static inline uint8_t 
atomic_exhange_uint8(uint8_t *obj, uint8_t value){
    return (uint8_t)_InterlockedExchange8((volatile char*)obj, (char)value);
}


#ifdef __cplusplus
}
#endif

#else /* GCC implementation should go next */

#ifdef __GCC__

#ifdef __cplusplus
extern "C" {
#endif

static inline uint8_t 
atomic_load_uint8(const uint8_t *obj)
{
    return __atomic_load_n(obj, __ATOMIC_SEQ_CST);
}
static inline uint8_t 
atomic_add_uint8(uint8_t* obj, uint8_t value)
{
    return __atomic_fetch_add(obj, value, __ATOMIC_SEQ_CST);
}

static inline int 
atomic_compare_exchange_uint8(uint8_t *obj, uint8_t *expected, uint8_t desired)
{ 
    return __atomic_compare_exchange_n(obj, expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); 
}

static inline uint8_t 
atomic_exchange_uint8(uint8_t *obj, uint8_t value)
{ 
    return __atomic_exchange_n(obj, value, __ATOMIC_SEQ_CST); 
}

#ifdef __cplusplus
}
#endif

#else 

/* Final Possible Workaround */

#ifdef __cplusplus
extern "C++" {
#  include <atomic>
}
#  define _ATOMIC_COMPAT_USING_STD using namespace std
#  define _Atomic(tp) atomic<tp>
#else
#  define  _ATOMIC_COMPAT_USING_STD
#  include <stdatomic.h>
#endif

static inline uint8_t
atomic_load_uint8(const uint8_t *obj)
{
    _ATOMIC_COMPAT_USING_STD;
    return atomic_load((const _Atomic(uint8_t)*)obj);
}

static inline uint8_t
atomic_add_uint8(uint8_t *obj, uint8_t value)
{
    _ATOMIC_COMPAT_USING_STD;
    return atomic_fetch_add((_Atomic(uint8_t)*)obj, value);
}

static inline int
atomic_compare_exchange_uint8(uint8_t *obj, uint8_t *expected, uint8_t desired)
{
    _ATOMIC_COMPAT_USING_STD;
    return atomic_compare_exchange_strong((_Atomic(uint8_t)*)obj,
                                          expected, desired);
}

static inline uint8_t
atomic_exchange_uint8(uint8_t *obj, uint8_t value)
{
    _ATOMIC_COMPAT_USING_STD;
    return atomic_exchange((_Atomic(uint8_t)*)obj, value);
}

#endif /* __GCC__ */

#endif /* _WIN32 */


#endif // __ATOMIC_COMPAT_H__