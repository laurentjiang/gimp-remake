/**
 * @file tool.cpp
 * @brief Implementation of the Tool base class state machine.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#include "core/tool.h"

namespace gimp {

bool Tool::onMousePress(const ToolInputEvent& event)
{
    if (state_ != ToolState::Idle) {
        return false;
    }

    state_ = ToolState::Active;
    beginStroke(event);
    return true;
}

bool Tool::onMouseMove(const ToolInputEvent& event)
{
    if (state_ != ToolState::Active) {
        return false;
    }

    continueStroke(event);
    return true;
}

bool Tool::onMouseRelease(const ToolInputEvent& event)
{
    if (state_ != ToolState::Active) {
        return false;
    }

    state_ = ToolState::Commit;
    endStroke(event);
    state_ = ToolState::Idle;
    return true;
}

void Tool::reset()
{
    if (state_ == ToolState::Active) {
        cancelStroke();
    }
    state_ = ToolState::Idle;
}

bool Tool::onKeyPress(Qt::Key /*key*/, Qt::KeyboardModifiers /*modifiers*/)
{
    // Default: no key handling
    return false;
}

bool Tool::onKeyRelease(Qt::Key /*key*/, Qt::KeyboardModifiers /*modifiers*/)
{
    // Default: no key handling
    return false;
}

}  // namespace gimp
