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

void ShortcutManager::createShortcut(const QKeySequence& key,
                                     const std::function<void()>& callback)
{
    auto shortcut = std::make_unique<QShortcut>(key, parentWidget_);
    shortcut->setContext(Qt::WindowShortcut);
    connect(shortcut.get(), &QShortcut::activated, callback);
    shortcuts_.push_back(std::move(shortcut));
}

void ShortcutManager::registerToolShortcuts()
{
    const auto& registry = ToolRegistry::instance();

    for (const auto& toolId : registry.getToolIds()) {
        const auto* tool = registry.getTool(toolId);
        if (tool == nullptr || tool->shortcut.empty()) {
            continue;
        }

        QKeySequence keySeq(QString::fromStdString(tool->shortcut));
        if (keySeq.isEmpty()) {
            continue;
        }

        std::string capturedId = toolId;
        createShortcut(keySeq, [this, capturedId]() {
            emit toolSwitchRequested(QString::fromStdString(capturedId));
        });
    }
}

void ShortcutManager::registerActionShortcuts()
{
    // Brush size decrease: [
    createShortcut(QKeySequence(Qt::Key_BracketLeft),
                   [this]() { emit brushSizeDecreaseRequested(); });

    // Brush size increase: ]
    createShortcut(QKeySequence(Qt::Key_BracketRight),
                   [this]() { emit brushSizeIncreaseRequested(); });

    // Swap colors: X
    createShortcut(QKeySequence(Qt::Key_X), [this]() { emit swapColorsRequested(); });

    // Reset to default colors: D
    createShortcut(QKeySequence(Qt::Key_D), [this]() { emit resetColorsRequested(); });
}

}  // namespace gimp

