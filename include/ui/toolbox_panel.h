/**
 * @file toolbox_panel.h
 * @brief Toolbox panel widget displaying available tools.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#pragma once

#include "core/event_bus.h"

#include <QButtonGroup>
#include <QGridLayout>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWidget>

#include <string>
#include <unordered_map>
#include <vector>

namespace gimp {

class ToolButton;

/**
 * @brief Panel displaying available tools from the ToolRegistry.
 *
 * Displays tools as icon buttons in a flat grid layout. Similar tools
 * are grouped with right-click menus for sub-tool selection.
 */
class ToolboxPanel : public QWidget {
    Q_OBJECT

  public:
    /*! @brief Constructs the toolbox panel.
     *  @param parent Optional parent widget.
     */
    explicit ToolboxPanel(QWidget* parent = nullptr);
    ~ToolboxPanel() override;

    /*! @brief Sets the active tool by ID.
     *  @param toolId The tool identifier to activate.
     */
    void setActiveTool(const std::string& toolId);
    /*! @brief Returns the currently active tool ID.
     *  @return The active tool identifier.
     */
    [[nodiscard]] std::string activeTool() const { return activeToolId_; }

  signals:
    /*! @brief Emitted when a tool is selected.
     *  @param toolId The selected tool identifier.
     */
    void toolSelected(const QString& toolId);

  protected:
    void resizeEvent(QResizeEvent* event) override;

  private:
    void setupUi();
    void populateTools();
    void reflowButtons();
    void onToolActivated(const std::string& toolId);
    [[nodiscard]] int calculateColumns(int width) const;

    static constexpr int kButtonSize = 24;
    static constexpr int kButtonSpacing = 1;
    static constexpr int kMinColumns = 4;
    static constexpr int kMargin = 4;

    QVBoxLayout* mainLayout_ = nullptr;
    QGridLayout* toolGrid_ = nullptr;
    QButtonGroup* buttonGroup_ = nullptr;
    std::vector<ToolButton*> orderedButtons_;
    std::unordered_map<std::string, ToolButton*> toolButtons_;
    std::unordered_map<std::string, std::string> toolToGroupMap_;
    std::string activeToolId_;
    int currentColumns_ = kMinColumns;
    EventBus::SubscriptionId toolSwitchSub_ = 0;
};

}  // namespace gimp
