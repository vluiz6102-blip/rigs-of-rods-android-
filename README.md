# Rigs of Rods Android Port

Experimental Android port workspace for the upstream Rigs of Rods simulator.

## Current state

The repository currently builds a **native diagnostic app, not a playable Rigs of Rods game**.

- NativeActivity + EGL + OpenGL ES probe and multi-touch control visualization.
- A platform-neutral normalized analog-control contract and host-side tests.
- Lifecycle handling clears touch controls on pause, focus loss, stop, and surface destruction to avoid stuck input.
- GitHub Actions includes an experimental OGRE 1.11.6 Android ARM64/GLES2 build job. OGRE output alone is not the complete game.
- The upstream RoR application, production renderer integration, real vehicle/terrain resources, OIS replacement, and connection to the vehicle physics are not yet integrated.

## Build and test status

See [GitHub Actions](https://github.com/vluiz6102-blip/rigs-of-rods-android-/actions). Artifacts are named to make clear that the diagnostic APK and OGRE libraries are **not the game**. Do not use them as evidence that driving works.

## Target device profile

Initial target: Android ARM64 devices with **8 GB RAM or more**. This is a design target, not a verified minimum requirement. Actual memory use and compatibility must be measured after real terrains and vehicles load. GPU compatibility must be validated per driver/vendor; no universal GPU support claim is made.

## Licensing and content

Rigs of Rods is GPL-3.0-or-later. A distributed derivative must preserve notices and meet the license's corresponding-source obligations. Review each community vehicle, terrain, texture, sound, and other asset's redistribution terms separately before bundling it.

## Required milestones before calling it playable

1. Integrate the upstream RoR application startup and Android surface lifecycle.
2. Build and package the engine plus all required runtime dependencies.
3. Replace/adapt the OIS backend while preserving RoR input semantics, including touch and compatible physical controllers.
4. Load a real terrain and a real vehicle through RoR's resource system.
5. Prove steering, throttle, braking, and collisions act on the upstream vehicle simulation.
6. Test pause/resume, surface recreation, sustained driving, crash logs, memory, GPU frame time, and thermal behavior on physical devices with 8 GB RAM.
7. Publish a source-complete, license-compliant build and a reproducible test report.

Upstream source: https://github.com/RigsOfRods/rigs-of-rods
