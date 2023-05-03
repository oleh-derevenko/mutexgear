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
* `mutexgear_completion_queue_t` and `mutexgear_completion_cancelablequeue_t` — multi-threaded 
server work queue skeletons with ability to wait for work item completion and, additionally, 
request and conduct work item handling cancellation.

Also, the library provides header-only C++11 wrapper classes for its features:
* `mg::mutex_toggle` and `mg::mutex_wheel` — wrappers for `mutexgear_toggle_t` and `mutexgear_wheel_t` respectively;
* `mg::shared_mutex` and `mg::trdl::shared_mutex` — wrappers for `mutexgear_rwlock_t` and `mutexgear_trdl_rwlock_t` respectively;
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
                                                  Sys /  MG  /MG-1WP/MG-2WP/MG-4WP/MG-!WP
    Testing                      8 Writers, C: ( 6.052/ 2.562/ 0.000/ 0.000/ 0.000/ 0.000): success
    Testing                     16 Readers, C: ( 4.168/ 4.913/ 0.000/ 0.000/ 0.000/ 0.000): success
    Testing       25% writes, 16 threads, C++: ( 9.584/18.059/ 8.199/ 6.820/ 6.160/ 6.039): success
    Testing 25% writes @4cnl, 32 threads, C++: (19.381/36.321/16.477/13.691/12.429/12.145): success
    ----------------------------------------------
    Feature tests failed:             0 out of   4
    
    Testing subsystem "TRDL-RWLock (with try-write, with try-read, 1000000 cycles/thr)"
    ----------------------------------------------
                                                  Sys /  MG  /MG-1WP/MG-2WP/MG-4WP/MG-!WP
    Testing                      8 Writers, C: ( 6.051/ 2.978/ 0.000/ 0.000/ 0.000/ 0.000): success
    Testing                     16 Readers, C: ( 4.156/ 9.440/ 0.000/ 0.000/ 0.000/ 0.000): success
    Testing       25% writes, 16 threads, C++: ( 9.573/17.827/ 8.962/ 7.089/ 6.591/ 6.390): success
    Testing 25% writes @4cnl, 32 threads, C++: (19.302/35.855/17.093/14.199/13.153/12.878): success
    ----------------------------------------------
    Feature tests failed:             0 out of   4

In the output, first column is the system implementation, then there are several variants of MutexGear object locking:
the default one (with immediately claimed writer priority); writer priority delayed till writer thread witnesses 1, 2 or 4
reader lock releases; and finally, no writer priority variant (this is behaviorally equivalent to the system implementation).
The writer priority customized test variants are skipped for single operation type runs as the feature has no effect in these cases.
The increased times in mixed operation tests are due to fewer write threads forcing larger number of readers to enter kernel for blocking 
to let the writers ahead (the very same writer priority). The mixed test are somewhat artificial yet though as write lock
requests are evenly distributed at every Nth operation in each thread.

At higher feature test levels the exact lock-unlock sequence is dumped to text file for every test. The files can then be used
for quality evaluation and analysis or converted to bitmaps with the included "bmpgen" application (Windows only) for examination at a glance.

Full rwlock test results with only the normal waiting lock operations (without the try-lock attempts) for the Ubuntu and QNX 7.0 can be viewed 
at Google Docs [here](https://docs.google.com/spreadsheets/d/e/2PACX-1vRC2eo9GgW6cVeUhM4mzHG5s3DrkiJS5H8meJ1DAoIzuTt6TYwqED2WI5lU332hp1hb37q-lpxUsB3y/pubhtml).
