/**
 * @file gpu_context.cpp
 * @brief Implementation of Skia GPU context wrapper.
 */

#include "render/gpu_context.h"

#include <gpu/ganesh/gl/GrGLDirectContext.h>
#include <gpu/ganesh/gl/GrGLInterface.h>

#include <spdlog/spdlog.h>

namespace gimp {

GpuContext::GpuContext() = default;
GpuContext::~GpuContext() = default;

bool GpuContext::initialize(QOpenGLContext* /*qtContext*/) {
    // Skia auto-detects the GL interface from the current OpenGL context.
    // The Qt OpenGL context must be current when this is called.
    m_grContext = GrDirectContexts::MakeGL(nullptr);
    if (!m_grContext) {
        spdlog::error("Failed to create Skia GPU context");
        return false;
    }
    spdlog::info("Skia GPU context created successfully");
    return true;
}

GrDirectContext* GpuContext::grContext() const {
    return m_grContext.get();
}

void GpuContext::flush() {
    if (m_grContext) {
        m_grContext->flushAndSubmit();
    }
}

void GpuContext::resetContext() {
    if (m_grContext) {
        // Tell Skia to re-query all GL state on next render.
        // Essential after QPainter modifies GL state behind Skia's back.
        m_grContext->resetContext();
    }
}

bool GpuContext::isValid() const {
    return m_grContext != nullptr;
}

}  // namespace gimp
