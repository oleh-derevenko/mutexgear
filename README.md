# mutexgear
MutexGear Library Preview

THIS PROJECT CONTAINS PRE-RELEASE LIBRARY SNAPSHOTS FOR EVALUATION PURPOSES ONLY.

AWAIT THE RELEASE AT https://mutexgear.com

Currently, the library exports C language implementation (C11 atomics are used but can be eliminated with preprocessor defines) for:
* mutexgear_wheel_t — a mutex-only compound syncronization object for signaling event type synchronization (similar to pthread_cond_t) with a restriction that the signaler must be a single thread pre-defined in advance (mainly, used for operation completion waits);
* mutexgear_rwlock_t and mutexgear_trdl_rwlock_t — mutex-only RWLock implementations without and with try_rdlock support respectively (writer priority, inter-process support, all direction implied priority inheritance).

The signaling of event type synchronization by means of serializing synchronization objects (the muteces or alike) is protected with the U.S. Patent #9983913.
