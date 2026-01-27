/**
 * @file command_palette.h
 * @brief Command palette dialog for quick command access.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace gimp {

class HistoryManager;

/**
 * @brief Represents a command that can be executed from the palette.
 */
struct PaletteCommand {
    std::string id;                ///< Unique command identifier.
    std::string name;              ///< Human-readable command name.
    std::string category;          ///< Command category for grouping.
    std::string shortcut;          ///< Keyboard shortcut string.
    std::function<void()> action;  ///< Callback to execute the command.
};

/**
 * @brief Quick command palette similar to VS Code's Ctrl+Shift+P.
 *
 * Provides fuzzy search over all available commands and allows
 * quick execution without navigating menus.
 */
class CommandPalette : public QDialog {
    Q_OBJECT

  public:
    /*! @brief Constructs the command palette.
     *  @param parent Optional parent widget.
     */
    explicit CommandPalette(QWidget* parent = nullptr);
    ~CommandPalette() override;

    /*! @brief Sets the history manager for command execution context.
     *  @param historyManager The history manager.
     */
    void setHistoryManager(HistoryManager* historyManager);
    /*! @brief Registers a command in the palette.
     *  @param command The command to register.
     */
    void registerCommand(const PaletteCommand& command);
    /*! @brief Shows the command palette dialog. */
    void show();

  protected:
    /*! @brief Handles key press events.
     *  @param event The key event.
     */
    void keyPressEvent(QKeyEvent* event) override;

  private slots:
    void onSearchTextChanged(const QString& text);
    void onItemActivated(QListWidgetItem* item);

  private:
    void setupUi();
    void filterCommands(const std::string& query);
    void executeSelected();

    QVBoxLayout* mainLayout_ = nullptr;
    QLineEdit* searchBox_ = nullptr;
    QListWidget* resultsList_ = nullptr;

    std::vector<PaletteCommand> commands_;
    std::vector<const PaletteCommand*> filteredCommands_;
    HistoryManager* historyManager_ = nullptr;
};

}  // namespace gimp
