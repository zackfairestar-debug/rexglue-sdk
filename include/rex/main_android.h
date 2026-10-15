#pragma once
/**
 * @file        rex/main_android.h
 * @brief       Android process-level helpers shared by the POSIX backends.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#include <cstdint>

#include <rex/platform.h>

#if REX_PLATFORM_ANDROID
#include <android/api-level.h>

namespace rex {

// The API level of the device, not the one the APK was built against.
inline int32_t GetAndroidApiLevel() {
  return static_cast<int32_t>(android_get_device_api_level());
}

}  // namespace rex
#endif  // REX_PLATFORM_ANDROID
