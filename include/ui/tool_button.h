/**
 * @file tool_button.h
 * @brief Custom tool button widget with sub-tool indicator and right-click menu.
 * @author Laurent Jiang
 * @date 2026-02-02
 */

#pragma once

#include "core/tool_registry.h"

#include <QMenu>
#include <QToolButton>

#include <string>
#include <vector>

namespace gimp {

/**
 * @brief Custom tool button with GIMP-style appearance.
 *
 * Features:
 * - Flat appearance with no border unless hovered or selected
 * - Small triangle indicator when tool has sub-tools
 * - Right-click menu to switch between sub-tools
 * - Dark gray background on hover/selection
 */
class ToolButton : public QToolButton {
    Q_OBJECT

  public:
    /*! @brief Constructs a tool button.
     *  @param descriptor The primary tool descriptor.
     *  @param parent Optional parent widget.
     */
    explicit ToolButton(const ToolDescriptor& descriptor, QWidget* parent = nullptr);

    /*! @brief Sets the sub-tools for this button's right-click menu.
     *  @param subTools Vector of tool descriptors for the sub-tools.
     */
    void setSubTools(const std::vector<ToolDescriptor>& subTools);

    /*! @brief Returns the current tool ID.
     *  @return The active tool ID for this button.
     */
    [[nodiscard]] std::string currentToolId() const { return currentToolId_; }

    /*! @brief Sets the current tool (updates icon if changed).
     *  @param toolId The tool ID to set as current.
     */
    void setCurrentTool(const std::string& toolId);

    /*! @brief Returns whether this button has sub-tools.
     *  @return True if the button has a sub-tool menu.
     */
    [[nodiscard]] bool hasSubTools() const { return !subTools_.empty(); }

  signals:
    /*! @brief Emitted when a tool is selected (primary or from sub-menu).
     *  @param toolId The selected tool ID.
     */
    void toolActivated(const std::string& toolId);

  protected:
    /// @brief Custom paint handler for button appearance.
    void paintEvent(QPaintEvent* event) override;
    /// @brief Handles mouse entering the widget.
    void enterEvent(QEnterEvent* event) override;
    /// @brief Handles mouse leaving the widget.
    void leaveEvent(QEvent* event) override;
    /// @brief Handles mouse button press.
    void mousePressEvent(QMouseEvent* event) override;
    /// @brief Handles right-click context menu.
    void contextMenuEvent(QContextMenuEvent* event) override;

  private:
    /// @brief Updates the button appearance based on current state.
    void updateAppearance();
    /// @brief Shows the sub-tool popup menu at the given position.
    void showSubToolMenu(const QPoint& pos);

    ToolDescriptor primaryTool_;            ///< The primary tool descriptor.
    std::vector<ToolDescriptor> subTools_;  ///< Sub-tools for right-click menu.
    std::string currentToolId_;             ///< Currently active tool ID.
    QMenu* subToolMenu_ = nullptr;          ///< Popup menu for sub-tools.
    bool hovered_ = false;                  ///< Whether the button is hovered.
};

}  // namespace gimp
