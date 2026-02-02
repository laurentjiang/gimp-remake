/**
 * @file tool_options_panel.h
 * @brief Panel widget for displaying and editing tool options.
 * @author Laurent Jiang
 * @date 2026-01-30
 */

#pragma once

#include "core/tool.h"
#include "core/tool_options.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

#include <map>
#include <memory>

namespace gimp {

/**
 * @brief Widget that displays and allows editing of current tool's options.
 *
 * Subscribes to ToolChangedEvent and dynamically populates with tool-specific
 * options. Updates tool state when options are modified.
 */
class ToolOptionsPanel : public QWidget {
    Q_OBJECT

  public:
    explicit ToolOptionsPanel(QWidget* parent = nullptr);
    ~ToolOptionsPanel() override = default;

    /**
     * @brief Set the active tool to display options for.
     * @param tool The tool (may implement ToolOptions interface).
     */
    void setTool(Tool* tool);

  private:
    /**
     * @brief Clear all option widgets.
     */
    void clearOptions();

    /**
     * @brief Populate panel with options from tool.
     */
    void populateOptions();

    /**
     * @brief Create a widget for a single option.
     * @param option The option to create a widget for.
     */
    void createOptionWidget(const ToolOption& option);

    Tool* m_currentTool = nullptr;
    ToolOptions* m_toolOptions = nullptr;

    QVBoxLayout* m_mainLayout = nullptr;

    // Maps option ID to its widget for quick lookup
    std::map<std::string, QWidget*> m_optionWidgets;
    std::map<std::string, QLabel*> m_optionLabels;
};

}  // namespace gimp
