# mutexgear
MutexGear Library Preview

THIS PROJECT CONTAINS PRE-RELEASE LIBRARY SNAPSHOTS.

AWAIT THE RELEASE AT https://mutexgear.com

Currently, the library exports C language implementation (C11 atomics are used but can be eliminated with preprocessor defines) for:
* mutexgear_toggle_t and mutexgear_wheel_t — mutex-only compound syncronization objects for signaling event type synchronization (similar to pthread_cond_t) with a restriction that the signaler must be a single thread pre-defined in advance (mainly, used for operation completion waits);
* mutexgear_rwlock_t and mutexgear_trdl_rwlock_t — mutex-only RWLock implementations without and with try_rdlock support respectively (writer priority, inter-process support, all direction implied priority inheritance).
* mutexgear_completion_queue_t and mutexgear_completion_cancelablequeue_t — multi-threaded server work queue skeletons with ability to wait for work item completion and, additionally, request and conduct work item handling cancellation.

Also, the library provides header-only C++11 wrapper classes for its features:
* mg::mutex_toggle and mg::mutex_wheel — wrappers for mutexgear_toggle_t and mutexgear_wheel_t respectively;
* mg::shared_mutex and mg::trdl::shared_mutex — wrappers for mutexgear_rwlock_t and mutexgear_trdl_rwlock_t respectively;
* mg::completion::waitable_queue and mg::completion::cancelable_queue — wrappers for mutexgear_completion_queue_t and mutexgear_completion_cancelablequeue_t respectively.

All the library features mentioned above depend on signaling of event type synchronization by means of serializing synchronization objects. The signaling of event type synchronization by means of serializing synchronization objects (the muteces or alike) is protected with the U.S. Patent #9983913.
