# Android touch-to-vehicle input contract

This module is a small, platform-neutral boundary for the eventual Android-to-RoR input adapter.

- Steering is normalized to [-1, 1].
- Throttle and brake are independently normalized to [0, 1].
- Non-finite values are rejected to neutral; out-of-range values are clamped.
- Call `neutralControls()` whenever a pointer is released, Android pauses the app, focus is lost, or the rendering surface is destroyed.
- The values are **not yet connected to RoR's InputEngine or vehicle physics**. That connection requires the upstream application and input abstraction to be integrated; do not present this contract as playable gameplay.

## Integration target

The upstream application routes OIS listener events through `RoR::AppContext` into `RoR::InputEngine`. The Android port should implement an Android input backend that emits the same existing RoR actions, or adapt normalized analog values at the active actor's established input path. Avoid directly mutating physics nodes from UI code.

Upstream architecture reference: https://developer.rigsofrods.org/d4/d38/_codebase_overview_page.html
