/**
 * @file skia_canvas_widget.h
 * @brief QWidget wrapper for displaying Skia content.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#pragma once

#include <QWidget>

#include <memory>

namespace gimp {
class Document;
class SkiaRenderer;

/*!
 * @class SkiaCanvasWidget
 * @brief QWidget that displays a document rendered via Skia.
 */
class SkiaCanvasWidget : public QWidget {
    Q_OBJECT

  public:
    /*!
     * @brief Constructs the canvas widget.
     * @param document The document to display.
     * @param renderer The Skia renderer for compositing.
     * @param parent Optional parent widget.
     */
    explicit SkiaCanvasWidget(std::shared_ptr<Document> document,
                              std::shared_ptr<SkiaRenderer> renderer,
                              QWidget* parent = nullptr);
    ~SkiaCanvasWidget() override;

  protected:
    /*! @brief Paints the rendered document to the widget. */
    void paintEvent(QPaintEvent* event) override;

  private:
    std::shared_ptr<Document> m_document;   ///< Document to display.
    std::shared_ptr<SkiaRenderer> m_renderer; ///< Skia renderer.
};

}  // namespace gimp
