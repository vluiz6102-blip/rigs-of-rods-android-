# Rigs of Rods Android Port

Experimental Android port workspace for Rigs of Rods.

## Verified status (2026-10-13)

- The ARM64 native diagnostic APK builds successfully in GitHub Actions.
- The generated APK archive was downloaded and checked locally; it contains `lib/arm64-v8a/libror_port_probe.so`, an ELF64 AArch64 shared library exporting `android_main`.
- The diagnostic app initializes a native Android activity and OpenGL ES 3 surface and receives multi-touch input events.
- **This is not the Rigs of Rods game.** No OGRE renderer, upstream RoR application, vehicle assets, or RoR physics are integrated yet. The touch values are prototype input state only, not connected to actual vehicle controls. Device installation, real-device rendering, driving, FPS, thermal behavior, and stability have not yet been validated.

## Download the diagnostic APK

Open the [latest GitHub Actions run](https://github.com/vluiz6102-blip/rigs-of-rods-android-/actions/workflows/android-port-preflight.yml). In a successful run, download the artifact named `ror-android-native-probe-arm64-not-the-game`.

## Licensing

Rigs of Rods is licensed under GPL-3.0-or-later. Preserve upstream copyright and license notices. Community vehicles and terrains may have separate licenses and must be reviewed individually.

## Next milestones

1. Validate installation and lifecycle on a physical ARM64 Android device.
2. Port and validate OGRE and dependencies for Android.
3. Adapt RoR application startup, resource loading, window and input systems.
4. Connect touch controls to the real vehicle simulation and input bindings.
5. Test driving, FPS, memory, heat, crashes and long-session stability on devices.

Upstream: https://github.com/RigsOfRods/rigs-of-rods
