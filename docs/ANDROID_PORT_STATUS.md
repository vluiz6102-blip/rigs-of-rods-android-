# Android port engineering status

Updated: 2026-10-13

## Implemented in this repository

- Native Android diagnostic activity using NativeActivity, EGL, and OpenGL ES.
- Multi-touch pointer tracking for the prototype steering, throttle, and brake pads.
- Platform-neutral analog value normalization in `vehicle_controls.h` and `vehicle_controls.cpp`.
- Host-side unit tests for normal values, clamping, NaN/infinity handling, and neutral controls.
- Touch controls reset on Android pause, stop, lost focus, and surface termination/recreation to reduce stuck-input risk.
- CI job for experimental OGRE 1.11.6 Android ARM64/GLES2 compilation.

## Explicitly not implemented yet

**This repository does not currently contain a playable Rigs of Rods Android port.** The native activity renders a diagnostic HUD and placeholder chassis. The normalized controls are not connected to `RoR::InputEngine`, an active vehicle, or the physics simulation. No real RoR terrain or vehicle is loaded.

## Integration architecture

The upstream application must be integrated as a whole rather than replaced by the diagnostic loop:

1. Port/adapt the RoR startup path, Android lifecycle, render-window creation, pause/resume, EGL context loss, and shutdown.
2. Integrate OGRE with the GLES renderer and validate RoR's material/shader/render features. A successful OGRE library build is necessary but not sufficient.
3. Adapt the OIS-backed `InputEngine` at the platform boundary. Preserve existing keyboard/joystick event semantics expected by gameplay, GUI, scripts, replay, and editor code; add Android touch and compatible physical controller mappings.
4. Route analog input through the existing RoR input/actor control path. Do not mutate physics nodes from the UI thread. Reset input on pointer release, focus loss, pause, and surface destruction.
5. Integrate resource discovery and writable paths for configuration, terrains, vehicles, textures, scripts, and audio; validate the exact resource formats and dependencies.
6. Package all required ARM64 libraries and legally redistributable resources, preserving GPL-3.0-or-later notices and source obligations. Audit community assets individually.

## Target hardware and optimization

Initial target profile: ARM64 Android devices with **8 GB RAM or more**. This is not a verified minimum yet. Measure actual resident memory, peak allocations, and load behavior after real content is integrated. GPU support must be tested across Adreno, Mali, PowerVR, and other supported GLES implementations; do not claim universal compatibility without device tests.

After real gameplay works, establish repeatable terrain/vehicle baselines. Use Android Studio Profiler/Perfetto, `adb shell dumpsys gfxinfo`, `dumpsys meminfo`, and logcat. Track CPU/GPU frame time, physics-step time, memory high-water mark, thermal throttling, and crash-free session duration. Then tune view distance, shadows, water/reflections, particles, vegetation, texture resolution, loading, and frame pacing. Keep the physics timestep independent of rendering FPS where supported.

## Required release gates

- APK installs and starts on a physical ARM64 device.
- A real terrain loads through RoR's resource system.
- A real RoR vehicle spawns and can be driven, steered, braked, and collided.
- Pause/resume and surface recreation do not leave controls stuck or crash.
- Sustained play sessions are measured on physical 8 GB RAM devices.
- GPU/device compatibility matrix, performance results, and known limitations are published.
- License notices and redistributable asset terms are verified.

Upstream: https://github.com/RigsOfRods/rigs-of-rods
