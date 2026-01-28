/**
 * @file main_window.h
 * @brief Main application window with dockable panel framework.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#pragma once

#include "core/event_bus.h"

#include <QAction>
#include <QDockWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QShortcut>
#include <QTabWidget>

#include <memory>

namespace gimp {

class BasicCommandBus;
class CommandPalette;
class DebugHud;
class Document;
class HistoryPanel;
class LayersPanel;
class SimpleHistoryManager;
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
    /*! @brief Constructs the main application window.
     *  @param parent Optional parent widget.
     */
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    /*! @brief Sets the active document.
     *  @param document The document to display and edit.
     */
    void set_document(std::shared_ptr<Document> document);

  protected:
    /*! @brief Handles key press events.
     *  @param event The key event.
     */
    void keyPressEvent(QKeyEvent* event) override;
    /*! @brief Handles window resize events.
     *  @param event The resize event.
     */
    void resizeEvent(QResizeEvent* event) override;

  private slots:
    void onToggleDebugHud();
    void onShowCommandPalette();
    void onUndo();
    void onRedo();

  private:
    void setupMenuBar();
    void setupToolbar();
    void setupDockWidgets();
    void setupShortcuts();
    void createDocument();
    void positionDebugHud();

    std::shared_ptr<Document> m_document;
    std::shared_ptr<SkiaRenderer> m_renderer;
    std::unique_ptr<SimpleHistoryManager> m_historyManager;
    std::unique_ptr<BasicCommandBus> m_commandBus;

    EventBus::SubscriptionId m_toolChangedSubscription = 0;

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
