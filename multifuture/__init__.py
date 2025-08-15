"""
MultiFuture
-----------

A Python Future Object that is made to make Blocking and asynchronous
handles for sharing data accross multiple threads
"""

from .multifuture import MultiFuture, InvalidStateError, CancelledError  # type: ignore
