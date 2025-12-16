/**
 * @file main_window.cpp
 * @brief Implementation of MainWindow.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#include "ui/main_window.h"
#include "core/document.h"
#include "core/layer.h"
#include "core/layer_stack.h"
#include "core/tile_store.h"
#include "render/skia_renderer.h"
#include "ui/skia_canvas_widget.h"

// Minimal concrete Document for testing
class SimpleDocument : public gimp::Document {
  public:
    SimpleDocument(int w, int h) : m_width(w), m_height(h) {}

    std::shared_ptr<gimp::Layer> add_layer() override
    {
        auto layer = std::make_shared<gimp::Layer>(m_width, m_height);
        m_layers.add_layer(layer);
        return layer;
    }

    void remove_layer(const std::shared_ptr<gimp::Layer>& layer) override
    {
        m_layers.remove_layer(layer);
    }

    [[nodiscard]] const gimp::LayerStack& layers() const override { return m_layers; }

    gimp::TileStore& tile_store() override { return m_dummyTileStore; }

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

namespace gimp {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("GIMP Remake - Pre-Alpha");
    resize(1024, 768);

    m_document = std::make_shared<SimpleDocument>(800, 600);
    m_renderer = std::make_shared<SkiaRenderer>();

    auto bg = m_document->add_layer();
    bg->set_name("Background");
    auto* pixels = reinterpret_cast<uint32_t*>(bg->data().data());
    for (int i = 0; i < 800 * 600; ++i)
        pixels[i] = 0xFFFFFFFF;

    auto fg = m_document->add_layer();
    fg->set_name("Circle");
    auto* fgPixels = reinterpret_cast<uint32_t*>(fg->data().data());
    const int cx = 400;
    const int cy = 300;
    const int r = 100;
    for (int y = 0; y < 600; ++y) {
        for (int x = 0; x < 800; ++x) {
            const int dx = x - cx;
            const int dy = y - cy;
            if (dx * dx + dy * dy < r * r) {
                fgPixels[y * 800 + x] = 0xFF0000FF;
            }
        }
    }

    m_canvasWidget = new SkiaCanvasWidget(m_document, m_renderer, this);
    setCentralWidget(m_canvasWidget);
}

MainWindow::~MainWindow() = default;

}  // namespace gimp
