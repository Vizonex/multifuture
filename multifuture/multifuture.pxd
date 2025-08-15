# cython: freethreading_compatible = True, binding=True

from libc.stdint cimport uint8_t
from cpython.time cimport PyTime_t
from .casyncio cimport Future

# Added fused type for overloading since MultiFuture can be used in 
# Cython as a bonus
ctypedef fused timeout_type:
    float
    int
    Py_ssize_t
    object

cdef class MultiFuture:
    cdef:
        object _result
        object _exception
        object _exc_handler
        bint _has_exc_handler
        list _callbacks
        set _futures
        uint8_t _state

    cdef int __handle_exception(MultiFuture self)
    cdef int _invoke_callbacks(self, bint exception_raised) except -1

    cpdef bint cancelled(self)
    cpdef bint done(self)
    cpdef bint cancel(self) except -1

    cdef object __get_result(self)
    cpdef object set_result(self, object result)
    cpdef object set_exception(self, object exception)
    cdef int wait_timeout(self, PyTime_t timeout) except -1
    
    cdef int wait_forever(self) except -1
    cdef int _wait_timeout(self, object timeout) except -1
    cpdef object result(self, timeout_type timeout = ?)
    cpdef object exception(self)
    cpdef object add_done_callback(self, object fn)
    cpdef Py_ssize_t remove_done_callback(self, object fn)
    # Creates a new child future from asyncio for an asynchronous thread to hold onto
    cpdef Future create_future(self)
    cpdef int add_future(self, object fut) except -1