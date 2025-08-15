from libc.stdint cimport uint8_t as uint8_t

cdef extern from "atomic_compat.h" nogil:
    uint8_t atomic_load_uint8(const uint8_t *obj)
    uint8_t atomic_add_uint8(uint8_t* obj, uint8_t value)
    int atomic_compare_exchange_uint8(uint8_t *obj, uint8_t *expected, uint8_t value)
    uint8_t atomic_exhange_uint8(uint8_t* obj, uint8_t value)

