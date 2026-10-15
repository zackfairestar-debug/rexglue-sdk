#pragma once
/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2021 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    2026 - Android port for ReXGlue runtime
 */

#include <rex/ui/surface.h>

struct ANativeWindow;

namespace rex {
namespace ui {

// The ANativeWindow is owned by SDL's Android backend (SDLSurface on the Java
// side). It is only valid between SDL_EVENT_WINDOW_SHOWN/RESTORED and the
// matching background event, which is why the presenter reconnects on resume.
class AndroidNativeWindowSurface final : public Surface {
 public:
  explicit AndroidNativeWindowSurface(ANativeWindow* window) : window_(window) {}
  TypeIndex GetType() const override { return kTypeIndex_AndroidNativeWindow; }
  ANativeWindow* window() const { return window_; }

 protected:
  bool GetSizeImpl(uint32_t& width_out, uint32_t& height_out) const override;

 private:
  ANativeWindow* window_;
};

}  // namespace ui
}  // namespace rex
