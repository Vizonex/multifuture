#include <Python.h>
#include "atomic_compat.h"

// I'm borrowing from the xxmodule.c template to help

static PyObject* CancelledError;
static PyObject* InvalidStateError;


typedef struct _multifut_state mod_state;

typedef enum _fut_state {
    STATE_RUNNING = 0,
    STATE_CANCELLED = 1,
    STATE_FINISHED = 2
} fut_state;

const char* fut_state_to_name(fut_state state){
    switch(state){
        case STATE_RUNNING: return "RUNNING";
        case STATE_CANCELLED: return "CANCELLED";
        case STATE_FINISHED: return "FINISHED";
        default: return "UNREACHABLE";
    }
}


typedef struct {
    PyObject_HEAD
    PyObject* _result;
    PyObject* _exception;
    PyObject* _exc_handler;

    /* Callbacks to invoke, if NULL, Ignore calling this */
    PyObject* _callbacks;

    /* Callbacks for communication with other future objects */
    PyObject* _futures;

    /* Atomic flag varaible */
    uint8_t _flag;


    /* keeps a reference of the state in order to remain threadsafe */
    mod_state* state;
} MultiFutureObject;

struct _multifut_state {
    PyObject* CancelledError;
    PyObject* InvalidStateError;
    PyTypeObject *MultiFut_Type;
    
    /* "cancel" as a preloaded string */
    PyObject* asyncio_cancel;
    /* "set_result" as a preloaded string */
    PyObject* asyncio_set_result;
    /* "set_exception" as a preloaded string */
    PyObject* asyncio_set_exception;
    PyObject* asyncio_get_loop;
    PyObject* asyncio_call_soon_threadsafe;
    PyObject* asyncio_add_done_callback;
    PyObject* create_future;

    PyObject* cancelled;
    
    /* asyncio.get_event_loop function */
    PyObject* get_event_loop;



    
};


static PyModuleDef multifut_module;

static inline mod_state *
get_mod_state(PyObject *mod)
{
    mod_state *state = (mod_state *)PyModule_GetState(mod);
    assert(state != NULL);
    return state;
}

static inline mod_state *
get_mod_state_by_def(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject *mod = PyType_GetModuleByDef(tp, &multifut_module);
    assert(mod != NULL);
    return get_mod_state(mod);
}


#define MultiFutureObject_CAST(op) ((MultiFutureObject*)(op))
#define MultiFutureObject_CheckExact(v, state)  \
    Py_IS_TYPE(v, &state->MultiFut_Type)
#define MultiFutureObject_Check(v, state) \
    (MultiFutureObject_CheckExact(v, state) || \
    PyObject_TypeCheck(v,  &state->MultiFut_Type))

/* ============================ MultiFut Methods ============================ */

/* mf means it belongs to the CAPI, if it's written in full then it's not */

typedef int (*MultiFut_Callback)(MultiFutureObject* self, PyObject* obj);

/* Can be used for traversing over the objects or optimizing communication or chains */
static int __mf_execute_callbacks(MultiFutureObject *self, MultiFut_Callback func_cb, MultiFut_Callback fut_cb){
    if (self->_callbacks != NULL){
        /* BEWARE AN EVIL: we replace callbacks with NULL to prevent
         * other threads from having the possibility of accessing this 
         * group */
        PyObject* callbacks = self->_callbacks;
        self->_callbacks = NULL;

        Py_ssize_t size = PyList_GET_SIZE(callbacks);
        for (Py_ssize_t i = 0; i < size; i++){
            PyObject* cb = PyList_GET_ITEM(callbacks, i);
            if (cb != NULL){
                if (func_cb(self, cb) < 0){
                    /* Assume execption was raised already */
                    return -1;
                }
            }
        }
        /* Let GC Play with the python list now */
        Py_CLEAR(callbacks);
    }

    if (self->_futures != NULL){
        PyObject* futures = self->_futures;
        self->_futures = NULL;
        Py_ssize_t size = PyList_GET_SIZE(futures);
        for (Py_ssize_t i = 0; i < size; i++){
            PyObject* fut = PyList_GET_ITEM(futures, i);
            if (fut != NULL){
                if (fut_cb(self, fut) < 0){
                    /* Assume execption was raised already */
                    return -1;
                }
            }
        }
        /* Let GC Play with the python list now */
        Py_CLEAR(futures);
    }
    return 0;
}

static inline int 
__mf_handle_exception(MultiFutureObject* self){
    PyObject* exc = PyErr_Occurred();

    /* see if there was a memory access violation first */
    if (exc == NULL) return -1;

    if (!PyErr_GivenExceptionMatches(PyExc_Exception, exc)){
        /* Only real possibile explanation 
         * is keyboard interruption by an outside signal
         * will want to exit immediately if this is the case */
        return -1;
    }
    if (self->_exc_handler == NULL){
        PyErr_Print();

    } else if (PyObject_CallOneArg(self->_exc_handler, exc) == NULL){
        return -1;
    }

    return 0;
}

static inline int __future_call_soon_threadsafe(mod_state* state, PyObject* fut, PyObject* method){
    PyObject* loop = PyObject_CallMethodNoArgs(fut, state->asyncio_get_loop);
    if (loop == NULL){
        loop = PyObject_GetAttrString(fut, "_loop");
        if (loop == NULL){
            return -1;
        }
        PyErr_Clear();
    }
    PyObject* func_to_call = PyObject_GetAttr(fut, method); 
    if (func_to_call == NULL){
        return -1;
    }
    PyObject* data = PyObject_CallMethodOneArg(loop, state->asyncio_call_soon_threadsafe, func_to_call);
    if (data == NULL){
        return -1;
    };
    return 0;
}

static inline int __future_call_soon_threadsafe_one_arg(mod_state* state, PyObject* fut, PyObject* method, PyObject* arg){
    PyObject* loop = PyObject_CallMethodNoArgs(fut, state->asyncio_get_loop);
    if (loop == NULL){
        loop = PyObject_GetAttrString(fut, "_loop");
        if (loop == NULL){
            return -1;
        }
        PyErr_Clear();
    }
    PyObject* func_to_call = PyObject_GetAttr(fut, method); 
    if (func_to_call == NULL){
        return -1;
    }
    PyObject* data = PyObject_CallMethodObjArgs(loop, state->asyncio_call_soon_threadsafe, func_to_call, arg);
    if (data == NULL){
        return -1;
    };
    return 0;
}


static inline int 
__mf_cancel_fut(MultiFutureObject* self, PyObject* fut){
    int result;
    /* up refcount of the future temporarly before calling it off */

    Py_INCREF(fut);
    result = __future_call_soon_threadsafe(self->state, fut, self->state->asyncio_cancel);
    Py_DECREF(fut);
    if (result < 0){
        return __mf_handle_exception(self);
    }
    return 0;
}

static inline int 
__mf_fut_set_result(MultiFutureObject* self, PyObject* fut){
    int result;
    /* up refcount of the future temporarly before calling it off */
    Py_INCREF(fut);
    result = __future_call_soon_threadsafe_one_arg(self->state, fut, self->state->asyncio_set_result, Py_NewRef(self->_result));
    Py_DECREF(fut);
    if (result < 0){
        return __mf_handle_exception(self);
    }
    return 0;
}

static inline int 
__mf_fut_set_exception(MultiFutureObject* self, PyObject* fut){
    int result;
    /* up refcount of the future temporarly before calling it off */
    Py_INCREF(fut);
    result =  __future_call_soon_threadsafe_one_arg(self->state, fut, self->state->asyncio_set_exception, Py_NewRef(self->_exception));
    Py_DECREF(fut);
    if (result < 0){
        return __mf_handle_exception(self);
    }
    return 0;
}



static inline int 
__mf_cb(MultiFutureObject* self, PyObject* cb){
    if (PyObject_CallOneArg(cb, self) != NULL){
        return __mf_handle_exception(self);
    }
    return 0;
}


static inline int
mf_cancel(MultiFutureObject* self, PyObject* msg){
    if (atomic_load_uint8(&self->_flag) != STATE_RUNNING){
        /* False */
        return 0;
    }

    (void)atomic_exhange_uint8(&self->_flag, (uint8_t)STATE_CANCELLED);
    if (msg != NULL){
        self->_exception = PyObject_CallOneArg(self->state->CancelledError, msg);
    } else {
        self->_exception = PyObject_CallNoArgs(self->state->CancelledError);
    }

    if (self->_exception == NULL){
        /* Exception did not succeed initalization */
        return -1;
    }
    
    if (__mf_execute_callbacks(self, __mf_cb, __mf_cancel_fut) < 0){
        return -1;
    }
    /* True */
    return 1;
}


static inline int 
mf_set_result(MultiFutureObject* self, PyObject* result){
    fut_state state = (fut_state)atomic_load_uint8(&self->_flag);
    if (state != STATE_RUNNING){
        PyErr_Format(self->state->InvalidStateError, "Invalid State Got %s", fut_state_to_name(state));
        return -1;
    }
    (void)atomic_exhange_uint8(&self->_flag, STATE_FINISHED);
    self->_result = result;
    return __mf_execute_callbacks(self, __mf_cb, __mf_fut_set_result);
}


static inline int 
mf_set_exception(MultiFutureObject* self, PyObject* exception){
    fut_state state = (fut_state)atomic_load_uint8(self->_flag);
    if (state != STATE_RUNNING){
        PyErr_Format(self->state->InvalidStateError, "Invalid State Got %s", fut_state_to_name(state));
        return -1;
    }
    self->_exception = exception;
    return __mf_execute_callbacks(self, __mf_cb, __mf_fut_set_exception);
}


static inline int
mf_exception(MultiFutureObject* self, PyObject** exc){
    switch ((fut_state)atomic_load_uint8(self->_flag)){
        case STATE_CANCELLED: {
            PyErr_SetNone(self->_exception);
            exc = NULL;
            return -1;
        }
        case STATE_RUNNING: {
            PyErr_SetString(self->state->InvalidStateError, "Exception is not set.");
            exc = NULL;
            return -1;
        }
        /* SUCCESS */
        default: {
            if (self->_exception != NULL){
                *exc = Py_NewRef(self->_exception);
                /* Obtained Exception */
                return 1;
            }
            break;
        }
    }
    return 0;
}


static inline int
mf_result(MultiFutureObject* self, PyObject** result){
    switch ((fut_state)atomic_load_uint8(self->_flag)){
        case STATE_CANCELLED: {
            PyErr_SetNone(self->_exception);
            result = NULL;
            return -1;
        }
        case STATE_RUNNING: {
            PyErr_SetString(self->state->InvalidStateError, "Result is not ready.");
            result = NULL;
            return -1;
        }
        /* SUCCESS */
        default: {
            if (self->_exception != NULL){
                PyErr_SetNone(self->_exception);
                return -1;
            }
            *result = Py_NewRef(self->_result);
        }
    }
    return 0;
}

static inline int
mf_add_done_callback(MultiFutureObject* self, PyObject* fn){
    fut_state state = (fut_state)atomic_load_uint8(self->_flag);
    if (state != STATE_RUNNING){
        if (PyObject_CallOneArg(fn, (PyObject*)self) == NULL){
            return __mf_handle_exception(self);
        }
        return 0;
    }

    if (PyCallable_Check(fn)){
        PyErr_SetString(PyExc_TypeError, "callback must be a callable type");
        return -1;
    }
    /* setup callbacks if brand new */
    if (self->_callbacks != NULL){
        self->_callbacks = PyList_New(0);
    }
    return (PyList_Append(self->_callbacks, fn) < 0);
}

/* Chains in a brand new asyncio.Future object and gets the thread's eventloop */
static inline PyObject*
mf_create_future(MultiFutureObject* self){
    PyObject* loop = PyObject_CallNoArgs(self->state->get_event_loop);
    if (loop == NULL){
        /* No Future to create because we aren't using an event loop on this thread */
        return NULL;
    }

    PyObject* fut = PyObject_CallMethodNoArgs(loop, self->state->create_future);
    if (fut == NULL){
        return NULL;
    }

    PyObject* fn_multifut__fut_done = PyObject_GetAttrString(self, "__fut_done");
    if (fn_multifut__fut_done == NULL){
        return NULL;
    }

    /* Chain to MultiFuture for futher safe-communication between different threads */
    if (PyObject_CallMethodOneArg(fut, self->state->asyncio_add_done_callback, fn_multifut__fut_done) == NULL){
        Py_XDECREF(fn_multifut__fut_done);
        goto fail;
    }

    /* See if we have any future objects ready or not */
    if (self->_futures == NULL){
        self->_futures = PyList_New(1);
        if (self->_futures == NULL){
            goto fail;
        }
        PyList_SET_ITEM(self->_futures, 0 , Py_NewRef(fut));
    }

    else if (PyList_Append(self->_futures, Py_NewRef(fut)) < 0){
        goto fail;
    }
    return fut;
fail:
    Py_XDECREF(fut);
    return NULL;
}


static int mf_init(MultiFutureObject* self, PyObject* exc_handler){
    if (exc_handler == NULL){
        return 0;
    }
    
    if (!PyCallable_Check(exc_handler)){
        PyErr_SetString(PyExc_TypeError, "Exception Handlers should be callable.");
        return -1;
    }
    self->_exc_handler = Py_NewRef(exc_handler);
    return 0;
}

/* Utilized mainly for the C-API */
MultiFutureObject* mf_new(mod_state* state, PyObject* exc_handler){
    MultiFutureObject* self = (MultiFutureObject*)state->MultiFut_Type->tp_alloc(state->MultiFut_Type, 0);
    if (self == NULL){
        return NULL;
    }
    if (mf_init(self, exc_handler) < 0){
        Py_CLEAR(self);
        return NULL;
    }
    PyObject_GC_Track(self);
    return self;
}

static inline int mf_done(MultiFutureObject* self){
    return atomic_load_uint8(&self->_flag) != STATE_RUNNING;
}

static inline int mf_cancelled(MultiFutureObject* self){
    return atomic_load_uint8(&self->_flag) == STATE_CANCELLED;
}

/* TODO: Should we implement _Py_yeild on builds where python GIL is disabled? */

static int mf_wait(MultiFutureObject* self, _PyTime_t timeout){
    if (mf_done(self)){
        return 1;
    }
    _PyTime_t now = _PyTime_GetMonotonicClock();
    _PyTime_t endgame = 0;
    if (timeout > 0) {
        /* TODO PyTime_add implementation? */
        endgame = now + timeout;
    }

    /* Py_BEGIN_ALLOW_THREADS but as an expanded macro due to circumstances */
    PyThreadState *_save; 
    _save = PyEval_SaveThread();

    while (1){
        fut_state state = atomic_load_uint8(&self->_flag);
        if (state != STATE_RUNNING){
            /* Restore from here so I don't have to think of something more hacky... */
            PyEval_RestoreThread(_save);
            return 1;
        }
        if (timeout == 0){
            /* FAILURE */
            PyEval_RestoreThread(_save);
            return 0;
        }

        if (timeout > 0){
            /* TODO: find backport of PyTime_GetMonotonicClock(...) */
            timeout = endgame - _PyTime_GetMonotonicClock();
            if (timeout <= 0){
                timeout = 0;
            }
        }
        if (PyErr_CheckSignals() < 0){
            /* We need to exit because the user wants to exit */
            PyEval_RestoreThread(_save);
            return -1;
        }
    }
    PyEval_RestoreThread(_save);
    /* UNREACHABLE */
    return 0;
}

static int mf_wait_forever(MultiFutureObject* self){
    if (mf_done(self)){
        return 1;
    }
    /* Py_BEGIN_ALLOW_THREADS but as an expanded macro due to circumstances */
    PyThreadState *_save; 
    _save = PyEval_SaveThread();
    while (1){
        if (atomic_load_uint8(&self->_flag) != STATE_RUNNING){
            /* Restore from here so I don't have to think of something more hacky... */
            PyEval_RestoreThread(_save);
            return 1;
        }
        if (PyErr_CheckSignals() < 0){
            /* We need to exit because the user wants to exit */
            PyEval_RestoreThread(_save);
            return -1;
        }
    }
    PyEval_RestoreThread(_save);
    /* UNREACHABLE */
    return 0;
}





/* ========================= Frontend ========================= */

static int multifut_tp_init(MultiFutureObject* self, PyObject* args, PyObject* kwargs){
    mod_state* state = get_mod_state_by_def(self);
    Py_ssize_t arg_size = PyTuple_GET_SIZE(args);
    PyObject_GC_Track(self);

    if (PyDict_GET_SIZE(kwargs) > 0){
        PyErr_SetString(PyExc_ValueError, "MultiFuture objects do not accept keywords arguments");
        return -1;
    }
    
    if (arg_size > 1){
        PyErr_SetString(PyExc_ValueError, "MultiFuture only accepts 1 optional postional argument for execption handling");   
    }
    /* Keep MultiFuture Threadsafe at all times */
    self->state = state;

    if (arg_size){
        return mf_init(self, PyTuple_GET_ITEM(args, 1));
    } else {
        self->_exc_handler = NULL;
    }
    return 1;
fail:
    Py_XDECREF(self);
    return -1;
}

static PyObject* multifut_done(MultiFutureObject* self){
    return PyBool_FromLong(mf_done(self));
}
static PyObject* multifut_cancelled(MultiFutureObject* self){
    return PyBool_FromLong(mf_cancelled(self));
}


static void multifut_tp_dealloc(MultiFutureObject *self){
    PyObject_GC_UnTrack(self);

    Py_TRASHCAN_BEGIN(self, multifut_tp_dealloc)
        PyObject_ClearWeakRefs((PyObject *)self);

    /* Cancel with no exceptions */
    (void)mf_cancel(self, NULL);
    Py_XDECREF(self->_exc_handler);
    Py_XDECREF(self->_callbacks);
    Py_XDECREF(self->_futures);
    Py_XDECREF(self->_result);
    Py_XDECREF(self->_exception);
    Py_TYPE(self)->tp_free((PyObject *)self); 
    Py_TRASHCAN_END  
}


static int multifut_tp_traverse(MultiFutureObject* self, visitproc visit, void *arg){
    Py_VISIT(Py_TYPE(self));
    if (self->_callbacks != NULL){
        Py_VISIT(self->_callbacks);
    }
    if (self->_exc_handler != NULL){
        Py_VISIT(self->_exc_handler);
    }
    if (self->_futures != NULL){
        Py_VISIT(self->_futures);
    }
    if (self->_result != NULL){
        Py_VISIT(self->_result);
    }
    if (self->_exception != NULL){
        Py_VISIT(self->_exception);
    }
    return 0;
}


/* 
    Function to call a future object and trigger cancellation of cleanup 
    It's programmed in a specific way so that MultiFuture can do the following

    1. if a result is already set it's safer to let the parent (MultiFuture)
        Just go ahead and reject it from being called later
    
    2. cancellation on the other hand should trigger parent (MultiFuture)
        to perform cleanup instead including all chidren (list[asyncio.Future[...]]). 
        Might change this functionallity in the future but for now it's a 
        safe idea.

*/

static PyObject* multifut__fut_done(MultiFutureObject* self, PyObject* fut){
    /* Use-After-Free is not allowed! I won't tollerate it here */
    if (self == NULL || fut == NULL) return NULL;
    
    if (mf_done(self)){
        /* Assume it's been handled already or that were in cleanup phase */
        Py_RETURN_NONE;
    };

    PyObject* cancelled = PyObject_CallMethodNoArgs(fut, self->state->cancelled);
    if (cancelled == NULL){
        return NULL;
    }
    if (Py_IsTrue(cancelled)){
        /* trigger parent to perform cancellation of all futures and itself */
        if (mf_cancel(self, NULL) < 0){
            return NULL;
        }
    }
    Py_RETURN_NONE;
}


static PyObject* multifut_cancel(MultiFutureObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames){
    if (nargs > 2){
        PyErr_SetString(PyExc_ValueError, "MultiFuture.cancel() only accepts 1 positional argument \"msg\"");
        return NULL;
    }
    if (mf_cancel(self, nargs ? (args[0]) : NULL) < 0){
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject* multifut_set_result(MultiFutureObject* self, PyObject* result){
    if (mf_set_result(self, result) < 0){
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject* multifut_set_exception(MultiFutureObject* self, PyObject* result){
    if (mf_set_result(self, result) < 0){
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject* multifut_exception(MultiFutureObject* self){
    PyObject* exc = NULL;
    switch (mf_exception(self, &exc)){
        case 1: {
            return exc;
        }
        case 0: {
            Py_RETURN_NONE;
        }
        default: {
            goto fail;
        }
    }
    fail:
        return NULL;
}


/* Synchronous function for a library named cyares if we are instead waiting on results */
static PyObject* multifut_wait(MultiFutureObject* self, PyObject* args){
    PyObject *timeout_obj = NULL;
    if (!PyArg_ParseTuple(args, "|O?:wait", &timeout_obj)) {
        return NULL;
    }

    if (timeout_obj != NULL){
        _PyTime_t time;
        if (_PyTime_FromSecondsObject(&time, timeout_obj, _PyTime_ROUND_TIMEOUT) < 0){
            return NULL;
        }
        if (mf_wait(self, time) < 0){
            return NULL;
        }
    }
    else {
        if (mf_wait_forever(self) < 0){
            return NULL;
        }
    }
    PyObject* result = NULL;
    if (mf_result(self, &result) < 0){
        return NULL;
    }
    return result;
}

/* Faster version of wait if it's believed that the result was already obtained */
static PyObject* multifut_result(MultiFutureObject* self){
    PyObject* result = NULL;
    if (mf_result(self, &result) < 0){
        return NULL;
    }
    return result;
}

static PyObject* multifut_add_done_callback(MultiFutureObject* self, PyObject* fn){
    if (mf_add_done_callback(self, fn) < 0){
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject* multifut_create_future(MultiFutureObject* self){
    return mf_create_future(self);
}

