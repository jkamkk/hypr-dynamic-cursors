#pragma once

#include <hyprland/src/helpers/AnimatedVariable.hpp>
#include <hyprland/src/devices/IPointer.hpp>
#include <hyprutils/animation/AnimatedVariable.hpp>
#include <hyprutils/math/Vector2D.hpp>
#include <chrono>
#include <memory>

using namespace Hyprutils::Math;
using namespace Hyprutils::Animation;
using namespace std::chrono;

class CClick {
  public:
    CClick();
    ~CClick() = default;

    /* calculates the scale factor for the current tick */
    float update();

    /* handles button press/release events */
    void onButton(const IPointer::SButtonEvent& e);
    void takeoverAndPress(float fromScale);

    /* whether any mouse button is currently held down */
    bool isPressed() const { return g_buttons > 0; }

    /* current click scale */
    float currentScale() const { return scale ? scale->value() : 1.0f; }

    /* whether cursor is actively in click release bounce animation */
    bool isReleasing() const;

  private:
    int                          g_buttons = 0;
    steady_clock::time_point     m_lastReleaseTime = steady_clock::now() - seconds(10);
    PHLANIMVAR<float>            scale;
    SP<SAnimationPropertyConfig> m_pConfig;
};
