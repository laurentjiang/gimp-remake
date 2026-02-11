/**
 * @file recent_files_manager.h
 * @brief Tracks recently opened project files and persists them to settings.
 * @author Laurent Jiang
 * @date 2026-02-11
 */

#pragma once

#include <QString>
#include <QStringList>

namespace gimp {

class RecentFilesManager {
  public:
    RecentFilesManager();

    [[nodiscard]] QStringList recentFiles() const;
    void addFile(const QString& filePath);
    void removeFile(const QString& filePath);
    void clear();
    [[nodiscard]] bool isMissing(const QString& filePath) const;
    [[nodiscard]] int maxEntries() const;

  private:
    void load();
    void save() const;
    [[nodiscard]] QString normalizePath(const QString& filePath) const;
    void trimToMax();

    QStringList recentFiles_;
    int maxEntries_ = 10;
    QString settingsKey_ = "recentFiles";
};

}  // namespace gimp
