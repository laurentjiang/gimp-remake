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
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QShortcut>
#include <QTabWidget>

#include <memory>

namespace gimp {

class BasicCommandBus;
class ColorChooserPanel;
class CommandPalette;
class DebugHud;
class Document;
class HistoryPanel;
class LayersPanel;
class ShortcutManager;
class SimpleHistoryManager;
class SkiaCanvasWidget;
class SkiaRenderer;
class Tool;
class ToolboxPanel;
class ToolOptionsPanel;

/**
 * @brief Main application window with Photoshop-like docking layout.
 *
 * Layout:
 * - Left dock: Toolbox panel and Tool Options panel (stacked)
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
    void onToolChanged(const Tool* tool);
    void onToolSwitchRequested(const QString& toolId);
    void onBrushSizeDecrease();
    void onBrushSizeIncrease();
    void onSwapColors();
    void onResetColors();
    void onApplyBlur();
    void onApplySharpen();

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
    EventBus::SubscriptionId m_colorChangedSubscription = 0;

    SkiaCanvasWidget* m_canvasWidget = nullptr;
    ToolboxPanel* m_toolboxPanel = nullptr;
    ToolOptionsPanel* m_toolOptionsPanel = nullptr;
    LayersPanel* m_layersPanel = nullptr;
    HistoryPanel* m_historyPanel = nullptr;
    ColorChooserPanel* m_colorChooserPanel = nullptr;
    CommandPalette* m_commandPalette = nullptr;
    DebugHud* m_debugHud = nullptr;
    ShortcutManager* m_shortcutManager = nullptr;

    QDockWidget* m_toolboxDock = nullptr;
    QDockWidget* m_toolOptionsDock = nullptr;
    QLabel* m_toolOptionsTitleLabel = nullptr;
    QDockWidget* m_rightDock = nullptr;
    QTabWidget* m_rightTabWidget = nullptr;

    QAction* m_toggleDebugAction = nullptr;
};

}  // namespace gimp
