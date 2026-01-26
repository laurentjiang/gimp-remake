/**
 * @file tile_store.h
 * @brief Tile storage abstraction for backing raster data.
 * @author Laurent Jiang
 * @date 2025-12-09
 */

#pragma once

#include <cstdint>

namespace gimp {

/*!
 * @struct Rect
 * @brief Axis-aligned rectangle for region specification.
 */
struct Rect {
    std::int32_t x;  ///< Left edge.
    std::int32_t y;  ///< Top edge.
    std::int32_t w;  ///< Width.
    std::int32_t h;  ///< Height.
};

/*!
 * @class TileStore
 * @brief Abstract interface for tile-based dirty region tracking.
 */
class TileStore {
  public:
    virtual ~TileStore() = default;

    /*!
     * @brief Marks a region as needing redraw.
     * @param region The rectangle to invalidate.
     */
    virtual void invalidate(const Rect& region) = 0;
};
}  // namespace gimp
