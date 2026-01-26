/**
 * @file history_panel.h
 * @brief History panel widget displaying undo/redo stack.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#pragma once

#include "core/event_bus.h"

#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <vector>

namespace gimp {

/**
 * @brief Represents a single history entry for display.
 */
struct HistoryEntry {
    std::string actionName;  ///< Name of the action (e.g., "Add Layer", "Fill Color").
    bool isUndone = false;   ///< True if this action has been undone.
};

/**
 * @brief Panel displaying the undo/redo history stack.
 *
 * Shows a list of all actions that can be undone or redone.
 * Clicking an entry jumps to that point in history.
 */
class HistoryPanel : public QWidget {
    Q_OBJECT

  public:
    /*! @brief Constructs the history panel.
     *  @param parent Optional parent widget.
     */
    explicit HistoryPanel(QWidget* parent = nullptr);
    ~HistoryPanel() override;

    /*! @brief Adds a new history entry.
     *  @param actionName The name of the action.
     */
    void addEntry(const std::string& actionName);
    /*! @brief Clears all history entries. */
    void clear();

  signals:
    /*! @brief Emitted when the user wants to jump to a history point.
     *  @param index The target history index.
     */
    void historyJumpRequested(int index);

  private slots:
    void onItemClicked(QListWidgetItem* item);
    void onUndoClicked();
    void onRedoClicked();
    void onClearClicked();

  private:
    void setupUi();
    void refreshList();

    QVBoxLayout* mainLayout_ = nullptr;
    QListWidget* historyList_ = nullptr;
    QPushButton* undoButton_ = nullptr;
    QPushButton* redoButton_ = nullptr;
    QPushButton* clearButton_ = nullptr;

    std::vector<HistoryEntry> entries_;
    int currentIndex_ = -1;
    EventBus::SubscriptionId historyChangedSub_ = 0;
};

}  // namespace gimp
