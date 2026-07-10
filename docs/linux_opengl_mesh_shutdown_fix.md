# Linux/OpenGL mesh shutdown freeze — root cause and fix

## Root cause

The freeze was in `src/platform/sdl2_compat.h`, not DirectX.

The SDL/Linux implementation of `WaitForSingleObject` did not implement timeout
semantics correctly:

1. `WaitForSingleObject(thread, 0)` only checked `thread_joined`. That flag is set
   by `pthread_join`, so polling could never detect a pthread that had already
   exited. `async_section_mesh_join_workers()` therefore looped forever with
   completed worker handles still counted as open.
2. `WaitForSingleObject(event, 100)` called `pthread_cond_wait`, which has no
   timeout. The four OpenGL CPU mesh workers share an auto-reset event; one wake
   can be consumed by one worker, leaving the others asleep forever instead of
   rechecking the mesh stop flag after 100 ms.

This is why the progress screen remained at **Cancelling mesh builders** on Linux.
The D3D11 path is not involved in an OpenGL build.

## Fix

- Linux thread polling now uses `pthread_tryjoin_np` for zero-time waits.
- Finite thread waits use `pthread_timedjoin_np`.
- Infinite waits still use `pthread_join`.
- Event waits now use `pthread_cond_timedwait` for finite timeouts.
- Auto-reset behavior is retained, but workers that miss a wake still return
  after their timeout and observe the stop flag.

No worker is force-killed, and no OpenGL operation is moved to a background
thread. Shutdown remains cooperative and safe.
