/**
 * @file tile_store.h
 * @brief Tile storage abstraction for backing raster data.
 * @author Laurent Jiang
 * @date 2025-12-09
 */

#pragma once

#include <cstdint>

namespace gimp {
struct Rect {
    std::int32_t x;
    std::int32_t y;
    std::int32_t w;
    std::int32_t h;
};

class TileStore {
  public:
    virtual ~TileStore() = default;
    virtual void invalidate(const Rect& region) = 0;
};
}  // namespace gimp
