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
#include <QStringList>
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

    // UI helper methods
    void addSeparator(QHBoxLayout* layout);
    void addSliderWithLabel(QHBoxLayout* layout,
                            const QString& labelText,
                            int minVal,
                            int maxVal,
                            int defaultVal);
    void addSpinBox(QHBoxLayout* layout,
                    const QString& labelText,
                    int minVal,
                    int maxVal,
                    int defaultVal,
                    const QString& suffix);
    void addComboBox(QHBoxLayout* layout,
                     const QString& labelText,
                     const QStringList& items,
                     int defaultIndex = 0);

    QHBoxLayout* mainLayout_ = nullptr;
    QLabel* toolNameLabel_ = nullptr;
    QWidget* optionsContainer_ = nullptr;
    EventBus::SubscriptionId toolChangedSub_ = 0;
};

}  // namespace gimp
