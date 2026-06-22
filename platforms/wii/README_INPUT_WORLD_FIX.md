# Wii input/world-entry fix

This revision fixes two issues seen in Dolphin after the TLS crash fix:

- The shared gamepad layer only switched to controller mode while a Wiimote
  button was held.  Wii now exposes a Wiimote controller every frame that WPAD
  reports the remote is present, and the shared gamepad focus stays in gamepad
  mode while a Wii controller exists.
- Runtime terrain streaming now runs on the main game thread on Wii instead of
  the background service thread.  This avoids racing the renderer/GX mesh state
  and prevents the in-game "Generating terrain chunks..." loop from being driven
  by a libogc pthread while the main thread is rendering.

Wii also now uses the same spawn safety surface path as PSP, so after entering a
new RAM world the player is guaranteed to stand on generated solid terrain.
