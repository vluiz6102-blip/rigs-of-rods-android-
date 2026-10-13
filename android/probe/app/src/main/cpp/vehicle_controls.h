#pragma once

#include <algorithm>
#include <cmath>

namespace ror_android {

// Platform-neutral analog control snapshot. Values are normalized to [-1, 1]
// for steering and [0, 1] for pedals so the Android UI can later feed the
// existing RoR input/vehicle path without coupling physics to touch handling.
struct VehicleControls {
    float steering = 0.0f;
    float throttle = 0.0f;
    float brake = 0.0f;
};

inline float finiteClamp(float value, float low, float high) noexcept {
    if (!std::isfinite(value)) return 0.0f;
    return std::max(low, std::min(value, high));
}

inline VehicleControls normalizeControls(float steering, float throttle, float brake) noexcept {
    return {
        finiteClamp(steering, -1.0f, 1.0f),
        finiteClamp(throttle, 0.0f, 1.0f),
        finiteClamp(brake, 0.0f, 1.0f)
    };
}

// Always call on focus loss, pause, surface destruction, or control release.
// This prevents a stuck accelerator after Android interrupts a touch stream.
inline VehicleControls neutralControls() noexcept {
    return {};
}

} // namespace ror_android
