#include "vehicle_controls.h"
#include <cassert>
#include <cmath>
#include <limits>

int main() {
    using namespace ror_android;
    const auto normal = normalizeControls(-0.4f, 0.7f, 0.2f);
    assert(std::abs(normal.steering + 0.4f) < 0.0001f);
    assert(std::abs(normal.throttle - 0.7f) < 0.0001f);
    assert(std::abs(normal.brake - 0.2f) < 0.0001f);

    const auto clamped = normalizeControls(-9.0f, 3.0f, -2.0f);
    assert(clamped.steering == -1.0f);
    assert(clamped.throttle == 1.0f);
    assert(clamped.brake == 0.0f);

    const auto invalid = normalizeControls(std::numeric_limits<float>::quiet_NaN(),
                                           std::numeric_limits<float>::infinity(), 0.5f);
    assert(invalid.steering == 0.0f);
    assert(invalid.throttle == 0.0f);
    assert(invalid.brake == 0.5f);

    const auto neutral = neutralControls();
    assert(neutral.steering == 0.0f && neutral.throttle == 0.0f && neutral.brake == 0.0f);
}
