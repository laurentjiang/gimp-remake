/**
 * @file toast_constants.h
 * @brief Shared constants for toast notifications
 * @author Laurent Jiang
 * @date 2026-02-05
 */

#pragma once

namespace gimp {
namespace toast {

/// Fixed width of each toast widget (pixels)
constexpr int kToastWidth = 280;

/// Margin between toasts and screen edge (pixels)
constexpr int kToastMargin = 10;

/// Vertical spacing between consecutive toasts (pixels)
constexpr int kToastSpacing = 6;

/// Auto-dismiss timeout for Info severity (milliseconds)
constexpr int kInfoTimeoutMs = 3000;

/// Auto-dismiss timeout for Warning severity (milliseconds)
constexpr int kWarningTimeoutMs = 5000;

/// Auto-dismiss timeout for Error severity (milliseconds) – 0 means persistent
constexpr int kErrorTimeoutMs = 0;

/// Maximum number of toasts visible at once
constexpr int kMaxToasts = 5;

/// Maximum number of toasts allowed per second (rate limiting)
constexpr int kMaxToastsPerSecond = 3;

}  // namespace toast
}  // namespace gimp
