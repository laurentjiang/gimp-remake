/**
 * @file shortcut_manager.h
 * @brief Global keyboard shortcut manager for tool switching and actions.
 * @author Laurent Jiang
 * @date 2026-01-30
 */

#pragma once

#include <QKeySequence>
#include <QObject>
#include <QShortcut>

#include <memory>
#include <unordered_map>
#include <vector>

namespace gimp {

/**
 * @brief Manages keyboard shortcuts for tool switching and common actions.
 *
 * The ShortcutManager creates QShortcut objects for:
 * - Tool switching (based on ToolRegistry shortcuts)
 * - Brush size adjustment ([ and ])
 * - Color operations (X for swap, D for default colors)
 *
 * All shortcuts are context-aware and only active when the main window
 * has focus (Qt::WindowShortcut context).
 */
class ShortcutManager : public QObject {
    Q_OBJECT

  public:
    /*! @brief Constructs the shortcut manager.
     *  @param parent The parent widget (typically MainWindow).
     */
    explicit ShortcutManager(QWidget* parent);
    ~ShortcutManager() override;

    ShortcutManager(const ShortcutManager&) = delete;
    ShortcutManager& operator=(const ShortcutManager&) = delete;
    ShortcutManager(ShortcutManager&&) = delete;
    ShortcutManager& operator=(ShortcutManager&&) = delete;

    /*! @brief Registers all shortcuts based on ToolRegistry definitions.
     *
     * Call this after the main window is fully constructed.
     */
    void registerToolShortcuts();

    /*! @brief Registers special action shortcuts (brush size, colors).
     *
     * Registers:
     * - [ : Decrease brush size
     * - ] : Increase brush size
     * - X : Swap foreground/background colors
     * - D : Reset to default colors (black/white)
     */
    void registerActionShortcuts();

    /*! @brief Rebinds an existing shortcut to a new key.
     *  @param actionId The action identifier (e.g., "tool:pencil" or "action:swap_colors").
     *  @param newKey The new key sequence to bind.
     *  @return True if rebinding succeeded, false if actionId not found.
     */
    bool rebindShortcut(const std::string& actionId, const QKeySequence& newKey);

    /*! @brief Returns the current key binding for an action.
     *  @param actionId The action identifier.
     *  @return The bound key sequence, or empty if not found.
     */
    [[nodiscard]] QKeySequence getBinding(const std::string& actionId) const;

  signals:
    /*! @brief Emitted when a tool switch shortcut is activated.
     *  @param toolId The ID of the tool to switch to.
     */
    void toolSwitchRequested(const QString& toolId);

    /*! @brief Emitted when brush size decrease is requested. */
    void brushSizeDecreaseRequested();

    /*! @brief Emitted when brush size increase is requested. */
    void brushSizeIncreaseRequested();

    /*! @brief Emitted when color swap is requested. */
    void swapColorsRequested();

    /*! @brief Emitted when default colors are requested. */
    void resetColorsRequested();

  private:
    void registerShortcut(const std::string& actionId,
                          const QKeySequence& key,
                          const std::function<void()>& callback);

    QWidget* parentWidget_ = nullptr;
    std::unordered_map<std::string, std::unique_ptr<QShortcut>> shortcuts_;
    std::unordered_map<std::string, std::function<void()>> callbacks_;
};

}  // namespace gimp
