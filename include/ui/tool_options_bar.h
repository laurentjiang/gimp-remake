/**
 * @file tool_options_bar.h
 * @brief Tool options bar widget displayed at the top of the canvas.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#pragma once

#include "core/event_bus.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>

#include <string>

namespace gimp {

/**
 * @brief Horizontal bar displaying options for the currently active tool.
 *
 * Subscribes to ToolChangedEvent to update its contents when the active
 * tool changes. Shows a property grid for tool-specific settings.
 */
class ToolOptionsBar : public QWidget {
    Q_OBJECT

  public:
    explicit ToolOptionsBar(QWidget* parent = nullptr);
    ~ToolOptionsBar() override;

  private:
    void setupUi();
    void updateForTool(const std::string& toolId);
    void clearOptions();

    QHBoxLayout* mainLayout_ = nullptr;
    QLabel* toolNameLabel_ = nullptr;
    QWidget* optionsContainer_ = nullptr;
    EventBus::SubscriptionId toolChangedSub_ = 0;
};

}  // namespace gimp
