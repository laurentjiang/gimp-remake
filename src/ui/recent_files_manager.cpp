/**
 * @file recent_files_manager.cpp
 * @brief Tracks recently opened project files and persists them to settings.
 * @author Laurent Jiang
 * @date 2026-02-11
 */

#include "ui/recent_files_manager.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>

#include <QSet>

namespace gimp {

namespace {
constexpr const char* kSettingsOrganization = "GimpRemake";
constexpr const char* kSettingsApplication = "GimpRemake";
}

RecentFilesManager::RecentFilesManager()
{
    load();
}

QStringList RecentFilesManager::recentFiles() const
{
    return recentFiles_;
}

void RecentFilesManager::addFile(const QString& filePath)
{
    const QString normalized = normalizePath(filePath);
    if (normalized.isEmpty()) {
        return;
    }

    recentFiles_.removeAll(normalized);
    recentFiles_.prepend(normalized);
    trimToMax();
    save();
}

void RecentFilesManager::removeFile(const QString& filePath)
{
    const QString normalized = normalizePath(filePath);
    if (normalized.isEmpty()) {
        return;
    }

    if (recentFiles_.removeAll(normalized) > 0) {
        save();
    }
}

void RecentFilesManager::clear()
{
    if (recentFiles_.isEmpty()) {
        return;
    }

    recentFiles_.clear();
    save();
}

bool RecentFilesManager::isMissing(const QString& filePath) const
{
    if (filePath.isEmpty()) {
        return true;
    }

    QFileInfo info(filePath);
    return !info.exists();
}

int RecentFilesManager::maxEntries() const
{
    return maxEntries_;
}

void RecentFilesManager::load()
{
    QSettings settings(kSettingsOrganization, kSettingsApplication);
    const QStringList stored = settings.value(settingsKey_).toStringList();

    QSet<QString> seen;
    QStringList cleaned;
    cleaned.reserve(stored.size());

    for (const auto& entry : stored) {
        const QString normalized = normalizePath(entry);
        if (normalized.isEmpty() || seen.contains(normalized)) {
            continue;
        }
        seen.insert(normalized);
        cleaned.append(normalized);
    }

    recentFiles_ = cleaned;
    trimToMax();
    save();
}

void RecentFilesManager::save() const
{
    QSettings settings(kSettingsOrganization, kSettingsApplication);
    settings.setValue(settingsKey_, recentFiles_);
}

QString RecentFilesManager::normalizePath(const QString& filePath) const
{
    if (filePath.trimmed().isEmpty()) {
        return {};
    }

    QFileInfo info(filePath);
    const QString absolutePath = info.absoluteFilePath();
    return QDir::cleanPath(absolutePath);
}

void RecentFilesManager::trimToMax()
{
    if (recentFiles_.size() <= maxEntries_) {
        return;
    }

    recentFiles_ = recentFiles_.mid(0, maxEntries_);
}

}  // namespace gimp
