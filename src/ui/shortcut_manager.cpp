/**
 * @file shortcut_manager.cpp
 * @brief Implementation of ShortcutManager.
 * @author Laurent Jiang
 * @date 2026-01-30
 */

#include "ui/shortcut_manager.h"

#include "core/tool_registry.h"

#include <QWidget>

namespace gimp {

ShortcutManager::ShortcutManager(QWidget* parent) : QObject(parent), parentWidget_(parent) {}

ShortcutManager::~ShortcutManager() = default;

void ShortcutManager::registerShortcut(const std::string& actionId,
                                       const QKeySequence& key,
                                       const std::function<void()>& callback)
{
    auto shortcut = std::make_unique<QShortcut>(key, parentWidget_);
    shortcut->setContext(Qt::WindowShortcut);
    connect(shortcut.get(), &QShortcut::activated, callback);
    shortcuts_[actionId] = std::move(shortcut);
    callbacks_[actionId] = callback;
}

bool ShortcutManager::rebindShortcut(const std::string& actionId, const QKeySequence& newKey)
{
    auto it = shortcuts_.find(actionId);
    if (it == shortcuts_.end()) {
        return false;
    }
    it->second->setKey(newKey);
    return true;
}

QKeySequence ShortcutManager::getBinding(const std::string& actionId) const
{
    auto it = shortcuts_.find(actionId);
    if (it == shortcuts_.end()) {
        return {};
    }
    return it->second->key();
}

void ShortcutManager::registerToolShortcuts()
{
    const auto& registry = ToolRegistry::instance();

    for (const auto& tool : registry.getAllTools()) {
        if (tool.shortcut.empty()) {
            continue;
        }

        QKeySequence keySeq(QString::fromStdString(tool.shortcut));
        if (keySeq.isEmpty()) {
            continue;
        }

        std::string actionId = "tool:" + tool.id;
        std::string capturedId = tool.id;
        registerShortcut(actionId, keySeq, [this, capturedId]() {
            emit toolSwitchRequested(QString::fromStdString(capturedId));
        });
    }
}

void ShortcutManager::registerActionShortcuts()
{
    registerShortcut("action:brush_size_decrease", QKeySequence(Qt::Key_BracketLeft), [this]() {
        emit brushSizeDecreaseRequested();
    });

    registerShortcut("action:brush_size_increase", QKeySequence(Qt::Key_BracketRight), [this]() {
        emit brushSizeIncreaseRequested();
    });

    registerShortcut(
        "action:swap_colors", QKeySequence(Qt::Key_X), [this]() { emit swapColorsRequested(); });

    registerShortcut(
        "action:reset_colors", QKeySequence(Qt::Key_D), [this]() { emit resetColorsRequested(); });
}

}  // namespace gimp
