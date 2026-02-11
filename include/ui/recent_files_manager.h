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

/**
 * @brief Manages the list of recent project files.
 */
class RecentFilesManager {
  public:
    /*! @brief Constructs the recent files manager and loads persisted entries. */
    RecentFilesManager();

    /*! @brief Returns the recent file list.
     *  @return List of recent file paths.
     */
    [[nodiscard]] QStringList recentFiles() const;
    /*! @brief Adds a file path to the recent list.
     *  @param filePath File path to add.
     */
    void addFile(const QString& filePath);
    /*! @brief Removes a file path from the recent list.
     *  @param filePath File path to remove.
     */
    void removeFile(const QString& filePath);
    /*! @brief Clears all recent entries. */
    void clear();
    /*! @brief Returns whether a file path is missing on disk.
     *  @param filePath File path to check.
     *  @return True if the path does not exist.
     */
    [[nodiscard]] bool isMissing(const QString& filePath) const;
    /*! @brief Returns the maximum number of recent entries.
     *  @return Maximum number of entries.
     */
    [[nodiscard]] int maxEntries() const;

  private:
    /*! @brief Loads recent files from settings. */
    void load();
    /*! @brief Saves recent files to settings. */
    void save() const;
    /*! @brief Normalizes a path for storage.
     *  @param filePath File path to normalize.
     *  @return Normalized file path.
     */
    [[nodiscard]] QString normalizePath(const QString& filePath) const;
    /*! @brief Trims the list to the maximum size. */
    void trimToMax();

    QStringList recentFiles_;              ///< Stored recent file paths.
    int maxEntries_ = 10;                  ///< Maximum number of entries to keep.
    QString settingsKey_ = "recentFiles";  ///< Settings key for persistence.
};

}  // namespace gimp
