# False Sharing Microbenchmark

This is an adjustable false sharing tool, eventually to be used for evaluating incoherent architectures. 
It allocates a shared buffer which is sized as the L1 cache line size, and aligns it on the same boundary.
A variable number of threads (up to the cache line size) write to a given byte _within_ the cache line. 
There are no overlapping reads are writes, thus the coherence traffic (at least for the memory references
contained within the `worker()` function) will be due _purely_ to false sharing. 

You can run it like so:

```
[you@machine] make
[you@machine] ./false -n 64 -a 1000000
```

This will create 64 threads (which will most often correspond to one thread owning a single byte within
the cacheline), and each thread will repeatedly issue a write to that cache line.

Thread counts greater than the cacheline size will (currently) be clipped, thus on a machine with
a standard 64-byte line size, this:

```
[you@machine] ./false -n 200 -a 100000
```

will be the same as:

```
[you@machine] ./false -n 64 -a 100000
```

To make sure things are working as expected, you can use `perf` and `PEBS` to get an idea of how many
inter-core line transfers are occuring. Note that AFAIK this will only work on an Intel machine with
a fairly recent Linux kernel (with PEBS support). Here's an example on a 64-core Skylake (Linux 4.19.9):

```
kyle@jebe ~/false-sharing> perf c2c record --user ./false -n 32 -a 500000000000
# false sharing experiment config:
#                threads : 32
#           L1 line size : 64B
#            memory refs : 500000000000
Allocated aligned shared buf (0x17a0cc0)
[ perf record: Woken up 362 times to write data ]
[ perf record: Captured and wrote 107.782 MB perf.data (441253 samples) ]
kyle@jebe ~/false-sharing> perf c2c repor
```
