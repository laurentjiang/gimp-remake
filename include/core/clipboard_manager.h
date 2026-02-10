/**
 * @file clipboard_manager.h
 * @brief Clipboard manager for image region copy/paste.
 * @author Aless Tosi
 * @date 2026-02-04
 */

#pragma once

#include <QImage>
#include <QPoint>

#include <memory>

namespace gimp {

class CommandBus;
class Document;
class Layer;

/**
 * @brief Singleton clipboard manager for image data.
 */
class ClipboardManager {
  public:
    /**
     * @brief Returns the singleton instance of the ClipboardManager.
     */
    static ClipboardManager& instance()
    {
        static ClipboardManager manager;
        return manager;
    }

    /**
     * @brief Returns true if clipboard has an image.
     */
    [[nodiscard]] bool hasImage() const { return !image_.isNull(); }

    /**
     * @brief Returns the stored clipboard image.
     */
    [[nodiscard]] const QImage& image() const { return image_; }

    /**
     * @brief Copies the current selection into the clipboard.
     * @param document The active document.
     * @param layer The layer to copy from (if nullptr, copies from first layer).
     * @return True if copy succeeded.
     */
    bool copySelection(const std::shared_ptr<Document>& document,
                       const std::shared_ptr<Layer>& layer = nullptr);

    /**
     * @brief Cuts the current selection into the clipboard and clears pixels.
     * @param document The active document.
     * @param layer The layer to cut from (if nullptr, cuts from first layer).
     * @param commandBus Command bus for undoable cut.
     * @return True if cut succeeded.
     */
    bool cutSelection(const std::shared_ptr<Document>& document,
                      const std::shared_ptr<Layer>& layer,
                      CommandBus* commandBus);

    /**
     * @brief Pastes the clipboard image into the document at a position.
     * @param document The active document.
     * @param commandBus Command bus for undoable paste.
     * @param canvasPos Target canvas position.
     * @param useCursor True to use canvasPos, false to center on document.
     * @return True if paste succeeded.
     */
    bool pasteToDocument(const std::shared_ptr<Document>& document,
                         CommandBus* commandBus,
                         const QPoint& canvasPos,
                         bool useCursor);

    /**
     * @brief Updates the stored image from system clipboard if available.
     * @return True if updated from system clipboard.
     */
    bool updateFromSystemClipboard();

  private:
    ClipboardManager() = default;

    void setImageInternal(const QImage& image);

    QImage image_;
};

}  // namespace gimp
