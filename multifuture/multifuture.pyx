# cython: freethreading_compatible = True, binding=True

import types
from cpython.object cimport PyObject
from cpython.time cimport PyTime_t, monotonic_ns
from libc.stdint cimport uint8_t
from .stdatomic cimport atomic_load_uint8, atomic_exhange_uint8
from cpython.exc cimport PyErr_CheckSignals, PyErr_Occurred, PyErr_GivenExceptionMatches, PyErr_Print, PyErr_SetObject
from cpython.list cimport PyList_Append, PyList_GET_SIZE, PyList_New, PyList_GET_ITEM 
from cpython.set cimport PySet_Add, PySet_Discard, PySet_Clear, PySet_Contains

# NOTE: Needed for determining if user compiled _asynciomodule.c or not (That is the only requirement with this library)
from asyncio.futures import _PyFuture
from asyncio import get_event_loop as _get_event_loop

from .casyncio cimport *
import_casyncio()


cdef extern from "Python.h":
    """
/* Fingers crossed that python puts this into it's public C-API at a higher level */
typedef _PyTime_round_t PyTime_round_t;
#define PyTime_FromSecondsObject _PyTime_FromSecondsObject

/* From Python 3.14 Brought here for backwards comptability*/

#if (PY_VERSION_HEX < 0x030d00f0)
    #ifndef PyTime_MAX
        #define PyTime_MAX LLONG_MAX
    #endif
    #ifndef PyTime_MIN
        #define PyTime_MIN LLONG_MIN
    #endif

    /* 3.12 or under */
    static inline int
    compat_pytime_add(int64_t *t1, int64_t t2)
    {
        if (t2 > 0 && *t1 > PyTime_MAX - t2) {
            *t1 = PyTime_MAX;
            return -1;
        }
        else if (t2 < 0 && *t1 < PyTime_MIN - t2) {
            *t1 = PyTime_MIN;
            return -1;
        }
        else {
            *t1 += t2;
            return 0;
        }
    }
    int64_t
    _PyTime_Add(int64_t t1, int64_t t2)
    {
        (void)compat_pytime_add(&t1, t2);
        return t1;
    }
    #define PyTime_Add _PyTime_Add
#else 
    #define PyTime_Add _PyTime_Add
#endif

    """

    PyObject *PyObject_CallOneArg(object callable, object arg)
    PyObject *PyObject_CallNoArgs(object callable) except NULL
    PyObject *PyObject_CallMethodNoArgs(object obj, object name) except NULL
    void Py_INCREF(PyObject*)
    void Py_DECREF(PyObject*)
  
    void Py_CLEAR(object)
    # We want to ignore NULL checking o so a modification to function signature 
    # was required to fit that request.
    void PyList_SET_ITEM(object list, Py_ssize_t i, PyObject* o)


    # Reason for needing this has to do with Floats and Doubles not being precise enough.
    # in order to remain accurate with desired timeouts Using the cpython implementation 
    # felt like the smartest solution.
    enum _PyTime_round_t:
        # Round towards minus infinity (-inf).
        # For example, used to read a clock.
        _PyTime_ROUND_FLOOR = 0,
        # Round towards infinity (+inf).
        # For example, used for timeout to wait "at least" N seconds.
        _PyTime_ROUND_CEILING = 1,
        # Round to nearest with ties going to nearest even integer.
        # For example, used to round from a Python float.
        _PyTime_ROUND_HALF_EVEN =2 ,
        # Round away from zero
        # For example, used for timeout. _PyTime_ROUND_CEILING rounds
        # -1e-9 to 0 milliseconds which causes bpo-31786 issue.
        _PyTime_ROUND_UP = 3
        #    the timeout sign as expected. select.poll(timeout) must block
        #    for negative values."
        # _PyTime_ROUND_TIMEOUT (an alias for _PyTime_ROUND_UP) should be
        # used for timeouts.
        _PyTime_ROUND_TIMEOUT = 3

    ctypedef _PyTime_round_t PyTime_round_t
    # Comptable version of _PyTime_Add for older versions of Python
    PyTime_t PyTime_Add(PyTime_t, PyTime_t)
    int PyTime_FromSecondsObject(PyTime_t *t, object obj, PyTime_round_t round) except -1


# Custom CPython function for copying off lists quickly rather than calling "list.copy(..)" in a hacky way
cdef list PyList_Copy(list l):
    cdef Py_ssize_t size = PyList_GET_SIZE(l)
    cdef list new_list = PyList_New(size)
    cdef Py_ssize_t i
    for i in range(size):
        PyList_SET_ITEM(new_list, i, PyList_GET_ITEM(new_list, i))
    return new_list

cdef int PyList_Clear(list l) except -1:
    # For Now leave it as this I'll change if I can make a version comptatable implementation
    if PyObject_CallMethodNoArgs(l, "copy") != NULL:
        return -1
    return 0




# NOTE: There's a pretty high chance I turn this into a Python Library
# with a pycapsule (This is for extra speed of course and to demonstrate 
# that CPython should try this with it's concurrent.futures module)
# otherwise I might try a parking-lot implementation

cdef class CancelledError(Exception):
    pass

cdef class InvalidStateError(Exception):
    pass


DEF RUNNING = 0
DEF CANCELLED = 1
DEF FINISHED = 2

cdef enum INVOKE_REASON:
    INV_RESULT = 0
    INV_CANCELLED = 1
    INV_EXCEPTION = 2




# Threadsafe-Calls incase MultiFuture needs to work in a seperate thread
# Example Scenario: Cyares on event_thread settings

# This program uses Hacked C Recastings of _asynciomodule.c Which I have my fingers crossed we get 
# a CPython Capsule for the asynciomodule in the future to speedup this library and winloop/uvloop

# These are All C Functions so we need a way to safely traverse these
# this may look messy to some of you or redundant but I assue you it's here 
# to attempt to increase speed without loops racing eatch other on one thread also 
# known as a race condition.

# NOTE: We have to pull different loops from different threads as that is the goal of MultiFuture 
# which is to share accross multiple threads and sometimes many different eventloops.

cpdef object __on_set_result(tuple t):
    cdef Future fut = t[0]
    cdef object result = t[1]
    if FutureObj_SetResult(fut, result) < 0:
        raise

cdef inline int __schedule_result_threadsafe(Future fut, object result) except -1:
    if FutureObj_EnsureAlive(fut) < 0:
        return -1
    if Loop_CallSoonThreadSafe(FutureObj_GetLoop(fut), __on_set_result, (fut, result), NULL) < 0:
        return -1
    return 0

cpdef object __on_set_exception(tuple t):
    if FutureObj_SetException(t[0], t[1]) < 0:
        raise

cdef inline int __schedule_exception_threadsafe(Future fut, object exc) except -1:
    if FutureObj_EnsureAlive(fut) < 0:
        return -1

    if Loop_CallSoonThreadSafe(FutureObj_GetLoop(fut), __on_set_exception, (fut, exc), NULL) < 0:
        return -1
    return 0

cpdef object __on_cancel_future(Future fut):
    # we only want to attempt to cacnel, 0 or 1 are acceptable however -1 isn't
    if FutureObj_Cancel(fut) < 0:
        raise

cdef inline int __schedule_cancellation_threadsafe(Future fut) except -1:
    if FutureObj_EnsureAlive(fut) < 0:
        return -1

    if Loop_CallSoonThreadSafe(FutureObj_GetLoop(fut), __on_cancel_future, fut, NULL) < 0:
        return -1
    return 0






cdef class MultiFuture:
    """A Multi-Purpose Future Object for communication between a synchronous parent
    to different asynchronous children that could be in different threads or eventloops"""
    __class_getitem__ = classmethod(types.GenericAlias)

    def __init__(self, exception_handler:object = None):
        self._state = RUNNING
        self._result = None
        self._exception = None
        self._callbacks = []
        self._futures = set()
        self._exc_handler = exception_handler
        self._has_exc_handler = exception_handler is not None

    cdef int __handle_exception(MultiFuture self):
        cdef PyObject* exc = PyErr_Occurred()
        if exc == NULL: return -1

        if not PyErr_GivenExceptionMatches(Exception, <object>exc):
            return -1

        if self._exc_handler is None:
            PyErr_Print()

        elif PyObject_CallOneArg(self._exc_handler, <object>exc) == NULL:
            return -1
        
        return 0

    cdef int _invoke_callbacks(self, bint exception_raised) except -1:
        cdef list callbacks
        cdef set futures
        cdef object fn
        cdef Future fut
        if self._callbacks:
            # Currently we have to copy off the list 
            # hopefully in the future we can implement NULL Values instead.
            callbacks = PyList_Copy(self._callbacks)
            PyList_Clear(self._callbacks)
            for fn in self._callbacks:
                if PyObject_CallOneArg(fn, self) == NULL:
                    if self.__handle_exception() < 0:
                        del callbacks
                        return -1
            del callbacks

        if self._futures:
            futures = self._futures.copy()
            PySet_Clear(self._futures)
            for fut in futures:
                if not exception_raised:
                    if __schedule_result_threadsafe(fut, self._result) < 0:
                        return -1
                elif __schedule_exception_threadsafe(fut, self._result) < 0:
                    return -1
            del futures

    cpdef bint cancelled(self):
        return atomic_load_uint8(&self._state) == CANCELLED

    cpdef bint done(self):
        return atomic_load_uint8(&self._state) != RUNNING

    cpdef bint cancel(self) except -1:
        cdef Future fut
        cdef object cb
        cdef unsigned int state = atomic_load_uint8(&self._state)
        if state == FINISHED:
            return False
        elif state == CANCELLED:
            return True
        else:
            self._exception = CancelledError(f"{self!r} was cancelled")
            atomic_exhange_uint8(&self._state , CANCELLED)
            # All futures waiting for cancellation signal need to run now.
            for fut in self._futures:
                if __schedule_cancellation_threadsafe(fut) < 0:
                    return -1
            
            # All Callbacks waiting for cancellation need to be signaled
            for cb in self._callbacks:
                if PyObject_CallOneArg(cb, self) == NULL:
                    return -1
        return True

    cdef object __get_result(self):
        if self._exception:
            raise self._exception
        else:
            return self._result
    

    cpdef object set_result(self, object result):
        cdef uint8_t state = atomic_load_uint8(&self._state)
        if state != RUNNING:
            raise InvalidStateError(f"{state}: {self!r}")
        self._result = result
        atomic_exhange_uint8(&self._state, FINISHED)
        self._invoke_callbacks(False)
    
    cpdef object set_exception(self, object exception):
        cdef uint8_t state = atomic_load_uint8(&self._state)
        if state != RUNNING:
            raise InvalidStateError(f"{state}: {self!r}")
        self._exception = exception
        atomic_exhange_uint8(&self._state, FINISHED)
        self._invoke_callbacks(True)
    
    # Public Cython-API
    cdef int wait_timeout(self, PyTime_t timeout) except -1:
        # Waits until object is finished, 
        # returns: 
        #   -1 if an exception occured
        #   0 if timeout did not finish
        #   1 if a respone was obtained


        cdef PyTime_t now = monotonic_ns()
        cdef PyTime_t endgame = 0
        if timeout > 0:
            endgame = PyTime_Add(now, timeout) 
        
        # Nogil rules in cython aren't as lax as cpython
        # so we need to get a shallow copy of the state 
        # at this very moment.
        cdef uint8_t* state = &self._state

        while True:
            # we have to go in and our of the gil so we don't 
            # end up deadlocking the atomic value.
            with nogil:
                if (atomic_load_uint8(state) != RUNNING):
                    return 1
                if not timeout:
                    # failure
                    return 0
                if timeout > 0:
                    timeout = endgame - monotonic_ns()
                    if timeout <= 0:
                        timeout = 0
            
            # We have to regain the gil to check if we need to exit  
            if PyErr_CheckSignals() < 0:
                return -1
        
    # This function always returns True and it's part of the implementation
    cdef int wait_forever(self) except -1:
        if self.done():
            return 1
        cdef uint8_t* state = &self._state
        while True:
            with nogil:
                if (atomic_load_uint8(state) != RUNNING):
                    return 1
            if PyErr_CheckSignals() < 0:
                return -1
        
 
    cdef int _wait_timeout(self, object timeout) except -1:
        cdef PyTime_t t
        if self.done():
            return 1
        if PyTime_FromSecondsObject(&t, timeout, _PyTime_ROUND_TIMEOUT) < 0:
            return -1
        return self.wait_timeout(t)


    cpdef object result(self, 
        timeout_type timeout = -1  # type: ignore (Cyright is complaining about timeout_type even though it's in the pxd file on purpose)
    ):
        """
        obtains the result from the MultiFuture
        if your tying to behave asynchronous you should await with a child object
        but remeber that awaiting requires a running eventloop

        Args:
            :param timeout: a float or integer in seconds to wait for the result in
                -1 assumes to wait forever until obtained.
        
        Returns:
            result object if there was no exception or cancellation
        
        Raises:
            Any Exception passed to the Object Or CancelledError
        """
        cdef int status 
        if not self.done():
            if timeout != -1:
                if timeout < 0:
                    raise ValueError("Timeouts cannot be less than 0")
                status = self._wait_timeout(timeout)
            else:
                status = self.wait_forever()
            if status < 0:
                raise
            # 0
            if not status:
                raise TimeoutError("MultiFuture is not finished yet")
        return self.__get_result()
    
    cpdef object exception(self):
        """Returns with a given exception if any are obtained"""
        cdef state = atomic_load_uint8(&self._state)
        if state == CANCELLED:
            raise self._exception
        elif state == RUNNING:
            raise InvalidStateError("Exception is not set.")
        return self._exception

        # XXX: My Speculation is that cython is waiting for Python 3.9's eol before I can perform this.
        # match state:
        #     case STATE_CANCELLED:
        #         raise self._exception
        #     case STATE_RUNNING:
        #         raise InvalidStateError("Exception is not set.")
        #     case STATE_FINISHED:
        #         return self._exception
    
    
    cpdef object add_done_callback(self, object fn):
        if not callable(fn):
            raise TypeError(f"{fn!r} must be callable")
        if atomic_load_uint8(&self._state) != RUNNING:
            if PyObject_CallOneArg(fn, self) == NULL:
                raise
        
        if PyList_Append(self._callbacks, fn) < 0:
            raise
    
    cpdef Py_ssize_t remove_done_callback(self, object fn):
        cdef list filtered_callbacks = [
            f for f in self._callbacks if f != fn
        ]
        cdef Py_ssize_t removed = PyList_GET_SIZE(self._callbacks) - PyList_GET_SIZE(filtered_callbacks)
        if removed > 0:
            self._callbacks[:] = filtered_callbacks
        return removed

    # Not Public do not use
    def __on_fut(self, Future fut):
        # if Future is cancelled then we need to cancel the others out of being utilized
        if FutureObj_IsDone(fut):
            # Edject from being utilized since user wanted to back off
            PySet_Discard(self._futures, fut)
        if FutureObj_IsCancelled(fut):
            # Call all of them
            self.cancel()
            PySet_Discard(self._futures, fut)
        



    cpdef Future create_future(self):
        """Creates a new asyncio.Future via calling the running 
        thread's eventloop, if none are running this will raise 
        an Exception"""
        # The inner C Function will make sure an EventLoop is obtained.
        # This should throw an error if the user doesn't have a running 
        # eventloop in the thread it's running in.
        cdef Future fut 
        if self.done():
            raise InvalidStateError("MultiFuture is already finished")
        fut = Future()
        if PySet_Add(self._futures, fut) < 0:
            raise
        FutureObj_AddDoneCallback(fut, self.__on_fut, NULL)
        return fut

    cpdef int add_future(self, object fut) except -1:
        """Adds a preexisting future to the MultiFuture, Raises TypeError if 
        future does not inherit from `_asyncio.Future` don't let the `asyncio.Future` 
        pure python version fool you."""
        
        if not isinstance(fut, Future):
            if isinstance(fut, _PyFuture):
                PyErr_SetObject(TypeError, "MultiFuture does not accept the Pure Python implementation of asyncio.Future")
            else:
                PyErr_SetObject(TypeError, f"Future Objects Must be inherited from _asyncio.Future not {self!r}")
            return -1
        FutureObj_AddDoneCallback(fut, self.__on_fut, NULL)
        return 0


 