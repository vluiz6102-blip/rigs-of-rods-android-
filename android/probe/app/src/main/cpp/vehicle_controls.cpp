#include "vehicle_controls.h"

// Keep this translation unit in the native target so the normalization
// contract is compiled by the same Android NDK toolchain as the application.
namespace ror_android {
static_assert(sizeof(VehicleControls) == sizeof(float) * 3,
              "VehicleControls must remain a compact three-float snapshot");
}
