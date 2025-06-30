#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

namespace axomotor::threading {

class EventGroup
{
public:
    EventGroup();
    EventGroup(const EventGroup &) = delete;
    EventGroup(EventGroup &&) = delete;
    ~EventGroup();

    uint32_t get_flags() const;
    uint32_t set_flags(uint32_t) const;
    uint32_t clear_flags(uint32_t) const;

    uint32_t wait_for_flags(
        uint32_t flags,
        bool clear_on_exit = false,
        bool wait_for_all_flags = false,
        TickType_t ticks_to_wait = portMAX_DELAY
    ) const;

    uint32_t sync_flags(
        uint32_t flags_to_set,
        uint32_t flags_to_wait_for,
        TickType_t ticks_to_wait = portMAX_DELAY
    ) const;

    EventGroup &operator=(const EventGroup &) = delete;
    EventGroup &operator=(EventGroup &&) = delete;

private:
    EventGroupHandle_t m_handle;
};

} // namespace axomotor::threading
