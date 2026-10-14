#pragma once

#include <hyprland/src/helpers/AnimatedVariable.hpp>
#include <hyprutils/animation/AnimatedVariable.hpp>
#include <hyprutils/math/Vector2D.hpp>
#include <chrono>
#include <string>
#include <functional>
#include <memory>

using namespace Hyprutils::Math;
using namespace Hyprutils::Animation;
using namespace std::chrono;

enum class EShapeTransitionPhase {
    IDLE = 0,
    SHRINKING,
    EXPANDING,
};

class CShapeTransition {
  public:
    CShapeTransition();
    ~CShapeTransition() = default;

    /* Returns the current transition scale (clamped to [0.5, 1.0]) and triggers texture swap when reached bottom */
    float update();

    /* Request a shape change. Returns true if immediately applied, false if deferred during shrink */
    bool onShapeRequest(void* thisptr, const std::string& newShape, const std::function<void(void*, const std::string&)>& applyShape);

    /* Forcibly reset transition (e.g. on client-side surface cursors or disabled) */
    void reset();

    /* Whether a transition is currently in progress */
    bool isTransitioning() const { return m_phase != EShapeTransitionPhase::IDLE; }

    /* Current visual scale */
    float currentScale() const { return scale ? scale->value() : 1.0f; }

    /* Interrupt transition immediately on user click: swaps texture if pending, halts transition, and returns visual scale */
    float interrupt(const std::function<void(void*, const std::string&)>& applyShape);

    /* Get currently active shape */
    const std::string& currentShape() const { return m_currentShape; }

    /* Force set current shape without animation (e.g. init) */
    void setCurrentShape(const std::string& s) { m_currentShape = s; }

  private:
    EShapeTransitionPhase                         m_phase = EShapeTransitionPhase::IDLE;
    std::string                                   m_currentShape = "";
    std::string                                   m_pendingShape = "";
    void*                                         m_pendingThisptr = nullptr;
    std::function<void(void*, const std::string&)> m_applyShapeCallback;

    steady_clock::time_point                      m_phaseStartTime;

    PHLANIMVAR<float>                             scale;
    SP<SAnimationPropertyConfig>                  m_pConfig;

    void registerBeziers();
};
