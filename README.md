# MultiFuture

A Faster implementation of concurrent.futures's Future object implemented using Cython with the added ability 
to communicate between multiple different asyncio.Future objects. Name comes from and is inspired by [MultiDict](https://github.com/aio-libs/multidict)

This Future Object is meant to share bounds between asyncio future objects so that it is faster than `asyncio.wrap_future`


This also contains a recast hack for optimizing asyncio.Future objects using C via recasting objects. 
Which so far is experimental but the results it has given have been quite promising.

It can also be used as a Cython Module and not just a Python Module. 


## Benchmarks
Benchmarked on a Windows 10 Lenovo ideapad 1i w/ Intel Pentium using Python 3.10.18


| Task    | MultiFuture | concurrent.futures.Future |
|---------|-------------|---------------------------|
| 1000000 set/get results | 0.593 sec | 7.896 sec | 

