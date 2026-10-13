# Android port engineering status

Updated: 2026-10-13

## Verified milestones

- A native Android ARM64 diagnostic APK is compiled in GitHub Actions.
- The workflow validates the APK ZIP structure, checks that `libror_port_probe.so` is ELF AArch64, and checks the exported NativeActivity entry point.
- The diagnostic app uses Android NativeActivity + EGL + OpenGL ES 3.0.
- Touch input tracks independent pointer IDs for steering, throttle, and brake.
- The HUD now draws visible touch pads and a placeholder chassis using GLES 3 shaders/VBOs.
- The CI workflow now has a separate build job for OGRE v1.11.6 targeting Android ARM64 with the GLES2 renderer. Its result must be checked in Actions; adding the job is not proof that OGRE built successfully.

## Important blockers to full Rigs of Rods integration

This repository does not yet contain the upstream Rigs of Rods application source, its content/resource packs, or an Android-adapted application lifecycle. The native diagnostic target is not the full game.

The upstream dependency meta-build currently pins OGRE 1.11.6. On its non-Windows defaults it disables OIS, while the RoR `InputEngine.h` includes OIS headers and the application initializes an OIS input manager. That means compiling OGRE alone is insufficient: RoR's input abstraction and window/render initialization must be ported as well.

The Android port needs a coordinated implementation of:

1. A GLES-compatible OGRE render system and verification of RoR shaders/materials and custom render features.
2. Android lifecycle integration for surface creation, pause/resume, and context loss.
3. Replacement or Android port of the OIS-backed input layer, while retaining existing RoR input event semantics.
4. A JNI/native touch bridge that injects analog steering, throttle, and brake into the active vehicle's actual input/physics path, including neutral/reset handling when a pointer is released or the app loses focus.
5. Resource discovery and storage paths for terrains, vehicles, textures, scripts, audio, and configuration.
6. An ARM64 package containing all required native libraries and legally distributable content.
7. Emulator and physical-device validation: boot, terrain load, vehicle spawn, drive/steer/brake, frame pacing, memory, thermal stability, pause/resume, and long-session crash checks.

## Optimization plan

Do not optimize by guesswork. First collect a baseline from a repeatable scene and vehicle. Use Android Studio Profiler/Perfetto, `adb shell dumpsys gfxinfo`, `dumpsys meminfo`, and logcat. Track CPU/GPU frame time, physics update time, memory high-water mark, thermal throttling, and crash-free session duration.

After the game is actually running, apply changes in this order:

- Release ARM64 build, LTO/section garbage collection only after checking plugin/resource loading.
- Cap frame rate based on device capability and keep physics timestep independent from render FPS.
- Reduce shadow quality, sight range, water/reflection updates, particles, vegetation density, and texture resolution through game settings before invasive engine changes.
- Use asynchronous loading and avoid blocking the main/render thread.
- Profile physics separately from rendering; preserve solver stability and simulation determinism.
- Re-run the same scene/vehicle regression tests after every optimization.

## License and assets

The upstream Rigs of Rods project is GPL-3.0-or-later. Preserve notices and provide corresponding source when distributing a derivative under the license. Do not bundle community vehicles or terrains until their individual redistribution terms are checked.
