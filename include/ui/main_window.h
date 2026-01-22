/**
 * @file main_window.h
 * @brief Main application window with dockable panel framework.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#pragma once

#include <QAction>
#include <QDockWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QShortcut>
#include <QTabWidget>

#include <memory>

namespace gimp {

class CommandPalette;
class DebugHud;
class Document;
class HistoryPanel;
class LayersPanel;
class SkiaCanvasWidget;
class SkiaRenderer;
class ToolboxPanel;
class ToolOptionsBar;

/**
 * @brief Main application window with Photoshop-like docking layout.
 *
 * Layout:
 * - Left dock: Toolbox panel
 * - Top bar: Tool options
 * - Center: Canvas widget
 * - Right dock: Layers and History panels (tabbed)
 * - Overlay: Debug HUD (toggleable)
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void set_document(std::shared_ptr<Document> document);

  protected:
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

  private slots:
    void onToggleDebugHud();
    void onShowCommandPalette();

  private:
    void setupMenuBar();
    void setupToolbar();
    void setupDockWidgets();
    void setupShortcuts();
    void createDocument();
    void positionDebugHud();

    std::shared_ptr<Document> m_document;
    std::shared_ptr<SkiaRenderer> m_renderer;

    SkiaCanvasWidget* m_canvasWidget = nullptr;
    ToolboxPanel* m_toolboxPanel = nullptr;
    ToolOptionsBar* m_toolOptionsBar = nullptr;
    LayersPanel* m_layersPanel = nullptr;
    HistoryPanel* m_historyPanel = nullptr;
    CommandPalette* m_commandPalette = nullptr;
    DebugHud* m_debugHud = nullptr;

    QDockWidget* m_toolboxDock = nullptr;
    QDockWidget* m_rightDock = nullptr;
    QTabWidget* m_rightTabWidget = nullptr;

    QAction* m_toggleDebugAction = nullptr;
};

}  // namespace gimp
