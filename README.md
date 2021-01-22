# mutexgear
The MutexGear Library Preview

THIS PROJECT CONTAINS PRE-RELEASE LIBRARY SNAPSHOTS.

AWAIT THE RELEASE AT https://mutexgear.com

Currently, the library exports C language implementation 
(C11 atomics are used but can be eliminated with preprocessor defines) for:
* mutexgear_toggle_t and mutexgear_wheel_t — mutex-only compound syncronization 
objects for signaling event type synchronization (similar to pthread_cond_t) with 
a restriction that the signaler must be a single thread pre-defined in advance 
(mainly, used for operation completion waits);
* mutexgear_rwlock_t and mutexgear_trdl_rwlock_t — mutex-only RWLock implementations 
without and with try_rdlock support respectively (writer priority, inter-process support, 
all direction implied priority inheritance).
* mutexgear_completion_queue_t and mutexgear_completion_cancelablequeue_t — multi-threaded 
server work queue skeletons with ability to wait for work item completion and, additionally, 
request and conduct work item handling cancellation.

Also, the library provides header-only C++11 wrapper classes for its features:
* mg::mutex_toggle and mg::mutex_wheel — wrappers for mutexgear_toggle_t and mutexgear_wheel_t respectively;
* mg::shared_mutex and mg::trdl::shared_mutex — wrappers for mutexgear_rwlock_t and mutexgear_trdl_rwlock_t respectively;
* mg::completion::waitable_queue and mg::completion::cancelable_queue — wrappers for 
mutexgear_completion_queue_t and mutexgear_completion_cancelablequeue_t respectively.

All the library features mentioned above depend on signaling of event type synchronization 
by means of serializing synchronization objects. 
The signaling of event type synchronization by means of serializing synchronization objects (the muteces or alike) 
is protected with the U.S. Patent #9983913.

---

Here are "lock + unlock" quick test stats for the system vs. MutexGear rwlock at Ubuntu 4.4.0-186-generic SMP x86_64.
For better code coverage threads first attempt respective try-lock variant and then, if it fails, do the normal lock.
The increased times in mixed tests are due to the writer priority (fewer write threads force larger number of
readers to enter kernel for blocking to let the writers ahead).

	Testing subsystem "RWLock (with try-write, 1000000 cycles/thr)"
	----------------------------------------------
	Testing                     8 Writers, C: (Sys/MG= 6.090/ 2.597): success
	Testing                    16 Readers, C: (Sys/MG= 4.204/ 4.833): success
	Testing  25% writes @4cnl, 64 threads, C: (Sys/MG=38.422/77.134): success
	Testing      25% writes, 16 threads, C++: (Sys/MG= 9.534/19.005): success
	----------------------------------------------
	Feature tests failed:             0 out of   4
	  
	Testing subsystem "TRDL-RWLock (with try-write, with try-read, 1000000 cycles/thr)"
	----------------------------------------------
	Testing                     8 Writers, C: (Sys/MG= 6.089/ 2.987): success
	Testing                    16 Readers, C: (Sys/MG= 4.169/ 9.302): success
	Testing  25% writes @4cnl, 64 threads, C: (Sys/MG=38.274/76.661): success
	Testing      25% writes, 16 threads, C++: (Sys/MG= 9.485/18.756): success
	----------------------------------------------
	Feature tests failed:             0 out of   4

Extra test results with only the normal waiting lock operations (without the try-lock attempts) can be viewed at Google Docs 
[here](https://docs.google.com/spreadsheets/d/e/2PACX-1vQU6r41rZXCd9aejzwyXTgAIKvhrPGK5ELjgaPjOPWcShzNUgAOKWmCZEx2AseO-qfiUekK-FWlqb0T/pubhtml).
