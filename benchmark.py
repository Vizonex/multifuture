# this Benchmark attempts to prove that there is improved performance even with the added in features.
# Currently as of 8/15/2025 MultiFuture is roughly 12x (12 times) faster.

multifut_setup = """\
from multifuture import MultiFuture
"""

python_setup = """\
from concurrent.futures import Future
"""
import timeit


print("concurrent.future.Future {:.3f} sec".format(timeit.timeit(
    "fut = Future(); fut.set_result(0); fut.result()", python_setup
)))

print("MultiFuture {:.3f} sec".format(
    timeit.timeit("fut = MultiFuture(); fut.set_result(0); fut.result()", multifut_setup)
))


