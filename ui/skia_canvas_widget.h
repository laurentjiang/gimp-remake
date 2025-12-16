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

class SkiaCanvasWidget : public QWidget {
    Q_OBJECT

public:
    explicit SkiaCanvasWidget(std::shared_ptr<Document> document, 
                              std::shared_ptr<SkiaRenderer> renderer, 
                              QWidget* parent = nullptr);
    ~SkiaCanvasWidget() override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::shared_ptr<Document> m_document;
    std::shared_ptr<SkiaRenderer> m_renderer;
};

} // namespace gimp
