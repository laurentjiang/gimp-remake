/**
 * @file main_window.h
 * @brief Main application window.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#pragma once

#include <QMainWindow>

#include <memory>

namespace gimp {
class Document;
class SkiaRenderer;
class SkiaCanvasWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

  private:
    std::shared_ptr<Document> m_document;
    std::shared_ptr<SkiaRenderer> m_renderer;
    SkiaCanvasWidget* m_canvasWidget = nullptr;
};

}  // namespace gimp
