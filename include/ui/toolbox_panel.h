/**
 * @file toolbox_panel.h
 * @brief Toolbox panel widget displaying available tools.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#pragma once

#include <QButtonGroup>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <string>

namespace gimp {

/**
 * @brief Panel displaying available tools from the ToolRegistry.
 *
 * Displays tools as icon buttons organized by category. When a tool is
 * selected, it emits a signal and publishes a ToolChangedEvent.
 */
class ToolboxPanel : public QWidget {
    Q_OBJECT

  public:
    explicit ToolboxPanel(QWidget* parent = nullptr);
    ~ToolboxPanel() override;

    void setActiveTool(const std::string& toolId);
    [[nodiscard]] std::string activeTool() const { return activeToolId_; }

  signals:
    void toolSelected(const QString& toolId);

  private slots:
    void onToolButtonClicked(int id);

  private:
    void setupUi();
    void populateTools();

    QVBoxLayout* mainLayout_ = nullptr;
    QButtonGroup* buttonGroup_ = nullptr;
    std::string activeToolId_;
};

}  // namespace gimp
