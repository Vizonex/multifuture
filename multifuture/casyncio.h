#ifndef __CASYNCIO_H__
#define __CASYNCIO_H__

# pragma once


/* This headerfile contains various components of different versions of the
 * CPython asynciomodule.c in order to make the objects run a bit smoother.
 * In Cython, Cython Exposes arrary.array under a hard-coded utility hack
 * Likewise The plan is to do that in a similar manner but since the amount
 * of functions we wish to optimize and sharpen is pretty chaotic it made
 * Sense not to just put them in a pxd file alone and call it a day.
 *
 * Future Objects and Task Objects are all over the place when we want to
 * start accounting for all supported versions of CPython that still get
 * security updates
 *
 *
 * Since the project is Technically MIT/APPACHE 2.0 Which CPython is APPACHE 2.0
 * There should be no problems with borrowing that portion of code in here to
 * improve the performance of winloop or uvloop if uvloop deicded to borrow from me
 * feel free to take whatever you need from me :)
 *
 *
 * Hopfully within the next 5 - 10 years this encourages the CPython maintainers to
 * Make a CAPI Capsule for us to use but we shall see...
 */

/* List of Items that should be in this custom CAPI are items
 * That must Eclipse over module "_asyncio" not "asyncio"
 *
 * These include
 *
 * - Future Object
 *   - Most of the functions are remade to return integers
 *     instead of Python Objects for speed. 
 * - Task Object
 *   - More or less the same idea
 * */



#include <Python.h>
#include <stdint.h>
#include <stddef.h>          // offsetof()

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    STATE_PENDING,
    STATE_CANCELLED,
    STATE_FINISHED
} fut_state;

// Different Python versions Laid out as Hex Values
// I made Macros to simplify saying if 3.XX >= ... or 3.XX+

#if (PY_VERSION_HEX >= 0x030a00f0)
#define CASYNCIO_PYTHON_310
/* 3.10 */
#define FutureObj_HEAD(prefix)                                              \
    PyObject_HEAD                                                           \
    PyObject *prefix##_loop;                                                \
    PyObject *prefix##_callback0;                                           \
    PyObject *prefix##_context0;                                            \
    PyObject *prefix##_callbacks;                                           \
    PyObject *prefix##_exception;                                           \
    PyObject *prefix##_exception_tb;                                        \
    PyObject *prefix##_result;                                              \
    PyObject *prefix##_source_tb;                                           \
    PyObject *prefix##_cancel_msg;                                          \
    fut_state prefix##_state;                                               \
    int prefix##_log_tb;                                                    \
    int prefix##_blocking;                                                  \
    PyObject *dict;                                                         \
    PyObject *prefix##_weakreflist;                                         \
    _PyErr_StackItem prefix##_cancelled_exc_state;
typedef struct {
    FutureObj_HEAD(fut)
} FutureObj;

#else 
/* 3.9 */
#define FutureObj_HEAD(prefix)                                              \
    PyObject_HEAD                                                           \
    PyObject *prefix##_loop;                                                \
    PyObject *prefix##_callback0;                                           \
    PyObject *prefix##_context0;                                            \
    PyObject *prefix##_callbacks;                                           \
    PyObject *prefix##_exception;                                           \
    PyObject *prefix##_result;                                              \
    PyObject *prefix##_source_tb;                                           \
    PyObject *prefix##_cancel_msg;                                          \
    fut_state prefix##_state;                                               \
    int prefix##_log_tb;                                                    \
    int prefix##_blocking;                                                  \
    PyObject *dict;                                                         \
    PyObject *prefix##_weakreflist;                                         \
    _PyErr_StackItem prefix##_cancelled_exc_state;
typedef struct {
    FutureObj_HEAD(fut)
} FutureObj;

#endif

#if (PY_VERSION_HEX >= 0x030b00f0)
#define CASYNCIO_PYTHON_311
/* Remove Macro if previously defined */
#ifdef FutureObj_HEAD
    #undef FutureObj_HEAD
#endif 

#define FutureObj_HEAD(prefix)                                              \
    PyObject_HEAD                                                           \
    PyObject *prefix##_loop;                                                \
    PyObject *prefix##_callback0;                                           \
    PyObject *prefix##_context0;                                            \
    PyObject *prefix##_callbacks;                                           \
    PyObject *prefix##_exception;                                           \
    PyObject *prefix##_exception_tb;                                        \
    PyObject *prefix##_result;                                              \
    PyObject *prefix##_source_tb;                                           \
    PyObject *prefix##_cancel_msg;                                          \
    fut_state prefix##_state;                                               \
    int prefix##_log_tb;                                                    \
    int prefix##_blocking;                                                  \
    PyObject *dict;                                                         \
    PyObject *prefix##_weakreflist;                                         \
    PyObject *prefix##_cancelled_exc;

typedef struct {
    FutureObj_HEAD(fut)
} FutureObj;
#endif
#if (PY_VERSION_HEX >= 0x030c00f0)
#define CASYNCIO_PYTHON_312
/* Remove Macro if previously defined */
 

#define FutureObj_HEAD(prefix)                                              \
    PyObject_HEAD                                                           \
    PyObject *prefix##_loop;                                                \
    PyObject *prefix##_callback0;                                           \
    PyObject *prefix##_context0;                                            \
    PyObject *prefix##_callbacks;                                           \
    PyObject *prefix##_exception;                                           \
    PyObject *prefix##_exception_tb;                                        \
    PyObject *prefix##_result;                                              \
    PyObject *prefix##_source_tb;                                           \
    PyObject *prefix##_cancel_msg;                                          \
    fut_state prefix##_state;                                               \
    int prefix##_log_tb;                                                    \
    int prefix##_blocking;                                                  \
    PyObject *dict;                                                         \
    PyObject *prefix##_weakreflist;                                         \
    PyObject *prefix##_cancelled_exc;

typedef struct {
    FutureObj_HEAD(fut)
} FutureObj;

#endif
#if (PY_VERSION_HEX >= 0x030d00f0)
#define CASYNCIO_PYTHON_313
/* Remove Macro if previously defined */
// #ifdef FutureObj_HEAD
//     #undef FutureObj_HEAD
// #endif 

#define FutureObj_HEAD(prefix)                                              \
    PyObject_HEAD                                                           \
    PyObject *prefix##_loop;                                                \
    PyObject *prefix##_callback0;                                           \
    PyObject *prefix##_context0;                                            \
    PyObject *prefix##_callbacks;                                           \
    PyObject *prefix##_exception;                                           \
    PyObject *prefix##_exception_tb;                                        \
    PyObject *prefix##_result;                                              \
    PyObject *prefix##_source_tb;                                           \
    PyObject *prefix##_cancel_msg;                                          \
    PyObject *prefix##_cancelled_exc;                                       \
    fut_state prefix##_state;                                               \
    /* These bitfields need to be at the end of the struct
       so that these and bitfields from TaskObj are contiguous.
    */                                                                      \
    unsigned prefix##_log_tb: 1;                                            \
    unsigned prefix##_blocking: 1;

typedef struct {
    FutureObj_HEAD(fut)
} FutureObj;

#endif
#if (PY_VERSION_HEX >= 0x030e00f0)
#define CASYNCIO_PYTHON_314
/* Remove Macro if previously defined */

#define FutureObj_HEAD(prefix)                                              \
    PyObject_HEAD                                                           \
    PyObject *prefix##_loop;                                                \
    PyObject *prefix##_callback0;                                           \
    PyObject *prefix##_context0;                                            \
    PyObject *prefix##_callbacks;                                           \
    PyObject *prefix##_exception;                                           \
    PyObject *prefix##_exception_tb;                                        \
    PyObject *prefix##_result;                                              \
    PyObject *prefix##_source_tb;                                           \
    PyObject *prefix##_cancel_msg;                                          \
    PyObject *prefix##_cancelled_exc;                                       \
    PyObject *prefix##_awaited_by;                                          \
    fut_state prefix##_state;                                               \
    /* Used by profilers to make traversing the stack from an external      \
       process faster. */                                                   \
    char prefix##_is_task;                                                  \
    char prefix##_awaited_by_is_set;                                        \
    /* These bitfields need to be at the end of the struct                  \
       so that these and bitfields from TaskObj are contiguous.             \
    */                                                                      \
    unsigned prefix##_log_tb: 1;                                            \
    unsigned prefix##_blocking: 1;                                          \
    
typedef struct {
    FutureObj_HEAD(fut)
} FutureObj;
#endif

#ifndef CASYNCIO_PYTHON_311

/* Build Doesn't want me using _PyErr_ChainStackItem 
but I have one last trick up my sleeve to make this hacked 
module work, one is called code-injection */

static inline PyObject* _PyErr_Occurred(PyThreadState *tstate)
{
    assert(tstate != NULL);
    return tstate->curexc_type;
}

void
_PyErr_Fetch(PyThreadState *tstate, PyObject **p_type, PyObject **p_value,
             PyObject **p_traceback)
{
    *p_type = tstate->curexc_type;
    *p_value = tstate->curexc_value;
    *p_traceback = tstate->curexc_traceback;

    tstate->curexc_type = NULL;
    tstate->curexc_value = NULL;
    tstate->curexc_traceback = NULL;
}

static PyObject*
_PyErr_CreateException(PyObject *exception_type, PyObject *value)
{
    PyObject *exc;

    if (value == NULL || value == Py_None) {
        exc = _PyObject_CallNoArg(exception_type);
    }
    else if (PyTuple_Check(value)) {
        exc = PyObject_Call(exception_type, value, NULL);
    }
    else {
        exc = PyObject_CallOneArg(exception_type, value);
    }

    if (exc != NULL && !PyExceptionInstance_Check(exc)) {
        PyErr_Format(PyExc_TypeError,
                     "calling %R should have returned an instance of "
                     "BaseException, not %s",
                     exception_type, Py_TYPE(exc)->tp_name);
        Py_CLEAR(exc);
    }

    return exc;
}

void
_PyErr_Restore(PyThreadState *tstate, PyObject *type, PyObject *value,
               PyObject *traceback)
{
    PyObject *oldtype, *oldvalue, *oldtraceback;

    if (traceback != NULL && !PyTraceBack_Check(traceback)) {
        /* XXX Should never happen -- fatal error instead? */
        /* Well, it could be None. */
        Py_DECREF(traceback);
        traceback = NULL;
    }

    /* Save these in locals to safeguard against recursive
       invocation through Py_XDECREF */
    oldtype = tstate->curexc_type;
    oldvalue = tstate->curexc_value;
    oldtraceback = tstate->curexc_traceback;

    tstate->curexc_type = type;
    tstate->curexc_value = value;
    tstate->curexc_traceback = traceback;

    Py_XDECREF(oldtype);
    Py_XDECREF(oldvalue);
    Py_XDECREF(oldtraceback);
}

void
_PyErr_SetObject(PyThreadState *tstate, PyObject *exception, PyObject *value)
{
    PyObject *exc_value;
    PyObject *tb = NULL;

    if (exception != NULL &&
        !PyExceptionClass_Check(exception)) {
        PyErr_Format(PyExc_SystemError,
                      "_PyErr_SetObject: "
                      "exception %R is not a BaseException subclass",
                      exception);
        return;
    }

    Py_XINCREF(value);
    exc_value = _PyErr_GetTopmostException(tstate)->exc_value;
    if (exc_value != NULL && exc_value != Py_None) {
        /* Implicit exception chaining */
        Py_INCREF(exc_value);
        if (value == NULL || !PyExceptionInstance_Check(value)) {
            /* We must normalize the value right now */
            PyObject *fixed_value;

            /* Issue #23571: functions must not be called with an
               exception set */
            _PyErr_Restore(tstate, NULL, NULL, NULL);

            fixed_value = _PyErr_CreateException(exception, value);
            Py_XDECREF(value);
            if (fixed_value == NULL) {
                Py_DECREF(exc_value);
                return;
            }

            value = fixed_value;
        }

        /* Avoid creating new reference cycles through the
           context chain, while taking care not to hang on
           pre-existing ones.
           This is O(chain length) but context chains are
           usually very short. Sensitive readers may try
           to inline the call to PyException_GetContext. */
        if (exc_value != value) {
            PyObject *o = exc_value, *context;
            PyObject *slow_o = o;  /* Floyd's cycle detection algo */
            int slow_update_toggle = 0;
            while ((context = PyException_GetContext(o))) {
                Py_DECREF(context);
                if (context == value) {
                    PyException_SetContext(o, NULL);
                    break;
                }
                o = context;
                if (o == slow_o) {
                    /* pre-existing cycle - all exceptions on the
                       path were visited and checked.  */
                    break;
                }
                if (slow_update_toggle) {
                    slow_o = PyException_GetContext(slow_o);
                    Py_DECREF(slow_o);
                }
                slow_update_toggle = !slow_update_toggle;
            }
            PyException_SetContext(value, exc_value);
        }
        else {
            Py_DECREF(exc_value);
        }
    }
    if (value != NULL && PyExceptionInstance_Check(value))
        tb = PyException_GetTraceback(value);
    Py_XINCREF(exception);
    _PyErr_Restore(tstate, exception, value, tb);
}

void
_PyErr_ChainStackItem(_PyErr_StackItem *exc_info)
{
    PyThreadState *tstate = PyThreadState_GET();
    assert(_PyErr_Occurred(tstate));

    int exc_info_given;
    if (exc_info == NULL) {
        exc_info_given = 0;
        exc_info = tstate->exc_info;
    } else {
        exc_info_given = 1;
    }
    if (exc_info->exc_type == NULL || exc_info->exc_type == Py_None) {
        return;
    }

    _PyErr_StackItem *saved_exc_info;
    if (exc_info_given) {
        /* Temporarily set the thread state's exc_info since this is what
           _PyErr_SetObject uses for implicit exception chaining. */
        saved_exc_info = tstate->exc_info;
        tstate->exc_info = exc_info;
    }

    PyObject *exc, *val, *tb;
    _PyErr_Fetch(tstate, &exc, &val, &tb);

    PyObject *exc2, *val2, *tb2;
    exc2 = exc_info->exc_type;
    val2 = exc_info->exc_value;
    tb2 = exc_info->exc_traceback;
    PyErr_NormalizeException(&exc2, &val2, &tb2);
    if (tb2 != NULL) {
        PyException_SetTraceback(val2, tb2);
    }

    /* _PyErr_SetObject sets the context from PyThreadState. */
    _PyErr_SetObject(tstate, exc, val);
    Py_DECREF(exc);  // since _PyErr_Occurred was true
    Py_XDECREF(val);
    Py_XDECREF(tb);

    if (exc_info_given) {
        tstate->exc_info = saved_exc_info;
    }
}

#endif 

/* XXX Can't use macros on FutureObj_HEAD Due to the way C Macros work but we can hardcode it in still */
/* We can copy and paste this work into TaskObj */



// typedef struct {
// #ifndef CASYNCIO_PYTHON_313
//     PyObject *task_loop;
//     PyObject *task_callback0;
//     PyObject *task_context0;
//     PyObject *task_callbacks;
//     PyObject *task_exception;
// #endif /* CASYNCIO_PYTHON_313 */
// #ifdef CASYNCIO_PYTHON_310
//     PyObject *task_exception_tb;
// #endif /* CASYNCIO_PYTHON_310 */
//     PyObject *task_result;
//     PyObject *task_source_tb;
//     PyObject *task_cancel_msg;
//     fut_state task_state;
//     int task_log_tb;
//     int task_blocking;
//     PyObject *dict;
//     PyObject *task_weakreflist;
// #if CASYNCIO_PYTHON_311
//     PyObject *task_cancelled_exc;
// #else
//     /* XXX: Altered after 3.10 */
//     _PyErr_StackItem task_cancelled_exc;
// #endif /* CASYNCIO_PYTHON_311 */

// #ifdef CASYNCIO_PYTHON_313
//     /* 313 Takes a Massive leap that is too impressive to organize */
//     PyObject *task_loop;
//     PyObject *task_callback0;
//     PyObject *task_context0;
//     PyObject *task_callbacks;
//     PyObject *task_exception;
//     PyObject *task_exception_tb;
//     PyObject *task_result;
//     PyObject *task_source_tb;
//     PyObject *task_cancel_msg;
//     PyObject *task_cancelled_exc;
//     fut_state task_state;
//     /* These bitfields need to be at the end of the struct
//        so that these and bitfields from TaskObj are contiguous.
//     */
//     unsigned task_log_tb : 1;
//     unsigned task_blocking : 1;

// #endif /* CASYNCIO_PYTHON_313 */
// #ifndef CASYNCIO_PYTHON_313
//     PyObject *task_fut_waiter;
//     PyObject *task_coro;
//     PyObject *task_name;
//     PyObject *task_context;
//     int task_must_cancel;
//     int task_log_destroy_pending;
// #endif
// #if CASYNCIO_PYTHON_311
//     int task_num_cancels_requested;
// #endif
// #if CASYNCIO_PYTHON_313 
//     /* 3.13 still takes massive leaps and makes crazy changes */
//     unsigned task_must_cancel: 1;
//     unsigned task_log_destroy_pending: 1;
//     int task_num_cancels_requested;
//     PyObject *task_fut_waiter;
//     PyObject *task_coro;
//     PyObject *task_name;
//     PyObject *task_context;
// #endif 
// } TaskObj;



/* 
Going to deviate from CPython's implementations a little bit Cython
Can Strictly take care of preventing unwated types 
from leaking through

- This C-API Assumes that the object 
  is that type & it exists & Cython
  will recast it as that object by 
  default.


*/

// Couldn't seem to get _Py_IDENTIFIER to work correctly
// so I had to come up with a comprimise.

#define CASYNCIO_IDENTIFIER(name) static _Py_Identifier PyId_##name = {#name, -1}

// ==================================== GLOBALS ====================================

// TODO: Any globals we borrow should be setup in a hacky import_XX function later...
PyObject* context_kwname; /* "context" in unicode */
PyObject* asyncio_InvalidStateError; /* asyncio.CancelledError */
PyObject* asyncio_CancelledError; /* asyncio.CancelledError */


PyTypeObject* FutureType; /* _asyncio.Future */
PyTypeObject* TaskType; /* _asyncio.Task */

CASYNCIO_IDENTIFIER(call_soon);

CASYNCIO_IDENTIFIER(call_soon_threadsafe);

// TODO: Check 3.9-3.13 for anything else or code-changes


static int 
Loop_CallSoon(PyObject* loop, PyObject* func, PyObject* arg, PyObject* ctx){
#ifndef CASYNCIO_PYTHON_311
    PyObject *handle;
    PyObject *stack[3];
    Py_ssize_t nargs;

    if (ctx == NULL) {
        handle = _PyObject_CallMethodIdObjArgs(
            loop, &PyId_call_soon, func, arg, NULL);
    }
    else {
        /* Use FASTCALL to pass a keyword-only argument to call_soon */

        PyObject *callable = _PyObject_GetAttrId(loop, &PyId_call_soon);
        if (callable == NULL) {
            return -1;
        }

        /* All refs in 'stack' are borrowed. */
        nargs = 1;
        stack[0] = func;
        if (arg != NULL) {
            stack[1] = arg;
            nargs++;
        }
        stack[nargs] = (PyObject *)ctx;

        handle = PyObject_Vectorcall(callable, stack, nargs, context_kwname);
        Py_DECREF(callable);
    }

    if (handle == NULL) {
        return -1;
    }
    Py_DECREF(handle);
    return 0;
#else /* 3.12+ */
    /* IDK if _Py_ID or any functionality is Private to CPython only 
       Feel free to throw an issue on github if you find something 
       Unaccessable or requires new objects or macros to be added.
    */
    PyObject *handle;

    if (ctx == NULL) {
        PyObject *stack[] = {loop, func, arg};
        size_t nargsf = 3 | PY_VECTORCALL_ARGUMENTS_OFFSET;
        handle = PyObject_VectorcallMethod(&_Py_ID(call_soon), stack, nargsf, NULL);
    }
    else {
        /* All refs in 'stack' are borrowed. */
        PyObject *stack[4];
        size_t nargs = 2;
        stack[0] = loop;
        stack[1] = func;
        if (arg != NULL) {
            stack[2] = arg;
            nargs++;
        }
        stack[nargs] = (PyObject *)ctx;
        size_t nargsf = nargs | PY_VECTORCALL_ARGUMENTS_OFFSET;
        handle = PyObject_VectorcallMethod(&_Py_ID(call_soon), stack, nargsf,
                                           context_kwname);
    }

    if (handle == NULL) {
        return -1;
    }
    Py_DECREF(handle);
    return 0;
#endif

}

/* This function is custom and I modified it for MultiFuture */

static int 
Loop_CallSoonThreadSafe(PyObject* loop, PyObject* func, PyObject* arg, PyObject* ctx){
#ifndef CASYNCIO_PYTHON_311
    PyObject *handle;
    PyObject *stack[3];
    Py_ssize_t nargs;

    if (ctx == NULL) {
        handle = _PyObject_CallMethodIdObjArgs(
            loop, &PyId_call_soon_threadsafe, func, arg, NULL);
    }
    else {
        /* Use FASTCALL to pass a keyword-only argument to call_soon */

        PyObject *callable = _PyObject_GetAttrId(loop, &PyId_call_soon_threadsafe);
        if (callable == NULL) {
            return -1;
        }

        /* All refs in 'stack' are borrowed. */
        nargs = 1;
        stack[0] = func;
        if (arg != NULL) {
            stack[1] = arg;
            nargs++;
        }
        stack[nargs] = (PyObject *)ctx;

        handle = PyObject_Vectorcall(callable, stack, nargs, context_kwname);
        Py_DECREF(callable);
    }

    if (handle == NULL) {
        return -1;
    }
    Py_DECREF(handle);
    return 0;
#else /* 3.12+ */
    /* IDK if _Py_ID or any functionality is Private to CPython only 
       Feel free to throw an issue on github if you find something 
       Unaccessable or requires new objects or macros to be added.
    */
    PyObject *handle;

    if (ctx == NULL) {
        PyObject *stack[] = {loop, func, arg};
        size_t nargsf = 3 | PY_VECTORCALL_ARGUMENTS_OFFSET;
        handle = PyObject_VectorcallMethod(&_Py_ID(call_soon_threadsafe), stack, nargsf, NULL);
    }
    else {
        /* All refs in 'stack' are borrowed. */
        PyObject *stack[4];
        size_t nargs = 2;
        stack[0] = loop;
        stack[1] = func;
        if (arg != NULL) {
            stack[2] = arg;
            nargs++;
        }
        stack[nargs] = (PyObject *)ctx;
        size_t nargsf = nargs | PY_VECTORCALL_ARGUMENTS_OFFSET;
        handle = PyObject_VectorcallMethod(&_Py_ID(call_soon_threadsafe), stack, nargsf,
                                           context_kwname);
    }

    if (handle == NULL) {
        return -1;
    }
    Py_DECREF(handle);
    return 0;
#endif
}

// NOTE: FutureObj_GetLoop could be inlined in a pxd file so I didn't write it
// If you want me to rewrite and addd it back in throw me an issue on github
// if you wanted to borrow it for anything else...


static inline int FutureObj_IsAlive(FutureObj* fut)
{
    return fut->fut_loop != NULL;
}

static inline PyObject* FutureObj_GetLoop(FutureObj* fut){
    return fut->fut_loop;
}

static inline int FutureObj_EnsureAlive(FutureObj *fut)
{
    if (!FutureObj_IsAlive(fut)) {
        PyErr_SetString(PyExc_RuntimeError,
                        "Future object is not initialized.");
        return -1;
    }
    return 0;
}

static int 
FutureObj_ScheduleCallbacks(FutureObj* fut){
    Py_ssize_t len;
    Py_ssize_t i;

    if (fut->fut_callback0 != NULL) {
        /* CPYTHON COMMENT: There's a 1st callback */
        #ifdef CASYNCIO_PYTHON_312
        /* 3.12+ */

        // CPYTHON WARNING! 
        // Beware: An evil call_soon could alter fut_callback0 or fut_context0.
        // Since we are anyway clearing them after the call, whether call_soon
        // succeeds or not, the idea is to transfer ownership so that external
        // code is not able to alter them during the call.
        PyObject *fut_callback0 = fut->fut_callback0;
        fut->fut_callback0 = NULL;
        PyObject *fut_context0 = fut->fut_context0;
        fut->fut_context0 = NULL;

        // XXX: We don't utilize a state since 
        // were borrowing to hack in optimized code.
        int ret = Loop_CallSoon(
            fut->fut_loop, fut_callback0,
            (PyObject *)fut, fut_context0);
        
        
        #else
        /* 3.9 - 3.11 */
        int ret = Loop_CallSoon(
            fut->fut_loop, fut->fut_callback0,
            (PyObject *)fut, fut->fut_context0);
        
        #endif /* CASYNCIO_PYTHON_312 */


        Py_CLEAR(fut->fut_callback0);
        Py_CLEAR(fut->fut_context0);
        if (ret) {
            /* If an error occurs in pure-Python implementation,
               all callbacks are cleared. */
            Py_CLEAR(fut->fut_callbacks);
            return ret;
        }

        /* we called the first callback, now try calling
           callbacks from the 'fut_callbacks' list. */
    }

    if (fut->fut_callbacks == NULL) {
        /* CPYTHON COMMENT: No more callbacks, return. */
        return 0;
    }

    #ifndef CASYNCIO_PYTHON_312

    /* 3.9 - 3.11 */

    len = PyList_GET_SIZE(fut->fut_callbacks);
    if (len == 0) {
        /* CPYTHON COMMENT: The list of callbacks was empty; clear it and return. */
        Py_CLEAR(fut->fut_callbacks);
        return 0;
    }

    for (i = 0; i < len; i++) {
        PyObject *cb_tup = PyList_GET_ITEM(fut->fut_callbacks, i);
        PyObject *cb = PyTuple_GET_ITEM(cb_tup, 0);
        PyObject *ctx = PyTuple_GET_ITEM(cb_tup, 1);

        if (Loop_CallSoon(fut->fut_loop, cb, (PyObject *)fut, ctx)) {
            /* If an error occurs in pure-Python implementation,
               all callbacks are cleared. */
            Py_CLEAR(fut->fut_callbacks);
            return -1;
        }
    }
    Py_CLEAR(fut->fut_callbacks);
    return 0; 
    #else 
    /* ===== 3.12+ ===== */
    
    // CPYTHON WARNING! 
    // Beware: An evil call_soon could change fut->fut_callbacks.
    // The idea is to transfer the ownership of the callbacks list
    // so that external code is not able to mutate the list during
    // the iteration.
    PyObject *callbacks = fut->fut_callbacks;
    fut->fut_callbacks = NULL;
    Py_ssize_t n = PyList_GET_SIZE(callbacks);
    for (Py_ssize_t i = 0; i < n; i++) {
        assert(PyList_GET_SIZE(callbacks) == n);
        PyObject *cb_tup = PyList_GET_ITEM(callbacks, i);
        PyObject *cb = PyTuple_GET_ITEM(cb_tup, 0);
        PyObject *ctx = PyTuple_GET_ITEM(cb_tup, 1);

        if (Loop_CallSoon(state, fut->fut_loop, cb, (PyObject *)fut, ctx)) {
            Py_DECREF(callbacks);
            return -1;
        }
    }
    Py_DECREF(callbacks);
    return 0;
    
    #endif
}

// one change I did make was that some return types are integers instead of PyObjects
// so that a few extra performance modifications could be made later...

static int
FutureObj_SetResult(FutureObj *fut, PyObject *res)
{
    if (FutureObj_EnsureAlive(fut)) {
        return -1;
    }

    if (fut->fut_state != STATE_PENDING) {
        PyErr_SetString(asyncio_InvalidStateError, "invalid state");
        return -1;
    }
    assert(!fut->fut_result);

    #ifndef CASYNCIO_PYTHON_312
    /* 3.9 - 3.11 */
    Py_INCREF(res);
    fut->fut_result = res;
    #else /* 3.12+ */
    fut->fut_result = Py_NewRef(res);
    #endif
    fut->fut_state = STATE_FINISHED;
    if (FutureObj_ScheduleCallbacks(fut) == -1) {
        return -1;
    }
    return 0;
}



static int 
FutureObj_SetException(FutureObj *fut, PyObject *exc)
{
    PyObject *exc_val = NULL;

    if (fut->fut_state != STATE_PENDING) {
        PyErr_SetString(asyncio_InvalidStateError, "invalid state");
        return -1;
    }

    if (PyExceptionClass_Check(exc)) {
        exc_val = PyObject_CallNoArgs(exc);
        if (exc_val == NULL) {
            return -1;
        }
        if (fut->fut_state != STATE_PENDING) {
            Py_DECREF(exc_val);
            PyErr_SetString(asyncio_InvalidStateError, "invalid state");
            return -1;
        }
    }
    else {
        exc_val = exc;
        Py_INCREF(exc_val);
    }
    if (!PyExceptionInstance_Check(exc_val)) {
        Py_DECREF(exc_val);
        PyErr_SetString(PyExc_TypeError, "invalid exception object");
        return 0;
    }
    if (Py_IS_TYPE(exc_val, (PyTypeObject *)PyExc_StopIteration)) {
        Py_DECREF(exc_val);
        PyErr_SetString(PyExc_TypeError,
                        "StopIteration interacts badly with generators "
                        "and cannot be raised into a Future");
        return -1;
    }

    assert(!fut->fut_exception);
    #ifdef CASYNCIO_PYTHON_310
    /* 3.10 */  
    assert(!fut->fut_exception_tb);
    #endif 
    fut->fut_exception = exc_val;
    fut->fut_exception_tb = PyException_GetTraceback(exc_val);
    fut->fut_state = STATE_FINISHED;

    if (FutureObj_ScheduleCallbacks(fut) == -1) {
        return -1;
    }

    fut->fut_log_tb = 1;
    return 0;
}

/* Combined create_cancelled_error + future_set_cancelled_error 
 * to be more optimized and organized */

static void
FutureObj_SetCancelledError(FutureObj* fut){

   
    #ifdef CASYNCIO_PYTHON_311
    /* 3.11+ */
    PyObject *exc;
    if (fut->fut_cancelled_exc != NULL) {
        /* transfer ownership */
        exc = fut->fut_cancelled_exc;
        fut->fut_cancelled_exc = NULL;
        return exc;
    }
    PyObject *msg = fut->fut_cancel_msg;
    if (msg == NULL || msg == Py_None) {
        exc = PyObject_CallNoArgs(asyncio_CancelledError);
    } else {
        exc = PyObject_CallOneArg(asyncio_CancelledError, msg);
    }
    if (exc == NULL){
        return;
    }
    PyErr_SetObject(asyncio_CancelledError, exc);
    Py_DECREF(exc);

    #else /* 3.9 - 3.10 */
    PyObject *exc, *msg;
    msg = fut->fut_cancel_msg;
    if (msg == NULL || msg == Py_None) {
        exc = PyObject_CallNoArgs(asyncio_CancelledError);
    } else {
        exc = PyObject_CallOneArg(asyncio_CancelledError, msg);
    }
    PyErr_SetObject(asyncio_CancelledError, exc);
    /* were done using the exception object so release... */
    Py_DECREF(exc);
    _PyErr_ChainStackItem(&fut->fut_cancelled_exc_state);
    #endif
}

static int 
FutureObj_GetResult(FutureObj* fut, PyObject** result){
    // made more sense to use a switchcase block here than what CPython has.
    switch (fut->fut_state)
    {
        case STATE_CANCELLED: {
            FutureObj_SetCancelledError(fut);
            return -1;
        } 

        case STATE_PENDING: {
            PyErr_SetString(asyncio_InvalidStateError, "Result is not set.");
            return -1;
        }
        
        case STATE_FINISHED: {
            fut->fut_log_tb = 0;
            if (fut->fut_exception != NULL){
                Py_INCREF(fut->fut_exception);
                *result = fut->fut_exception;
                return 1;
            }
            Py_INCREF(fut->fut_result);
            *result = fut->fut_result;
            return 0;
        }
        default:
            /* UNREACHABLE */
            assert(0);
    }
}


static int
FutureObj_AddDoneCallback(FutureObj *fut, PyObject *arg, PyObject *ctx)
{
    if (ctx == NULL){
        /* get the context */
        ctx = PyContext_CopyCurrent();
        if (ctx == NULL){
            return -1;
        }
    }
    if (!FutureObj_IsAlive(fut)) {
        PyErr_SetString(PyExc_RuntimeError, "uninitialized Future object");
        return -1;
    }
    if (fut->fut_state != STATE_PENDING) {
        /* CPYTHON COMMENT: The future is done/cancelled, so schedule the callback
           right away. */
        if (Loop_CallSoon(fut->fut_loop, arg, (PyObject*) fut, ctx)) {
            return -1;
        }
    } else {
        // Large Comment was kind of ignorable I would guess you understand 
        // that the first callback is the optimized object otherwise 
        // the list takes it's place
        if (fut->fut_callbacks == NULL && fut->fut_callback0 == NULL) {
            Py_INCREF(arg);
            fut->fut_callback0 = arg;
            
            Py_INCREF(ctx);
            fut->fut_context0 = ctx;
        }
        else {

            PyObject *tup = PyTuple_New(2);
            if (tup == NULL) {
                return -1;
            }
            Py_INCREF(arg);
            PyTuple_SET_ITEM(tup, 0, arg);
            Py_INCREF(ctx);

            PyTuple_SET_ITEM(tup, 1, (PyObject *)ctx);

            if (fut->fut_callbacks != NULL) {
                int err = PyList_Append(fut->fut_callbacks, tup);
                if (err) {
                    Py_DECREF(tup);
                    return -1;
                }
                Py_DECREF(tup);
            }
            else {
                fut->fut_callbacks = PyList_New(1);
                if (fut->fut_callbacks == NULL) {
                    return -1;
                }

                PyList_SET_ITEM(fut->fut_callbacks, 0, tup);  /* borrow */
            }
        }
    }
    return 0;
}

// Will use numbers for these ones but basically it goes like this...
// 1 = success
// 0 = fail
// -1 = error
static int 
FutureObj_CancelWithMessage(FutureObj* fut, PyObject* msg){
    fut->fut_log_tb = 0;

    if (fut->fut_state != STATE_PENDING) {
        return 0;
    }
    fut->fut_state = STATE_CANCELLED;
    Py_XINCREF(msg);
    Py_XSETREF(fut->fut_cancel_msg, msg);

    return (FutureObj_ScheduleCallbacks(fut) < 0) ? -1: 1;
}

// Cancel Future with No Message it will be set as NULL.
static int 
FutureObj_Cancel(FutureObj* fut){
    fut->fut_log_tb = 0;

    if (fut->fut_state != STATE_PENDING) {
        return 0;
    }
    fut->fut_state = STATE_CANCELLED;
    fut->fut_cancel_msg = NULL;
    return (FutureObj_ScheduleCallbacks(fut) < 0) ? -1: 1;
}

// This function is a combined version of __new__ & __init__
// It's assumed asynciomodule.c already tied these objects together.
// This will also take care of the costly calls & globals and lets asynciomodule.c 
// do the heavy lifiting for us. 
// WARNING: IF LOOP IS NULL THIS FUNCTION IS COSTLY AND MAY CAP PERFORAMANCE BENEFITS!!!
// WARNING: This is not a good idea if you have a cutsom task-factory in action!

// TODO: Fix this
// static PyObject* 
// FutureObj_New(){
//     PyObject* fut = FutureType->tp_alloc(FutureType, 0);
//     return (FutureType->tp_init(fut, PyTuple_New(0), PyDict_New()) < 0) ? NULL : fut;
// fail:
//     Py_CLEAR(fut);
//     return NULL;
// }

// Can be slower but beneficial when code can't directly get to the current eventloop.
#define FutureObj_NewNoArgs FutureObj_New


static int 
FutureObj_IsDone(FutureObj* fut){
    return fut->fut_state == STATE_FINISHED;
}

static int 
FutureObj_IsCancelled(FutureObj* fut){
    return fut->fut_state == STATE_CANCELLED;
}



/* ============================== MODULE IMPORT ============================== */
/* This is the main function involved in obtaining all required globals 
Currently there is no implementation for a Python Capsule as no module is 
required currently however that doesn't mean there's a catch. 
We still need the globals otherwise they will all be NULL and cause an immediate 
segfault and I can't save you if you fail to do this simple but important step!
*/


/* Imports c-asyncio globals 

Exteremely important you do first thing in any application even in C itself and is not
limited to just cython. returns 0 if success, -1 on failure.

*/
static int 
Import_CAsyncio(){
    PyObject *module = NULL;
    // Import as Python Objects and then recast to Types...
    PyObject *FutureTypeObj;
    PyObject *TaskTypeObj;

// From CPython's WITH_MOD / GET_MOD_ATTR Macros

#define CASYNCIO_WITH_MOD(NAME) \
    Py_CLEAR(module); \
    module = PyImport_ImportModule(NAME); \
    if (module == NULL) { \
        goto fail; \
    }

#define CASYNCIO_GET_MOD_ATTR(VAR, NAME) \
    VAR = PyObject_GetAttrString(module, NAME); \
    if (VAR == NULL) { \
        goto fail; \
    }

    context_kwname = Py_BuildValue("(s)", "context");
    if (context_kwname == NULL) {
        goto fail;
    }

    CASYNCIO_WITH_MOD("asyncio.exceptions")
    CASYNCIO_GET_MOD_ATTR(asyncio_InvalidStateError, "InvalidStateError")
    CASYNCIO_GET_MOD_ATTR(asyncio_CancelledError, "CancelledError")

    /* Yank the target Module were hacking objects from 
     * If _asyncio was not compiled on the target system 
     * Everything I invented fails. */

    CASYNCIO_WITH_MOD("_asyncio")

    /* _asyncio.Future */
    CASYNCIO_GET_MOD_ATTR(FutureTypeObj, "Future");
    
    /* _asyncio.Task */
    CASYNCIO_GET_MOD_ATTR(TaskTypeObj, "Task");
    FutureType = Py_TYPE(FutureTypeObj);
    TaskType = Py_TYPE(TaskTypeObj);
    return 0;

fail:

    Py_CLEAR(context_kwname);
    Py_CLEAR(asyncio_InvalidStateError);
    Py_CLEAR(asyncio_CancelledError);

    Py_CLEAR(FutureType); /* _asyncio.Future */
    Py_CLEAR(TaskType); /* _asyncio.Task */

    return -1;
}


#ifdef __cplusplus
}
#endif

#endif // __CASYNCIO_H__