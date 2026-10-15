#pragma once
/**
 * @file        rex/ui/android_bridge.h
 * @brief       Process setup that needs the Java side of an SDL3 Android app.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#include <filesystem>

#include <rex/platform.h>

#if REX_PLATFORM_ANDROID
namespace rex::ui::android {

// Call once on the SDL main thread before anything touches the filesystem:
// points HOME/XDG_DATA_HOME at the app's storage and installs the
// ContentResolver bridge behind rex::filesystem::OpenAndroidContentFileDescriptor.
void InitializeBridge();

// Per-app storage that needs no permission and survives updates:
// /storage/emulated/0/Android/data/<package>/files when external storage is
// mounted, the internal files dir otherwise.
std::filesystem::path GetAppStoragePath();

}  // namespace rex::ui::android
#endif  // REX_PLATFORM_ANDROID
