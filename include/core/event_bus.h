/**
 * @file event_bus.h
 * @brief Minimal event bus stub for decoupled notifications.
 * @author Laurent Jiang
 * @date 2025-12-09
 */

#pragma once

namespace gimp {
class EventBus {
  public:
    virtual ~EventBus() = default;

    virtual void publish() = 0;  // TODO: add payloads and topics when needed.
};
}  // namespace gimp
