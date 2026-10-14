#pragma once

#include <hyprland/src/helpers/AnimatedVariable.hpp>
#include <hyprland/src/devices/IPointer.hpp>
#include <hyprutils/animation/AnimatedVariable.hpp>
#include <hyprutils/math/Vector2D.hpp>
#include <memory>

using namespace Hyprutils::Math;
using namespace Hyprutils::Animation;

class CClick {
  public:
    CClick();
    ~CClick() = default;

    /* calculates the scale factor for the current tick */
    float update();

    /* handles button press/release events */
    void onButton(const IPointer::SButtonEvent& e);

    /* whether any mouse button is currently held down */
    bool isPressed() const { return g_buttons > 0; }

  private:
    int                          g_buttons = 0;
    PHLANIMVAR<float>            scale;
    SP<SAnimationPropertyConfig> m_pConfig;
};
