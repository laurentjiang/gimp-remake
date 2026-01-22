/**
 * @file debug_hud.cpp
 * @brief Implementation of DebugHud overlay widget.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#include "ui/debug_hud.h"

#include "core/document.h"
#include "core/events.h"
#include "core/layer_stack.h"

#ifdef _WIN32
// clang-format off
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
// clang-format on
#endif

namespace gimp {

DebugHud::DebugHud(QWidget* parent) : QWidget(parent)
{
    setupUi();

    updateTimer_ = new QTimer(this);
    connect(updateTimer_, &QTimer::timeout, this, &DebugHud::updateStats);
    updateTimer_->start(500);

    mousePosSub_ = EventBus::instance().subscribe<MousePositionChangedEvent>(
        [this](const MousePositionChangedEvent& event) {
            updateMousePosition(event.canvasX, event.canvasY);
        });

    zoomSub_ = EventBus::instance().subscribe<CanvasViewChangedEvent>(
        [this](const CanvasViewChangedEvent& event) {
            zoomLevel_ = event.zoomLevel;
            zoomLabel_->setText(QString("Zoom: %1%").arg(static_cast<int>(zoomLevel_ * 100)));
        });

    setAttribute(Qt::WA_TransparentForMouseEvents);
}

DebugHud::~DebugHud()
{
    EventBus::instance().unsubscribe(mousePosSub_);
    EventBus::instance().unsubscribe(zoomSub_);
}

void DebugHud::setupUi()
{
    setStyleSheet("DebugHud { background-color: rgba(0, 0, 0, 180); border-radius: 6px; }"
                  "QLabel { color: #00ff00; font-family: 'Consolas', 'Monaco', monospace; "
                  "font-size: 11px; }");

    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(10, 8, 10, 8);
    mainLayout_->setSpacing(2);

    auto* titleLabel = new QLabel("DEBUG HUD", this);
    titleLabel->setStyleSheet("color: #ffff00; font-weight: bold;");
    mainLayout_->addWidget(titleLabel);

    fpsLabel_ = new QLabel("FPS: --", this);
    mainLayout_->addWidget(fpsLabel_);

    memoryLabel_ = new QLabel("Memory: -- MB", this);
    mainLayout_->addWidget(memoryLabel_);

    layerCountLabel_ = new QLabel("Layers: --", this);
    mainLayout_->addWidget(layerCountLabel_);

    canvasSizeLabel_ = new QLabel("Canvas: -- x --", this);
    mainLayout_->addWidget(canvasSizeLabel_);

    mousePositionLabel_ = new QLabel("Mouse: --, --", this);
    mainLayout_->addWidget(mousePositionLabel_);

    zoomLabel_ = new QLabel("Zoom: 100%", this);
    mainLayout_->addWidget(zoomLabel_);

    setFixedWidth(180);
    adjustSize();
}

void DebugHud::setDocument(std::shared_ptr<Document> document)
{
    document_ = std::move(document);
    updateStats();
}

void DebugHud::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    if (visible) {
        updateTimer_->start(500);
    } else {
        updateTimer_->stop();
    }
}

void DebugHud::updateMousePosition(int canvasX, int canvasY)
{
    mouseX_ = canvasX;
    mouseY_ = canvasY;
    mousePositionLabel_->setText(QString("Mouse: %1, %2").arg(canvasX).arg(canvasY));
}

void DebugHud::frameRendered()
{
    frameTimes_.push_back(std::chrono::steady_clock::now());

    while (frameTimes_.size() > 60) {
        frameTimes_.pop_front();
    }
}

void DebugHud::updateStats()
{
    const double fps = calculateFps();
    fpsLabel_->setText(QString("FPS: %1").arg(fps, 0, 'f', 1));

    const std::size_t memMb = getMemoryUsage() / static_cast<std::size_t>(1024 * 1024);
    memoryLabel_->setText(QString("Memory: %1 MB").arg(memMb));

    if (document_) {
        const std::size_t layerCount = document_->layers().count();
        layerCountLabel_->setText(QString("Layers: %1").arg(layerCount));

        canvasSizeLabel_->setText(
            QString("Canvas: %1 x %2").arg(document_->width()).arg(document_->height()));
    }
}

double DebugHud::calculateFps()
{
    if (frameTimes_.size() < 2) {
        return 0.0;
    }

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(frameTimes_.back() -
                                                                          frameTimes_.front());

    if (duration.count() == 0) {
        return 0.0;
    }

    return static_cast<double>(frameTimes_.size() - 1) * 1000.0 /
           static_cast<double>(duration.count());
}

std::size_t DebugHud::getMemoryUsage()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(
            GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
        return static_cast<std::size_t>(pmc.WorkingSetSize);
    }
#endif
    return 0;
}

}  // namespace gimp
