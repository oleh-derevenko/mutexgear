# mutexgear
The MutexGear Library Preview

THIS PROJECT CONTAINS PRE-RELEASE LIBRARY SNAPSHOTS.

AWAIT THE RELEASE AT https://mutexgear.com

Currently, the library exports C language implementation 
(C11 atomics are used but can be eliminated with preprocessor defines) for:
* `mutexgear_toggle_t` and `mutexgear_wheel_t` — mutex-only compound syncronization 
objects for signaling event type synchronization (similar to `pthread_cond_t`) with 
a restriction that the signaler must be a single thread pre-defined in advance 
(mainly, used for operation completion waits);
* `mutexgear_rwlock_t` and `mutexgear_trdl_rwlock_t` — mutex-only RWLock implementations 
without and with try_rdlock support respectively (customizable writer priority, inter-process support, 
all direction implicit priority inheritance).
* `mutexgear_maintlock_t` — a mutex-only object to lock a shared resource for maintenance (update)
while that resource is accessed by multiple threads in shared lock mode 
(the object can be used to wait for all the threads to stop accessing the resource);
* `mutexgear_completion_queue_t` and `mutexgear_completion_cancelablequeue_t` — multi-threaded 
server work queue skeletons with ability to wait for work item completion and, additionally, 
request and conduct work item handling cancellation.

Also, the library provides header-only C++11 wrapper classes for its features:
* `mg::mutex_toggle` and `mg::mutex_wheel` — wrappers for `mutexgear_toggle_t` and `mutexgear_wheel_t` respectively;
* `mg::shared_mutex` and `mg::trdl::shared_mutex` — wrappers for `mutexgear_rwlock_t` and `mutexgear_trdl_rwlock_t` respectively;
* `mg::main_mutex` — a wrapper for `mutexgear_maintlock_t`;
* `mg::completion::waitable_queue` and `mg::completion::cancelable_queue` — wrappers for 
`mutexgear_completion_queue_t` and `mutexgear_completion_cancelablequeue_t` respectively.

**All the library features mentioned above depend on signaling of event type synchronization 
by means of serializing synchronization objects. 
The signaling of event type synchronization by means of serializing synchronization objects (the muteces or alike) 
is protected with the U.S. Patent #9983913.**

See build/vs2013/vs2013atomics.h and `__MUTEXGEAR_ATOMIC_HEADER` define in build/vs2013/mg.vcxproj for an example 
of how to build the library with C99.

---

Here are "lock + unlock" quick test stats for the system vs. MutexGear rwlock at Ubuntu 4.4.0-210-generic SMP x86_64.
For better code coverage, threads first attempt respective try-lock variant and then, if it fails, do the normal lock.

    Testing subsystem "RWLock (with try-write, 1000000 cycles/thr)"
    ----------------------------------------------
    Total = Consumed clock time for the test
    SBID = sum of lock times bigger than 1 quantum
    MBID = the maximal lock time of those bigger than 1 quantum
                                                         Sys         /          MG         /        MG-1WP       /        MG-2WP       /        MG-4WP       /        MG-!WP
                                                 Total,  SBID , MBID / Total,  SBID , MBID / Total,  SBID , MBID / Total,  SBID , MBID / Total,  SBID , MBID / Total,  SBID , MBID
    Testing                      8 Writers, C: ( 6.566, 38.829, 0.000/ 3.654, 19.104, 0.000/ 0.000,  0.000, 0.000/ 0.000,  0.000, 0.000/ 0.000,  0.000, 0.000/ 0.000,  0.000, 0.000): success
    Testing                     16 Readers, C: ( 5.025,  0.000, 0.000/ 5.522,  2.340, 0.044/ 0.000,  0.000, 0.000/ 0.000,  0.000, 0.000/ 0.000,  0.000, 0.000/ 0.000,  0.000, 0.000): success
    Testing       25% writes, 16 threads, C++: ( 9.620,128.245, 0.001/21.411,329.521, 0.004/10.398,158.216, 0.002/ 7.680,115.635, 0.006/ 6.775,101.128, 0.008/ 6.526, 97.366, 0.009): success
    Testing 25% writes @4cnl, 32 threads, C++: (19.204,561.823, 0.001/42.579,1335.058, 0.012/20.962,652.258, 0.004/15.559,482.082, 0.006/13.685,421.613, 0.011/13.266,408.011, 0.008): success
    ----------------------------------------------
    Feature tests failed:             0 out of   4
    
    Testing subsystem "TRDL-RWLock (with try-write, with try-read, 1000000 cycles/thr)"
    ----------------------------------------------
                                                         Sys         /          MG         /        MG-1WP       /        MG-2WP       /        MG-4WP       /        MG-!WP
                                                 Total,  SBID , MBID / Total,  SBID , MBID / Total,  SBID , MBID / Total,  SBID , MBID / Total,  SBID , MBID / Total,  SBID , MBID
    Testing                      8 Writers, C: ( 6.551, 38.703, 0.000/ 4.005, 21.410, 0.000/ 0.000,  0.000, 0.000/ 0.000,  0.000, 0.000/ 0.000,  0.000, 0.000/ 0.000,  0.000, 0.000): success
    Testing                     16 Readers, C: ( 4.957,  0.000, 0.000/10.322,  0.000, 0.000/ 0.000,  0.000, 0.000/ 0.000,  0.000, 0.000/ 0.000,  0.000, 0.000/ 0.000,  0.000, 0.000): success
    Testing       25% writes, 16 threads, C++: ( 9.747,130.002, 0.001/21.621,330.592, 0.005/10.539,160.202, 0.002/ 7.850,118.124, 0.003/ 7.010,104.940, 0.014/ 6.834,102.035, 0.014): success
    Testing 25% writes @4cnl, 32 threads, C++: (19.289,564.727, 0.002/43.350,1354.363, 0.011/20.838,648.507, 0.003/15.630,484.634, 0.003/14.066,433.938, 0.009/13.796,425.780, 0.008): success
    ----------------------------------------------
    Feature tests failed:             0 out of   4

In the output, first column is the system implementation, then there are several variants of MutexGear object locking:
the default one (with immediately claimed writer priority); writer priority delayed till writer thread witnesses 1, 2 or 4
reader lock releases; and finally, no writer priority variant (this is behaviorally equivalent to the system implementation).
The writer priority customized test variants are skipped for single operation type runs as the feature has no effect in these cases.
In the mixed tests, threads are performing each its own pseudo-random lock sequence where write lock is executed with the specified 
probability and read lock is executed otherwise. The thread sequences match across per-object test runs so that each of the object modifications
is presented with equal operation sequences (being only subject to thread scheduling order and lock success order variations). 
The increased times in mixed operation tests are due to fewer write threads forcing larger number of readers to enter kernel for blocking 
to let the writers ahead (the very same writer priority).

At higher feature test levels the exact lock-unlock sequence is dumped to text file for every test. The files can then be used
for quality evaluation and analysis or converted to bitmaps with the included "bmpgen" application (Windows only) for examination at a glance.

Full rwlock test results with only the normal waiting lock operations (without the try-lock attempts) for the Ubuntu and QNX 7.0 can be viewed 
at Google Docs [here](https://docs.google.com/spreadsheets/d/e/2PACX-1vRC2eo9GgW6cVeUhM4mzHG5s3DrkiJS5H8meJ1DAoIzuTt6TYwqED2WI5lU332hp1hb37q-lpxUsB3y/pubhtml).
