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
    /// @brief Handles resize to reflow tool buttons.
    void resizeEvent(QResizeEvent* event) override;

  private:
    /// @brief Sets up the UI layout.
    void setupUi();
    /// @brief Populates tools from the registry.
    void populateTools();
    /// @brief Reflows buttons based on current column count.
    void reflowButtons();
    /// @brief Handles tool activation.
    void onToolActivated(const std::string& toolId);
    /// @brief Calculates number of columns for given width.
    [[nodiscard]] int calculateColumns(int width) const;

    static constexpr int kButtonSize = 24;      ///< Size of tool buttons in pixels.
    static constexpr int kButtonSpacing = 1;    ///< Spacing between buttons.
    static constexpr int kMinColumns = 5;       ///< Minimum number of columns.
    static constexpr int kMargin = 4;           ///< Panel margin.

    QVBoxLayout* mainLayout_ = nullptr;                              ///< Main vertical layout.
    QGridLayout* toolGrid_ = nullptr;                                ///< Grid for tool buttons.
    QButtonGroup* buttonGroup_ = nullptr;                            ///< Button group for exclusivity.
    std::vector<ToolButton*> orderedButtons_;                        ///< Ordered list of buttons.
    std::unordered_map<std::string, ToolButton*> toolButtons_;       ///< Tool ID to button map.
    std::unordered_map<std::string, std::string> toolToGroupMap_;    ///< Tool to group mapping.
    std::string activeToolId_;                                       ///< Currently active tool ID.
    int currentColumns_ = kMinColumns;                               ///< Current column count.
    EventBus::SubscriptionId toolSwitchSub_ = 0;                     ///< Event subscription ID.
};

}  // namespace gimp
