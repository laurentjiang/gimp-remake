/**
 * @file gpu_context.h
 * @brief Skia GPU context wrapper with Null Object pattern for fallback support.
 *
 * Provides a clean abstraction over Skia's GrDirectContext, enabling GPU-accelerated
 * rendering when available and graceful fallback to CPU raster when not.
 * @author Laurent Jiang
 * @date 2025-02-10
 */

#pragma once

#include <memory>

#include <gpu/ganesh/GrDirectContext.h>

class QOpenGLContext;

namespace gimp {

/**
 * @brief Abstract interface for GPU context (enables Null Object pattern).
 *
 * All rendering code interacts with this interface, eliminating null checks.
 * Use GpuContext for real GPU rendering, NullGpuContext for CPU fallback.
 */
class IGpuContext {
  public:
    virtual ~IGpuContext() = default;

    /**
     * @brief Get the underlying Skia GPU context.
     * @return Pointer to GrDirectContext, or nullptr if GPU unavailable.
     */
    virtual GrDirectContext* grContext() const = 0;

    /**
     * @brief Flush pending GPU work to ensure rendering is complete.
     */
    virtual void flush() = 0;

    /**
     * @brief Reset OpenGL state tracking after external GL modifications.
     *
     * Must be called after QPainter touches OpenGL state, so Skia knows
     * to re-query all GL state on the next render.
     */
    virtual void resetContext() = 0;

    /**
     * @brief Check if GPU rendering is available.
     * @return true if GPU context was successfully initialized.
     */
    virtual bool isValid() const = 0;
};

/**
 * @brief Real GPU context using Skia's OpenGL backend.
 *
 * Wraps GrDirectContext creation and lifecycle. Initialize after Qt's
 * OpenGL context is current (e.g., in QOpenGLWidget::initializeGL()).
 */
class GpuContext : public IGpuContext {
  public:
    GpuContext();
    ~GpuContext() override;

    // Non-copyable, non-movable (owns GPU resources)
    GpuContext(const GpuContext&) = delete;
    GpuContext& operator=(const GpuContext&) = delete;
    GpuContext(GpuContext&&) = delete;
    GpuContext& operator=(GpuContext&&) = delete;

    /**
     * @brief Initialize Skia GPU context from current OpenGL context.
     * @param qtContext The Qt OpenGL context (for future extensions; currently unused).
     * @return true if GPU context was created successfully.
     * @pre An OpenGL context must be current on the calling thread.
     */
    bool initialize(QOpenGLContext* qtContext);

    GrDirectContext* grContext() const override;
    void flush() override;
    void resetContext() override;
    bool isValid() const override;

  private:
    sk_sp<GrDirectContext> m_grContext;
};

/**
 * @brief Null Object fallback when GPU is unavailable.
 *
 * All methods are no-ops or return safe defaults. This eliminates the need
 * for null checks throughout the rendering code—just call isValid() to
 * decide between GPU and raster paths.
 */
class NullGpuContext : public IGpuContext {
  public:
    GrDirectContext* grContext() const override { return nullptr; }
    void flush() override {}         // no-op
    void resetContext() override {}  // no-op
    bool isValid() const override { return false; }
};

}  // namespace gimp
