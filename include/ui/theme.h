/**
 * @file theme.h
 * @brief Centralized UI theme constants and helpers.
 * @author Laurent Jiang
 * @date 2026-02-02
 */

#pragma once

#include <QColor>
#include <QString>

namespace gimp {

/**
 * @brief Centralized theme constants for consistent UI styling.
 *
 * All color values used across the application should be defined here
 * to ensure consistency and make global theme changes trivial.
 */
namespace Theme {

// Panel backgrounds
inline constexpr int kPanelBackground = 0x404040;   ///< Light gray panel/workspace background
inline constexpr int kSliderBackground = 0x2b2b2b;  ///< Dark background for sliders
inline constexpr int kSliderFill = 0xa0a0a0;        ///< Light gray slider fill color

// Text colors
inline constexpr int kTextPrimary = 0xffffff;    ///< White primary text
inline constexpr int kTextSecondary = 0xcccccc;  ///< Light gray secondary text

// Border colors
inline constexpr int kBorderLight = 0x666666;  ///< Light border for group boxes
inline constexpr int kBorderDark = 0x555555;   ///< Dark border

// Control colors
inline constexpr int kCheckboxUnchecked = 0x3c3c3c;  ///< Unchecked checkbox background
inline constexpr int kCheckboxChecked = 0x555555;    ///< Checked checkbox background
inline constexpr int kCheckboxBorder = 0x555555;     ///< Checkbox border

/**
 * @brief Converts a hex color value to QColor.
 * @param hex The hex color value (e.g., 0x404040).
 * @return QColor instance.
 */
inline QColor toQColor(int hex)
{
    return {(hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF};
}

/**
 * @brief Converts a hex color value to CSS hex string.
 * @param hex The hex color value (e.g., 0x404040).
 * @return CSS hex string (e.g., "#404040").
 */
inline QString toHex(int hex)
{
    return QString("#%1").arg(hex, 6, 16, QLatin1Char('0'));
}

/**
 * @brief Returns the standard dock widget stylesheet.
 * @return QString with dock widget styling.
 */
inline QString dockStyleSheet()
{
    return QString("QDockWidget { background-color: %1; }"
                   "QDockWidget::title { background-color: %1; color: %2; padding: 4px; }")
        .arg(toHex(kPanelBackground), toHex(kTextPrimary));
}

/**
 * @brief Returns the standard title bar stylesheet.
 * @return QString with title bar styling.
 */
inline QString titleBarStyleSheet()
{
    return QString("background-color: %1;").arg(toHex(kPanelBackground));
}

/**
 * @brief Returns the standard bold label stylesheet.
 * @return QString with bold label styling.
 */
inline QString boldLabelStyleSheet()
{
    return QString("color: %1; font-weight: bold;").arg(toHex(kTextPrimary));
}

}  // namespace Theme

}  // namespace gimp
