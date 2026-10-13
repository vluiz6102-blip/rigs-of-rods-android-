# Rigs of Rods Android Port

Experimental Android port workspace for Rigs of Rods.

## Current status

This repository is being initialized from the Android port bootstrap. The current prototype is a native Android/OpenGL ES diagnostic app with touch-control experiments; it is **not yet the Rigs of Rods game**. OGRE, the upstream RoR engine, vehicle physics integration, and real-device validation remain pending.

## Licensing

Rigs of Rods is licensed under GPL-3.0-or-later. Preserve upstream copyright and license notices. Community vehicles and terrains may have separate licenses and must be reviewed individually.

## Next milestones

1. Add the bootstrap source, tests, and CI workflow.
2. Build the ARM64 diagnostic APK in GitHub Actions.
3. Port and validate OGRE and dependencies for Android.
4. Adapt the application lifecycle and input system.
5. Connect mobile controls to actual vehicle physics and test on device.

Upstream: https://github.com/RigsOfRods/rigs-of-rods
