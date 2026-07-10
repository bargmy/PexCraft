# PexCraft

PexCraft is an experimental custom client written in C. It is trying to work like **Minecraft JE r1.2.5**, with a focus on stability, predictable behavior, and cross-platform support.

> [!IMPORTANT]
> **PexCraft is not affiliated with, supported by, or approved by Mojang AB, Minecraft, or Microsoft in any way.**  
> **PexCraft is not trying to replicate modern Minecraft.**  
> **The target is Minecraft Java Edition r1.2.5-style behavior, while keeping the project stable, portable, and usable on many platforms.**

## Current platform state

- PSP State = barely playable.
- Windows State = playable.
- Linux SDL2 State = playable.
- Android TV State = build target added; uses SDL2/OpenGL ES and TV remote/gamepad input.

## Renderer states

- Direct3D 9 = unusable.
- Direct3D 11 = working.
- OpenGL = working.
- SDL2-OpenGL = working.

## Android TV

See `android_tv/README.md` for the APK build, signing, TV remote mapping, and Google TV channel integration.

## Save and Quit lifecycle fix

This patched tree treats **Save and Quit to Title** as a complete world-session
teardown: it stops world audio and every world-owned worker, drains autosaves, writes
the final local save synchronously, releases world render/state ownership, and starts
title music on return. See `docs/save_and_quit_full_world_teardown.md`.
