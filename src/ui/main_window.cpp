/**
 * @file main_window.cpp
 * @brief Implementation of MainWindow with dockable panel framework.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#include "ui/main_window.h"

#include "core/command_bus.h"
#include "core/document.h"
#include "core/events.h"
#include "core/filters/blur_filter.h"
#include "core/filters/sharpen_filter.h"
#include "core/layer.h"
#include "core/layer_stack.h"
#include "core/tile_store.h"
#include "core/tool_factory.h"
#include "core/tools/brush_tool.h"
#include "core/tools/color_picker_tool.h"
#include "core/tools/ellipse_selection_tool.h"
#include "core/tools/eraser_tool.h"
#include "core/tools/fill_tool.h"
#include "core/tools/gradient_tool.h"
#include "core/tools/move_tool.h"
#include "core/tools/pencil_tool.h"
#include "render/skia_renderer.h"
#include "ui/color_chooser_panel.h"
#include "ui/command_palette.h"
#include "ui/debug_hud.h"
#include "ui/history_panel.h"
#include "ui/layers_panel.h"
#include "ui/log_bridge.h"
#include "ui/log_panel.h"
#include "ui/shortcut_manager.h"
#include "ui/skia_canvas_widget.h"
#include "ui/theme.h"
#include "ui/toast_manager.h"
#include "ui/tool_options_panel.h"
#include "ui/toolbox_panel.h"

#include "history/simple_history_manager.h"

#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QStatusBar>
#include <QToolBar>

#include <spdlog/spdlog.h>

namespace {

class SimpleDocument : public gimp::Document {
  public:
    SimpleDocument(int w, int h) : m_width(w), m_height(h) {}

    std::shared_ptr<gimp::Layer> addLayer() override
    {
        auto layer = std::make_shared<gimp::Layer>(m_width, m_height);
        m_layers.addLayer(layer);
        return layer;
    }

    void removeLayer(const std::shared_ptr<gimp::Layer>& layer) override
    {
        m_layers.removeLayer(layer);
    }

    [[nodiscard]] const gimp::LayerStack& layers() const override { return m_layers; }

    gimp::TileStore& tileStore() override { return m_dummyTileStore; }

    [[nodiscard]] int width() const override { return m_width; }
    [[nodiscard]] int height() const override { return m_height; }

  private:
    int m_width;
    int m_height;
    gimp::LayerStack m_layers;

    class DummyTileStore : public gimp::TileStore {
        void invalidate(const gimp::Rect&) override {}
    } m_dummyTileStore;
};

}  // namespace

namespace gimp {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("GIMP Remake - Alpha");
    resize(1280, 900);

    m_renderer = std::make_shared<SkiaRenderer>();
    m_historyManager = std::make_unique<SimpleHistoryManager>();
    m_commandBus = std::make_unique<BasicCommandBus>(*m_historyManager);

    // Register tools with the factory
    auto& factory = ToolFactory::instance();
    factory.registerTool("pencil", []() { return std::make_unique<PencilTool>(); });
    factory.registerTool("paintbrush", []() { return std::make_unique<BrushTool>(); });
    factory.registerTool("eraser", []() { return std::make_unique<EraserTool>(); });
    factory.registerTool("move", []() { return std::make_unique<MoveTool>(); });
    factory.registerTool("color_picker", []() { return std::make_unique<ColorPickerTool>(); });
    factory.registerTool("bucket_fill", []() { return std::make_unique<FillTool>(); });
    factory.registerTool("gradient", []() { return std::make_unique<GradientTool>(); });
    factory.registerTool("select_ellipse",
                         []() { return std::make_unique<EllipseSelectTool>(); });

    // Subscribe to tool changes to update ToolFactory
    m_toolChangedSubscription =
        EventBus::instance().subscribe<ToolChangedEvent>([this](const ToolChangedEvent& event) {
            ToolFactory::instance().setActiveTool(event.currentToolId);
            auto& factory = ToolFactory::instance();
            Tool* currentTool = factory.getTool(event.currentToolId);
            onToolChanged(currentTool);
        });

    // Subscribe to color changes to update status bar and foreground color
    m_colorChangedSubscription =
        EventBus::instance().subscribe<ColorChangedEvent>([this](const ColorChangedEvent& event) {
            const std::uint32_t rgba = event.color;
            const int red = static_cast<int>((rgba >> 24) & 0xFF);
            const int green = static_cast<int>((rgba >> 16) & 0xFF);
            const int blue = static_cast<int>((rgba >> 8) & 0xFF);
            const int alpha = static_cast<int>(rgba & 0xFF);
            statusBar()->showMessage(
                QString("Color: RGB(%1, %2, %3) A:%4").arg(red).arg(green).arg(blue).arg(alpha));

            // Update global foreground color
            ToolFactory::instance().setForegroundColor(rgba);
        });

    // Create log bridge and panel
    m_logBridge = new LogBridge(this);
    m_logPanel = new LogPanel(this);
    m_logPanel->connectToBridge(m_logBridge);
    m_logBridge->start();  // start timer to drain messages

    // Create toast manager
    m_toastManager = new ToastManager(this, this);
    m_toastManager->connectToBridge(m_logBridge);

    // Register the sink with spdlog's default logger
    auto* sink = m_logBridge->sink();
    if (sink) {
        spdlog::default_logger()->sinks().push_back(
            std::shared_ptr<spdlog::sinks::sink>(sink, [](auto*) {
                // Custom deleter: sink is owned by LogBridge, do not delete here
            }));
    }

    setupMenuBar();
    setupDockWidgets();
    setupShortcuts();
    createDocument();

    statusBar()->showMessage("Ready");
}

MainWindow::~MainWindow()
{
    EventBus::instance().unsubscribe(m_toolChangedSubscription);
    EventBus::instance().unsubscribe(m_colorChangedSubscription);
}

void MainWindow::setupMenuBar()
{
    auto* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&New", QKeySequence::New, []() {});
    fileMenu->addAction("&Open", QKeySequence::Open, []() {});
    fileMenu->addSeparator();
    fileMenu->addAction("&Save", QKeySequence::Save, []() {});
    fileMenu->addAction("Save &As...", QKeySequence::SaveAs, []() {});
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", QKeySequence::Quit, this, &QMainWindow::close);

    auto* editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction("&Undo", QKeySequence::Undo, this, &MainWindow::onUndo);
    editMenu->addAction("&Redo", QKeySequence::Redo, this, &MainWindow::onRedo);
    editMenu->addSeparator();
    editMenu->addAction("Cu&t", QKeySequence::Cut, []() {});
    editMenu->addAction("&Copy", QKeySequence::Copy, []() {});
    editMenu->addAction("&Paste", QKeySequence::Paste, []() {});

    auto* viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction("Zoom &In", QKeySequence::ZoomIn, []() {});
    viewMenu->addAction("Zoom &Out", QKeySequence::ZoomOut, []() {});
    viewMenu->addAction("&Fit in Window", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E), []() {});
    viewMenu->addSeparator();

    m_toggleDebugAction = viewMenu->addAction(
        "Toggle &Debug HUD", QKeySequence(Qt::Key_F12), this, &MainWindow::onToggleDebugHud);
    m_toggleDebugAction->setCheckable(true);

    viewMenu->addAction(
        "Show &Log Panel", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L), this, [this]() {
            if (m_rightTabWidget && m_logPanel) {
                m_rightTabWidget->setCurrentWidget(m_logPanel);
            }
        });

    auto* layerMenu = menuBar()->addMenu("&Layer");
    layerMenu->addAction("&New Layer", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), []() {});
    layerMenu->addAction("&Duplicate Layer", QKeySequence(Qt::CTRL | Qt::Key_J), []() {});
    layerMenu->addAction("&Delete Layer", []() {});
    layerMenu->addSeparator();
    layerMenu->addAction("&Merge Down", []() {});
    layerMenu->addAction("&Flatten Image", []() {});

    auto* filtersMenu = menuBar()->addMenu("Filte&rs");
    filtersMenu->addAction("&Blur...", this, &MainWindow::onApplyBlur);
    filtersMenu->addAction("&Sharpen...", this, &MainWindow::onApplySharpen);

    auto* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About", []() {});
    helpMenu->addAction("Command &Palette",
                        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P),
                        this,
                        &MainWindow::onShowCommandPalette);
}

void MainWindow::setupDockWidgets()
{
    setDockNestingEnabled(true);

    const QString dockStyle = Theme::dockStyleSheet();

    // Helper to create custom title bar with bold label
    auto createTitleBar = [](const QString& title) {
        auto* titleBar = new QWidget();
        titleBar->setStyleSheet(Theme::titleBarStyleSheet());
        auto* layout = new QHBoxLayout(titleBar);
        layout->setContentsMargins(8, 4, 8, 4);
        auto* label = new QLabel(title);
        label->setStyleSheet(Theme::boldLabelStyleSheet());
        layout->addWidget(label);
        layout->addStretch();
        return titleBar;
    };

    // Toolbox at top of left dock
    m_toolboxPanel = new ToolboxPanel(this);
    m_toolboxDock = new QDockWidget(this);
    m_toolboxDock->setTitleBarWidget(createTitleBar("Toolbox"));
    m_toolboxDock->setWidget(m_toolboxPanel);
    m_toolboxDock->setStyleSheet(dockStyle);
    m_toolboxDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::LeftDockWidgetArea, m_toolboxDock);

    // Tool Options Panel under Toolbox
    m_toolOptionsPanel = new ToolOptionsPanel(this);
    m_toolOptionsDock = new QDockWidget(this);
    m_toolOptionsTitleLabel = new QLabel("Tool Options");
    m_toolOptionsTitleLabel->setStyleSheet(Theme::boldLabelStyleSheet());
    auto* optionsTitleBar = new QWidget();
    optionsTitleBar->setStyleSheet(Theme::titleBarStyleSheet());
    auto* optionsLayout = new QHBoxLayout(optionsTitleBar);
    optionsLayout->setContentsMargins(8, 4, 8, 4);
    optionsLayout->addWidget(m_toolOptionsTitleLabel);
    optionsLayout->addStretch();
    m_toolOptionsDock->setTitleBarWidget(optionsTitleBar);
    m_toolOptionsDock->setWidget(m_toolOptionsPanel);
    m_toolOptionsDock->setStyleSheet(dockStyle);
    m_toolOptionsDock->setFeatures(QDockWidget::DockWidgetMovable |
                                   QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::LeftDockWidgetArea, m_toolOptionsDock);

    m_layersPanel = new LayersPanel(this);
    m_historyPanel = new HistoryPanel(this);
    m_colorChooserPanel = new ColorChooserPanel(this);

    m_rightTabWidget = new QTabWidget(this);
    m_rightTabWidget->addTab(m_colorChooserPanel, "Colors");
    m_rightTabWidget->addTab(m_layersPanel, "Layers");
    m_rightTabWidget->addTab(m_historyPanel, "History");
    m_rightTabWidget->addTab(m_logPanel, "Log");

    m_rightDock = new QDockWidget("Panels", this);
    m_rightDock->setWidget(m_rightTabWidget);
    m_rightDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_rightDock->setMinimumWidth(200);
    addDockWidget(Qt::RightDockWidgetArea, m_rightDock);

    m_commandPalette = new CommandPalette(this);

    m_debugHud = new DebugHud(this);
    m_debugHud->setVisible(false);
}

void MainWindow::setupShortcuts()
{
    auto* paletteShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P), this);
    connect(paletteShortcut, &QShortcut::activated, this, &MainWindow::onShowCommandPalette);

    m_shortcutManager = new ShortcutManager(this);
    m_shortcutManager->registerToolShortcuts();
    m_shortcutManager->registerActionShortcuts();

    connect(m_shortcutManager,
            &ShortcutManager::toolSwitchRequested,
            this,
            &MainWindow::onToolSwitchRequested);
    connect(m_shortcutManager,
            &ShortcutManager::brushSizeDecreaseRequested,
            this,
            &MainWindow::onBrushSizeDecrease);
    connect(m_shortcutManager,
            &ShortcutManager::brushSizeIncreaseRequested,
            this,
            &MainWindow::onBrushSizeIncrease);
    connect(
        m_shortcutManager, &ShortcutManager::swapColorsRequested, this, &MainWindow::onSwapColors);
    connect(m_shortcutManager,
            &ShortcutManager::resetColorsRequested,
            this,
            &MainWindow::onResetColors);
}

void MainWindow::createDocument()
{
    m_document = std::make_shared<SimpleDocument>(800, 600);

    auto bg = m_document->addLayer();
    bg->setName("Background");
    auto* pixels = reinterpret_cast<uint32_t*>(bg->data().data());
    for (int i = 0; i < 800 * 600; ++i) {
        pixels[i] = 0xFFFFFFFF;
    }

    // Configure ToolFactory with document and command bus
    auto& factory = ToolFactory::instance();
    factory.setDocument(m_document);
    factory.setCommandBus(m_commandBus.get());
    factory.setActiveTool("paintbrush");

    // Initialize tool options panel with default tool
    onToolChanged(factory.activeTool());

    m_canvasWidget = new SkiaCanvasWidget(m_document, m_renderer, this);
    setCentralWidget(m_canvasWidget);

    // Connect performance counter signal
    connect(m_canvasWidget, &SkiaCanvasWidget::framePainted, m_debugHud, &DebugHud::onFramePainted);

    m_layersPanel->setDocument(m_document);
    m_debugHud->setDocument(m_document);
}

void MainWindow::set_document(std::shared_ptr<Document> document)
{
    m_document = std::move(document);
    if (m_canvasWidget != nullptr) {
        m_canvasWidget->update();
    }
    m_layersPanel->setDocument(m_document);
    m_debugHud->setDocument(m_document);
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_F12) {
        onToggleDebugHud();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    positionDebugHud();
    if (m_toastManager) {
        m_toastManager->repositionToasts();
    }
}

void MainWindow::onToggleDebugHud()
{
    const bool visible = !m_debugHud->isVisible();
    m_debugHud->setVisible(visible);
    m_toggleDebugAction->setChecked(visible);
    positionDebugHud();
}

void MainWindow::onShowCommandPalette()
{
    m_commandPalette->show();
}

void MainWindow::positionDebugHud()
{
    if (m_debugHud->isVisible() && m_canvasWidget != nullptr) {
        auto canvasPos = m_canvasWidget->mapTo(this, QPoint(10, 10));
        m_debugHud->move(canvasPos);
        m_debugHud->raise();
    }
}

void MainWindow::onUndo()
{
    if (m_historyManager && m_historyManager->undo()) {
        if (m_canvasWidget != nullptr) {
            m_canvasWidget->invalidateCache();
        }
        statusBar()->showMessage("Undo", 2000);
    }
}

void MainWindow::onRedo()
{
    if (m_historyManager && m_historyManager->redo()) {
        if (m_canvasWidget != nullptr) {
            m_canvasWidget->invalidateCache();
        }
        statusBar()->showMessage("Redo", 2000);
    }
}

void MainWindow::onToolChanged(const Tool* tool)
{
    if (m_toolOptionsPanel && tool) {
        m_toolOptionsPanel->setTool(const_cast<Tool*>(tool));
        if (m_toolOptionsTitleLabel) {
            m_toolOptionsTitleLabel->setText(QString::fromStdString(tool->name()));
        }
    } else if (m_toolOptionsTitleLabel) {
        m_toolOptionsTitleLabel->setText("Tool Options");
    }
}

void MainWindow::onToolSwitchRequested(const QString& toolId)
{
    EventBus::instance().publish(ToolSwitchRequestEvent{toolId.toStdString()});
}

void MainWindow::onBrushSizeDecrease()
{
    auto* tool = ToolFactory::instance().activeTool();
    if (tool == nullptr) {
        return;
    }

    int currentSize = tool->brushSize();
    int newSize = std::max(1, currentSize - 5);
    tool->setBrushSize(newSize);

    EventBus::instance().publish(ToolPropertyChangedEvent{tool->id(), "brushSize"});
    statusBar()->showMessage(QString("Brush size: %1").arg(newSize), 1000);
}

void MainWindow::onBrushSizeIncrease()
{
    auto* tool = ToolFactory::instance().activeTool();
    if (tool == nullptr) {
        return;
    }

    int currentSize = tool->brushSize();
    int newSize = std::min(500, currentSize + 5);
    tool->setBrushSize(newSize);

    EventBus::instance().publish(ToolPropertyChangedEvent{tool->id(), "brushSize"});
    statusBar()->showMessage(QString("Brush size: %1").arg(newSize), 1000);
}

void MainWindow::onSwapColors()
{
    if (m_colorChooserPanel != nullptr) {
        m_colorChooserPanel->swapColors();
        statusBar()->showMessage("Colors swapped", 1000);
    }
}

void MainWindow::onResetColors()
{
    if (m_colorChooserPanel != nullptr) {
        m_colorChooserPanel->resetToDefaults();
        statusBar()->showMessage("Colors reset to defaults", 1000);
    }
}

void MainWindow::onApplyBlur()
{
    if (!m_document || m_document->layers().count() == 0) {
        statusBar()->showMessage("No layer to apply filter", 2000);
        return;
    }

    bool ok = false;
    double radius =
        QInputDialog::getDouble(this, "Blur", "Radius (1-100):", 5.0, 1.0, 100.0, 1, &ok);

    if (!ok) {
        return;
    }

    auto layer = m_document->layers()[0];
    BlurFilter filter;
    filter.setRadius(static_cast<float>(radius));

    if (filter.apply(layer)) {
        m_canvasWidget->update();
        statusBar()->showMessage(QString("Applied blur with radius %1").arg(radius), 2000);
    } else {
        statusBar()->showMessage("Failed to apply blur filter", 2000);
    }
}

void MainWindow::onApplySharpen()
{
    if (!m_document || m_document->layers().count() == 0) {
        statusBar()->showMessage("No layer to apply filter", 2000);
        return;
    }

    bool ok = false;
    double amount =
        QInputDialog::getDouble(this, "Sharpen", "Amount (0.0-2.0):", 1.0, 0.0, 2.0, 2, &ok);

    if (!ok) {
        return;
    }

    auto layer = m_document->layers()[0];
    SharpenFilter filter;
    filter.setAmount(static_cast<float>(amount));

    if (filter.apply(layer)) {
        m_canvasWidget->update();
        statusBar()->showMessage(QString("Applied sharpen with amount %1").arg(amount), 2000);
    } else {
        statusBar()->showMessage("Failed to apply sharpen filter", 2000);
    }
}

}  // namespace gimp
