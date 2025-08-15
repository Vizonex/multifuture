from threading import Thread
from typing import Callable, TypeVar
from typing_extensions import ParamSpec
import time

from multifuture import MultiFuture, InvalidStateError, CancelledError
import pytest

T = TypeVar("T")
P = ParamSpec("P")


def submit(
    mf: MultiFuture[T], fn: Callable[P, T], *args: P.args, **kwargs: P.kwargs
) -> Thread:
    """Sets up a dummy test for a Multifuture to test different case scenarios"""

    def wrapper(*args: P.args, **kwargs: P.kwargs):
        mf.set_result(fn(*args, **kwargs))

    thread = Thread(target=wrapper, args=args, kwargs=kwargs)
    thread.start()
    return thread


def multiply(x: int, y: int) -> int:
    return x * y


def add(x: int, y: int) -> int:
    return x + y


@pytest.fixture(params=[(multiply, 2), (add, 3)], ids=("multiply", "add"))
def launch_task(
    request: pytest.FixtureRequest,
) -> tuple[Callable[[int, int], int], int]:
    return request.param


def test_multifut_set_object_result() -> None:
    m: MultiFuture[int] = MultiFuture()
    m.set_result(1)
    assert m.result() == 1


def test_submission(launch_task) -> None:
    m: MultiFuture[int] = MultiFuture()
    t = submit(m, launch_task[0], 1, 2)
    assert m.result() == launch_task[1]
    t.join()


def poll():
    time.sleep(0.001)
    return "SUCCESS"


def test_polling():
    m: MultiFuture[str] = MultiFuture()
    submit(m, poll)
    item = m.result(0.1)
    assert item == "SUCCESS"


def test_polling_failure():
    m = MultiFuture()
    with pytest.raises(TimeoutError, match=r"MultiFuture is not finished yet"):
        m.result(0)


def test_cancellation():
    m = MultiFuture()
    m.cancel()
    assert m.cancelled()


def test_cancellation_exception():
    m = MultiFuture()
    m.cancel()
    with pytest.raises(CancelledError):
        m.result()


def test_invalid_state_from_cancellation():
    m = MultiFuture()
    with pytest.raises(InvalidStateError):
        m.cancel()
        m.set_result(0)


def test_invalid_state_from_secondary_result():
    m = MultiFuture()
    with pytest.raises(InvalidStateError):
        m.set_result(0)
        m.set_result(0)
