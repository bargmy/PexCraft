Wii TLS crash fix
=================

The Wii build disables PEX_THREAD_LOCAL in src/common/common.h. devkitPPC/libogc DOLs
are not using a normal desktop TLS runtime, and the previous build placed thread-local
mesh/render state at tiny TLS offsets. Dolphin then crashed with DSI reads around small
addresses such as 0x0000000B during world generation / mesh preparation.

The Wii port has async terrain and async mesh workers disabled, so the affected state is
not required to be thread-local. Keeping it as plain static storage is safer for now.
