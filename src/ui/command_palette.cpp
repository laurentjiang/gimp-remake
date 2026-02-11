/**
 * @file command_palette.cpp
 * @brief Implementation of CommandPalette dialog.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#include "ui/command_palette.h"

#include "core/history_manager.h"

#include <QKeyEvent>
#include <QLabel>

#include <algorithm>

namespace gimp {

CommandPalette::CommandPalette(QWidget* parent)
    : QDialog(parent, Qt::FramelessWindowHint | Qt::Popup)
{
    setupUi();

    // NOLINTBEGIN(modernize-use-designated-initializers)
    registerCommand({"file.new", "New Project", "File", "Ctrl+N", []() {
                     }});
    registerCommand({"file.open", "Open Project", "File", "Ctrl+O", []() {
                     }});
    registerCommand({"file.save", "Save Project", "File", "Ctrl+S", []() {
                     }});
    registerCommand({"file.save_as", "Save Project As...", "File", "Ctrl+Shift+S", []() {
                     }});
    registerCommand({"file.export", "Export As...", "File", "Ctrl+E", []() {
                     }});

    registerCommand({"edit.undo", "Undo", "Edit", "Ctrl+Z", [this]() {
                         if (historyManager_) {
                             historyManager_->undo();
                         }
                     }});
    registerCommand({"edit.redo", "Redo", "Edit", "Ctrl+Y", [this]() {
                         if (historyManager_) {
                             historyManager_->redo();
                         }
                     }});
    registerCommand({"edit.cut", "Cut", "Edit", "Ctrl+X", []() {
                     }});
    registerCommand({"edit.copy", "Copy", "Edit", "Ctrl+C", []() {
                     }});
    registerCommand({"edit.paste", "Paste", "Edit", "Ctrl+V", []() {
                     }});

    registerCommand({"select.all", "Select All", "Select", "Ctrl+A", []() {
                     }});
    registerCommand({"select.none", "Select None", "Select", "Ctrl+Shift+A", []() {
                     }});
    registerCommand({"select.invert", "Invert Selection", "Select", "Ctrl+I", []() {
                     }});

    registerCommand({"view.zoom_in", "Zoom In", "View", "Ctrl++", []() {
                     }});
    registerCommand({"view.zoom_out", "Zoom Out", "View", "Ctrl+-", []() {
                     }});
    registerCommand({"view.fit_window", "Fit in Window", "View", "Shift+Ctrl+E", []() {
                     }});
    registerCommand({"view.actual_size", "Actual Size (1:1)", "View", "1", []() {
                     }});
    registerCommand({"view.toggle_debug", "Toggle Debug HUD", "View", "F12", []() {
                     }});

    registerCommand({"layer.new", "New Layer", "Layer", "Shift+Ctrl+N", []() {
                     }});
    registerCommand({"layer.duplicate", "Duplicate Layer", "Layer", "Ctrl+J", []() {
                     }});
    registerCommand({"layer.delete", "Delete Layer", "Layer", "", []() {
                     }});
    registerCommand({"layer.merge_down", "Merge Down", "Layer", "Ctrl+E", []() {
                     }});
    registerCommand({"layer.flatten", "Flatten Image", "Layer", "", []() {
                     }});

    registerCommand({"image.resize", "Resize Image", "Image", "", []() {
                     }});
    registerCommand({"image.crop_to_selection", "Crop to Selection", "Image", "", []() {
                     }});
    registerCommand({"image.rotate_90cw", "Rotate 90° CW", "Image", "", []() {
                     }});
    registerCommand({"image.rotate_90ccw", "Rotate 90° CCW", "Image", "", []() {
                     }});
    registerCommand({"image.flip_horizontal", "Flip Horizontal", "Image", "", []() {
                     }});
    registerCommand({"image.flip_vertical", "Flip Vertical", "Image", "", []() {
                     }});
    // NOLINTEND(modernize-use-designated-initializers)
}

CommandPalette::~CommandPalette() = default;

void CommandPalette::setHistoryManager(HistoryManager* historyManager)
{
    historyManager_ = historyManager;
}

void CommandPalette::setupUi()
{
    setMinimumWidth(500);
    setMaximumHeight(400);

    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(8, 8, 8, 8);
    mainLayout_->setSpacing(4);

    searchBox_ = new QLineEdit(this);
    searchBox_->setPlaceholderText("Type a command...");
    searchBox_->setStyleSheet(
        "QLineEdit { padding: 8px; font-size: 14px; border: 1px solid #ccc; border-radius: 4px; }");
    mainLayout_->addWidget(searchBox_);

    resultsList_ = new QListWidget(this);
    resultsList_->setStyleSheet(
        "QListWidget { border: 1px solid #ccc; border-radius: 4px; }"
        "QListWidget::item { padding: 6px; }"
        "QListWidget::item:selected { background-color: #0078d4; color: white; }");
    mainLayout_->addWidget(resultsList_);

    connect(searchBox_, &QLineEdit::textChanged, this, &CommandPalette::onSearchTextChanged);
    connect(resultsList_, &QListWidget::itemActivated, this, &CommandPalette::onItemActivated);

    filterCommands("");
}

void CommandPalette::registerCommand(const PaletteCommand& command)
{
    commands_.push_back(command);
}

bool CommandPalette::setCommandAction(const std::string& id, std::function<void()> action)
{
    for (auto& command : commands_) {
        if (command.id == id) {
            command.action = std::move(action);
            return true;
        }
    }

    return false;
}

void CommandPalette::show()
{
    searchBox_->clear();
    filterCommands("");

    if (auto* parent = parentWidget()) {
        auto center = parent->geometry().center();
        move(center.x() - (width() / 2), parent->geometry().top() + 100);
    }

    QDialog::show();
    searchBox_->setFocus();
}

void CommandPalette::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        hide();
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        executeSelected();
        return;
    }

    if (event->key() == Qt::Key_Down) {
        const int row = resultsList_->currentRow();
        if (row < resultsList_->count() - 1) {
            resultsList_->setCurrentRow(row + 1);
        }
        return;
    }

    if (event->key() == Qt::Key_Up) {
        const int row = resultsList_->currentRow();
        if (row > 0) {
            resultsList_->setCurrentRow(row - 1);
        }
        return;
    }

    QDialog::keyPressEvent(event);
}

void CommandPalette::onSearchTextChanged(const QString& text)
{
    filterCommands(text.toStdString());
}

void CommandPalette::onItemActivated(QListWidgetItem* /*item*/)
{
    executeSelected();
}

void CommandPalette::filterCommands(const std::string& query)
{
    filteredCommands_.clear();
    resultsList_->clear();

    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    for (const auto& cmd : commands_) {
        if (query.empty()) {
            filteredCommands_.push_back(&cmd);
            continue;
        }

        std::string lowerName = cmd.name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        std::string lowerCategory = cmd.category;
        std::transform(
            lowerCategory.begin(), lowerCategory.end(), lowerCategory.begin(), [](unsigned char c) {
                return std::tolower(c);
            });

        if (lowerName.find(lowerQuery) != std::string::npos ||
            lowerCategory.find(lowerQuery) != std::string::npos) {
            filteredCommands_.push_back(&cmd);
        }
    }

    for (const auto* cmd : filteredCommands_) {
        QString displayText = QString("%1: %2").arg(QString::fromStdString(cmd->category),
                                                    QString::fromStdString(cmd->name));
        if (!cmd->shortcut.empty()) {
            displayText += QString("  [%1]").arg(QString::fromStdString(cmd->shortcut));
        }

        auto* item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, QString::fromStdString(cmd->id));
        resultsList_->addItem(item);
    }

    if (resultsList_->count() > 0) {
        resultsList_->setCurrentRow(0);
    }
}

void CommandPalette::executeSelected()
{
    auto items = resultsList_->selectedItems();
    if (items.isEmpty()) {
        hide();
        return;
    }

    const QString cmdId = items.first()->data(Qt::UserRole).toString();
    for (const auto& cmd : commands_) {
        if (cmd.id == cmdId.toStdString()) {
            hide();
            if (cmd.action) {
                cmd.action();
            }
            return;
        }
    }

    hide();
}

}  // namespace gimp
