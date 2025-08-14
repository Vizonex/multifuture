# MultiFuture

A Faster implementation of concurrent.futures's Future object implemented using C with the added ability 
to communicate between multiple different asyncio.Future objects. Name comes from and is inspired by `MultiDict`

This Future Object is meant to share bounds between asyncio future objects so that it is faster than `asyncio.wrap_future`


