/**
 * @file history_panel.cpp
 * @brief Implementation of HistoryPanel widget.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#include "ui/history_panel.h"

#include "core/events.h"

#include <QHBoxLayout>
#include <QLabel>

namespace gimp {

HistoryPanel::HistoryPanel(QWidget* parent) : QWidget(parent)
{
    setupUi();

    historyChangedSub_ = EventBus::instance().subscribe<HistoryChangedEvent>(
        [this](const HistoryChangedEvent& event) {
            if (!event.lastActionName.empty()) {
                addEntry(event.lastActionName);
            }
        });

    addEntry("Original");
    currentIndex_ = 0;
}

HistoryPanel::~HistoryPanel()
{
    EventBus::instance().unsubscribe(historyChangedSub_);
}

void HistoryPanel::setupUi()
{
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(4, 4, 4, 4);
    mainLayout_->setSpacing(4);

    auto* titleLabel = new QLabel("History", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    mainLayout_->addWidget(titleLabel);

    historyList_ = new QListWidget(this);
    historyList_->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout_->addWidget(historyList_);

    connect(historyList_, &QListWidget::itemClicked, this, &HistoryPanel::onItemClicked);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(2);

    undoButton_ = new QPushButton("Undo", this);
    undoButton_->setToolTip("Undo last action (Ctrl+Z)");
    buttonLayout->addWidget(undoButton_);

    redoButton_ = new QPushButton("Redo", this);
    redoButton_->setToolTip("Redo last undone action (Ctrl+Y)");
    buttonLayout->addWidget(redoButton_);

    clearButton_ = new QPushButton("Clear", this);
    clearButton_->setToolTip("Clear history");
    buttonLayout->addWidget(clearButton_);

    mainLayout_->addLayout(buttonLayout);

    connect(undoButton_, &QPushButton::clicked, this, &HistoryPanel::onUndoClicked);
    connect(redoButton_, &QPushButton::clicked, this, &HistoryPanel::onRedoClicked);
    connect(clearButton_, &QPushButton::clicked, this, &HistoryPanel::onClearClicked);

    setMinimumWidth(150);
}

void HistoryPanel::addEntry(const std::string& actionName)
{
    if (currentIndex_ >= 0 && currentIndex_ < static_cast<int>(entries_.size()) - 1) {
        entries_.erase(entries_.begin() + currentIndex_ + 1, entries_.end());
    }

    // NOLINTNEXTLINE(modernize-use-designated-initializers)
    entries_.push_back({actionName, false});
    currentIndex_ = static_cast<int>(entries_.size()) - 1;
    refreshList();
}

void HistoryPanel::clear()
{
    entries_.clear();
    currentIndex_ = -1;
    addEntry("Original");
}

void HistoryPanel::refreshList()
{
    historyList_->clear();

    for (std::size_t i = 0; i < entries_.size(); ++i) {
        auto* item = new QListWidgetItem(QString::fromStdString(entries_[i].actionName));
        item->setData(Qt::UserRole, static_cast<int>(i));

        // NOLINTNEXTLINE(modernize-use-integer-sign-comparison)
        if (static_cast<int>(i) > currentIndex_) {
            item->setForeground(Qt::gray);
            // NOLINTNEXTLINE(modernize-use-integer-sign-comparison)
        } else if (static_cast<int>(i) == currentIndex_) {
            item->setBackground(QColor(200, 220, 255));
        }

        historyList_->addItem(item);
    }

    if (currentIndex_ >= 0 && currentIndex_ < historyList_->count()) {
        historyList_->setCurrentRow(currentIndex_);
    }

    undoButton_->setEnabled(currentIndex_ > 0);
    redoButton_->setEnabled(currentIndex_ < static_cast<int>(entries_.size()) - 1);
}

void HistoryPanel::onItemClicked(QListWidgetItem* item)
{
    const int targetIndex = item->data(Qt::UserRole).toInt();
    if (targetIndex != currentIndex_) {
        currentIndex_ = targetIndex;
        refreshList();
        emit historyJumpRequested(currentIndex_);
    }
}

void HistoryPanel::onUndoClicked()
{
    if (currentIndex_ > 0) {
        --currentIndex_;
        refreshList();
        emit historyJumpRequested(currentIndex_);
    }
}

void HistoryPanel::onRedoClicked()
{
    if (currentIndex_ < static_cast<int>(entries_.size()) - 1) {
        ++currentIndex_;
        refreshList();
        emit historyJumpRequested(currentIndex_);
    }
}

void HistoryPanel::onClearClicked()
{
    clear();
}

}  // namespace gimp
