import anyio as anyio
import pytest
import sys
import asyncio
from multifuture import MultiFuture

uvloop = pytest.importorskip("winloop" if sys.platform == "win32" else "uvloop")

PARAMS = [
    pytest.param(
        ("asyncio", {"loop_factory": uvloop.new_event_loop}), id="asyncio[uvloop]"
    ),
    pytest.param(("asyncio", {"use_uvloop": False}), id="asyncio"),
]

if sys.platform == "win32":
    PARAMS.append(
        pytest.param(
            ("asyncio", {"loop_factory": asyncio.SelectorEventLoop}),
            id="asyncio[win32+selector]",
        )
    )


@pytest.fixture(params=PARAMS)
def anyio_backend(request: pytest.FixtureRequest):
    return request.param


@pytest.mark.anyio
async def test_parent_child():
    fut = MultiFuture()
    child = fut.create_future()
    fut.set_result(42069)
    assert (await child) == 42069


@pytest.mark.anyio
async def test_parent_cancel_kills_child_off():
    fut = MultiFuture()
    child = fut.create_future()
    fut.cancel()
    await asyncio.sleep(0)
    assert child.cancelled()


@pytest.mark.anyio
async def test_child_kills_parent_off():
    fut = MultiFuture()
    child = fut.create_future()
    child.cancel()
    await asyncio.sleep(0)
    assert fut.cancelled()


@pytest.mark.anyio
async def test_child_kills_children():
    fut = MultiFuture()
    children = [fut.create_future() for _ in range(3)]
    children[0].cancel()
    # takes a few cycles to cleanup children
    await asyncio.sleep(0.002)

    for child in children:
        assert child.cancelled()
    # child should kill parent as well...
    assert fut.cancelled()


@pytest.mark.anyio
async def test_everyone_gets_same_result():
    fut: MultiFuture[str] = MultiFuture()
    children = [fut.create_future() for _ in range(3)]
    fut.set_result("Vizonex")

    for child in children:
        assert (await child) == "Vizonex"
    assert fut.result() == "Vizonex"


@pytest.mark.anyio
async def test_edjected_child_remains_different():
    fut: MultiFuture[str] = MultiFuture()
    f = fut.create_future()
    f.set_result("0")
    await asyncio.sleep(0)
    fut.set_result("1")
    assert fut.result() == "1"
    assert (await f) == "0"


@pytest.mark.anyio
async def test_edjected_child_from_children_remains_different():
    fut: MultiFuture[str] = MultiFuture()
    children = [fut.create_future() for _ in range(3)]
    # child 3 should be ejected
    children[-1].set_result(0)
    await asyncio.sleep(0)
    fut.set_result("Vizonex")

    for child in children[:-1]:
        assert (await child) == "Vizonex"
    assert fut.result() == "Vizonex"
    assert (await children[-1]) == 0
