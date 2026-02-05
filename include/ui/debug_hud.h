/**
 * @file debug_hud.h
 * @brief Debug heads-up display overlay widget.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#pragma once

#include "core/event_bus.h"

#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <chrono>
#include <deque>
#include <memory>

namespace gimp {

class Document;

/**
 * @brief Semi-transparent overlay displaying debug information.
 *
 * Shows FPS, frame time, memory usage, layer count, canvas size, and mouse position.
 * Can be toggled on/off via View menu or F12.
 */
class DebugHud : public QWidget {
    Q_OBJECT

  public:
    /*! @brief Constructs the debug HUD overlay.
     *  @param parent Optional parent widget.
     */
    explicit DebugHud(QWidget* parent = nullptr);
    ~DebugHud() override;

    /*! @brief Sets the document to display statistics for.
     *  @param document The active document.
     */
    void setDocument(std::shared_ptr<Document> document);
    /*! @brief Shows or hides the debug HUD.
     *  @param visible True to show, false to hide.
     */
    void setVisible(bool visible) override;

  public slots:
    /*! @brief Updates the mouse position display.
     *  @param canvasX X coordinate in canvas space.
     *  @param canvasY Y coordinate in canvas space.
     */
    void updateMousePosition(int canvasX, int canvasY);
    /*! @brief Called when a frame is rendered to update FPS. */
    void frameRendered();
    /*! @brief Called when a paint event completes, providing frame time.
     *  @param frameTimeMs The duration of the paint event in milliseconds.
     */
    void onFramePainted(double frameTimeMs);

  private slots:
    void updateStats();

  private:
    void setupUi();
    double calculateFps();
    std::size_t getMemoryUsage();

    QVBoxLayout* mainLayout_ = nullptr;
    QLabel* fpsLabel_ = nullptr;
    QLabel* memoryLabel_ = nullptr;
    QLabel* layerCountLabel_ = nullptr;
    QLabel* canvasSizeLabel_ = nullptr;
    QLabel* mousePositionLabel_ = nullptr;
    QLabel* zoomLabel_ = nullptr;
    QLabel* frameTimeLabel_ = nullptr;

    std::shared_ptr<Document> document_;
    QTimer* updateTimer_ = nullptr;

    std::deque<std::chrono::steady_clock::time_point> frameTimes_;
    int mouseX_ = 0;
    int mouseY_ = 0;
    float zoomLevel_ = 1.0F;

    EventBus::SubscriptionId mousePosSub_ = 0;
    EventBus::SubscriptionId zoomSub_ = 0;
};

}  // namespace gimp
