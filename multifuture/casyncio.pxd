# cython: freethreading_compatible = True
from cpython.object cimport PyObject as PyObject


# Seems my old project had a different use Repo: https://raw.githubusercontent.com/Vizonex/c-asyncio
# Know that using this version rather than the one in the repo is discouraged

cdef extern from "casyncio.h":
    """
/* ========================= IMPORTANT NOTICE ========================= */
/*     Module This was Externed from casyncio library by Vizonex    */
/*     It is used under the intent of optimizing internal calls that */
/*      are common bottleknecks                                      */
/*      Repo: https://raw.githubusercontent.com/Vizonex/c-asyncio   */
    """
    # Returns -1 on fail. required you import this first otherwise
    # expect segfaults...
    int import_casyncio "Import_CAsyncio"() except -1

    # Structures are too dynamic to define in cython so I wrote a C-API for gaining the important parts.
    struct FutureObj:
        pass

    ctypedef class _asyncio.Future [object FutureObj, check_size ignore]:
       pass 

    
    # Cython already does some strict checks on its own so we didn't
    # need any internal checks. This way we save a bit of time. 
    # And time is also saved during execution

    bint FutureObj_IsAlive(Future fut)
    int FutureObj_EnsureAlive(Future fut) except -1
    

    # NOTE: You might be wondering why I didn't include these functions
    # FutureObj_ScheduleCallbacks(FutureObj* fut)
    # FutureObj_SetCancelledError(Future fut)

    # They are internal, not really made to be used by average Cython / CPython Developers
    # however these can be invoked on their own in other ways.

    int FutureObj_SetResult(Future fut, object res) except -1
    # sets the result, returns 0 if sucessful otherwise -1 if an error occured.

    int FutureObj_SetException(Future fut, object exc) except -1
    # sets an exception, returns 0 if sucessful otherwise -1 if an error occured.
    
    int FutureObj_GetResult(Future fut, PyObject** result) except -1
    # Attempts to get the result back from the future, returns -1 if the state is not finished

    int FutureObj_AddDoneCallback(Future fut, object func, PyObject* ctx) except -1
    # appends a new callback simillar to Future.add_done_callback, pass an empty dictionary 
    # to ctx if you don't want to use contexts

    int FutureObj_Cancel(Future fut) except -1
    # Returns -1 if exception was raise, 0 if it couldn't cancel, 1 if success

    Future FutureObj_New(object loop)
    # Creates a New Future Object from an existing eventloop returns NULL on 
    # failure although cython will handle this for you...

    Future FutureObj_NewNoArgs()
    # this maybe slower but beneficial when code can't directly get to the current eventloop.

    bint FutureObj_IsDone(Future fut)
    # if a result or exception was set this function returns True, otherwise False

    bint FutureObj_IsCancelled(Future fut)
    # if the Future Object was cancelled the function returns True, otherwise False


    int Loop_CallSoon(object loop, object func, object arg, PyObject* ctx) except -1
    int Loop_CallSoonThreadSafe(object loop, object func, object arg, PyObject* ctx) except -1
    # Schedules a callback soon but threadsafe, pass NULL to ctx if you choose not to pass one.

    object FutureObj_GetLoop(Future fut)
    # Obtains the eventloop if it exists be very careful when using this function
    
