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
    std::string actionName;
    bool isUndone = false;
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
    explicit HistoryPanel(QWidget* parent = nullptr);
    ~HistoryPanel() override;

    void addEntry(const std::string& actionName);
    void clear();

  signals:
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
